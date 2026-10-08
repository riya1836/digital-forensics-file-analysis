#include "file_analyzer.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------
 * Helper: safely join two paths
 * ------------------------------------------------------------ */
static int build_path(
    char *destination,
    size_t destination_size,
    const char *base,
    const char *name)
{
    int written;

    if (destination == NULL ||
        base == NULL ||
        name == NULL ||
        destination_size == 0)
    {
        return 0;
    }

    if (base[0] == '\0')
    {
        written = snprintf(
            destination,
            destination_size,
            "%s",
            name);
    }
    else
    {
        written = snprintf(
            destination,
            destination_size,
            "%s/%s",
            base,
            name);
    }

    if (written < 0 || (size_t)written >= destination_size)
    {
        return 0;
    }

    return 1;
}

/* ------------------------------------------------------------
 * Helper: extract filename from a path
 * ------------------------------------------------------------ */
static const char *get_filename_from_path(const char *path)
{
    const char *last_slash;

    if (path == NULL)
    {
        return "";
    }

    last_slash = strrchr(path, '/');

    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

/* ------------------------------------------------------------
 * Helper: extract file extension
 *
 * Hidden files such as ".gitkeep" are treated as having
 * no extension.
 * ------------------------------------------------------------ */
static void get_extension(
    const char *filename,
    char *extension,
    size_t extension_size)
{
    const char *last_dot;
    const char *last_slash;

    if (extension == NULL || extension_size == 0)
    {
        return;
    }

    extension[0] = '\0';

    if (filename == NULL)
    {
        return;
    }

    last_dot = strrchr(filename, '.');
    last_slash = strrchr(filename, '/');

    /* No dot. */
    if (last_dot == NULL)
    {
        return;
    }

    /* Dot belongs to a directory name. */
    if (last_slash != NULL && last_dot < last_slash)
    {
        return;
    }

    /* ".gitkeep", ".bashrc", etc. are hidden files,
       not files with an extension. */
    if (last_dot == filename)
    {
        return;
    }

    /* Filename ending in "." has no useful extension. */
    if (*(last_dot + 1) == '\0')
    {
        return;
    }

    snprintf(
        extension,
        extension_size,
        "%s",
        last_dot);
}

/* ------------------------------------------------------------
 * Helper: determine whether a filename is hidden
 * ------------------------------------------------------------ */
static int is_hidden_filename(const char *filename)
{
    if (filename == NULL || filename[0] == '\0')
    {
        return 0;
    }

    return filename[0] == '.' ? 1 : 0;
}

/* ------------------------------------------------------------
 * Helper: escape a CSV field
 *
 * A field is quoted if it contains:
 *   comma
 *   quote
 *   newline
 * ------------------------------------------------------------ */
static void write_csv_field(
    FILE *output,
    const char *field)
{
    const char *ptr;
    int needs_quotes = 0;

    if (output == NULL)
    {
        return;
    }

    if (field == NULL)
    {
        field = "";
    }

    ptr = field;

    while (*ptr != '\0')
    {
        if (*ptr == ',' ||
            *ptr == '"' ||
            *ptr == '\n' ||
            *ptr == '\r')
        {

            needs_quotes = 1;
            break;
        }

        ptr++;
    }

    if (!needs_quotes)
    {
        fputs(field, output);
        return;
    }

    fputc('"', output);

    ptr = field;

    while (*ptr != '\0')
    {
        if (*ptr == '"')
        {
            fputc('"', output);
            fputc('"', output);
        }
        else
        {
            fputc(*ptr, output);
        }

        ptr++;
    }

    fputc('"', output);
}

/* ------------------------------------------------------------
 * Helper: write one metadata record to CSV
 * ------------------------------------------------------------ */
static void write_metadata_csv(
    FILE *output,
    const FileMetadata *metadata)
{
    if (output == NULL || metadata == NULL)
    {
        return;
    }

    write_csv_field(output, metadata->path);
    fputc(',', output);

    write_csv_field(output, metadata->filename);
    fputc(',', output);

    write_csv_field(output, metadata->extension);
    fputc(',', output);

    fprintf(
        output,
        "%llu,",
        metadata->size_bytes);

    write_csv_field(output, metadata->modified_time);
    fputc(',', output);

    /*
     * Store the complete Unix permission value.
     *
     * Example:
     *   0644 -> "644"
     *   0664 -> "664"
     *   0600 -> "600"
     *   0755 -> "755"
     *
     * M2 and M3 will later interpret this value as
     * octal and determine the owner-write permission.
     */
    fprintf(
        output,
        "%03o,",
        metadata->permissions);

    fprintf(
        output,
        "%d\n",
        metadata->is_hidden);
}

/* ------------------------------------------------------------
 * Extract metadata from one regular file
 * ------------------------------------------------------------ */
int extract_file_metadata(
    const char *full_path,
    const char *relative_path,
    FileMetadata *metadata)
{
    struct stat file_info;
    struct tm time_info;
    const char *filename;

    if (full_path == NULL ||
        relative_path == NULL ||
        metadata == NULL)
    {

        return 0;
    }

    /*
     * Read filesystem metadata.
     */
    if (stat(full_path, &file_info) != 0)
    {

        fprintf(
            stderr,
            "Error: Cannot access file '%s': %s\n",
            full_path,
            strerror(errno));

        return 0;
    }

    /*
     * M1 processes regular files.
     */
    if (!S_ISREG(file_info.st_mode))
    {
        return 0;
    }

    memset(metadata, 0, sizeof(FileMetadata));

    /*
     * Relative path.
     */
    snprintf(
        metadata->path,
        sizeof(metadata->path),
        "%s",
        relative_path);

    /*
     * Filename.
     */
    filename = get_filename_from_path(relative_path);

    snprintf(
        metadata->filename,
        sizeof(metadata->filename),
        "%s",
        filename);

    /*
     * Extension.
     */
    get_extension(
        filename,
        metadata->extension,
        sizeof(metadata->extension));

    /*
     * File size.
     */
    metadata->size_bytes =
        (unsigned long long)file_info.st_size;

    /*
     * Modified time.
     *
     * Format:
     * YYYY-MM-DD HH:MM:SS
     */
    if (localtime(&file_info.st_mtime) == NULL)
    {

        fprintf(
            stderr,
            "Error: Cannot convert modification time for '%s'\n",
            full_path);

        return 0;
    }

    time_info = *localtime(&file_info.st_mtime);

    if (strftime(
            metadata->modified_time,
            sizeof(metadata->modified_time),
            "%Y-%m-%d %H:%M:%S",
            &time_info) == 0)
    {

        fprintf(
            stderr,
            "Error: Cannot format modification time for '%s'\n",
            full_path);

        return 0;
    }

    /*
     * Permissions.
     *
     * Store the COMPLETE Unix permission bits.
     *
     * Examples:
     *   0644
     *   0664
     *   0600
     *   0755
     *
     * Only the lower 9 permission bits are stored.
     */
    metadata->permissions =
        (int)(file_info.st_mode & 0777);

    /*
     * Hidden file.
     *
     * Linux convention:
     * filename beginning with '.' is hidden.
     */
    metadata->is_hidden =
        is_hidden_filename(filename);

    return 1;
}

/* ------------------------------------------------------------
 * Recursive directory scanning
 * ------------------------------------------------------------ */
static int scan_directory(
    const char *directory_path,
    const char *relative_directory,
    FILE *output)
{
    DIR *directory;
    struct dirent *entry;
    int files_processed = 0;

    if (directory_path == NULL ||
        relative_directory == NULL ||
        output == NULL)
    {

        return -1;
    }

    directory = opendir(directory_path);

    if (directory == NULL)
    {

        fprintf(
            stderr,
            "Error: Cannot open directory '%s': %s\n",
            directory_path,
            strerror(errno));

        return -1;
    }

    while ((entry = readdir(directory)) != NULL)
    {

        char full_path[MAX_PATH_LENGTH];
        char relative_path[MAX_PATH_LENGTH];
        struct stat entry_info;
        FileMetadata metadata;

        /*
         * Never recursively process "." or "..".
         */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {

            continue;
        }

        /*
         * Build complete filesystem path.
         */
        if (!build_path(
                full_path,
                sizeof(full_path),
                directory_path,
                entry->d_name))
        {

            fprintf(
                stderr,
                "Warning: Path too long, skipping '%s'\n",
                entry->d_name);

            continue;
        }

        /*
         * Build path relative to the input directory.
         */
        if (relative_directory[0] == '\0')
        {

            if (!build_path(
                    relative_path,
                    sizeof(relative_path),
                    "",
                    entry->d_name))
            {

                fprintf(
                    stderr,
                    "Warning: Relative path too long, skipping '%s'\n",
                    entry->d_name);

                continue;
            }
        }
        else
        {

            if (!build_path(
                    relative_path,
                    sizeof(relative_path),
                    relative_directory,
                    entry->d_name))
            {

                fprintf(
                    stderr,
                    "Warning: Relative path too long, skipping '%s'\n",
                    entry->d_name);

                continue;
            }
        }

        /*
         * Obtain information about this filesystem entry.
         */
        if (stat(full_path, &entry_info) != 0)
        {

            fprintf(
                stderr,
                "Warning: Cannot access '%s': %s\n",
                full_path,
                strerror(errno));

            continue;
        }

        /*
         * Recursively process directories.
         */
        if (S_ISDIR(entry_info.st_mode))
        {

            int result;

            result = scan_directory(
                full_path,
                relative_path,
                output);

            if (result < 0)
            {
                closedir(directory);
                return -1;
            }

            files_processed += result;
        }

        /*
         * Process regular files.
         */
        else if (S_ISREG(entry_info.st_mode))
        {

            if (extract_file_metadata(
                    full_path,
                    relative_path,
                    &metadata))
            {

                write_metadata_csv(
                    output,
                    &metadata);

                files_processed++;
            }
        }

        /*
         * Other filesystem objects such as symbolic links,
         * devices, sockets, etc. are ignored.
         */
    }

    closedir(directory);

    return files_processed;
}

/* ------------------------------------------------------------
 * Public directory-analysis function
 * ------------------------------------------------------------ */
int analyze_directory(
    const char *input_directory,
    const char *metadata_output)
{
    FILE *output;
    struct stat input_info;
    int files_processed;

    if (input_directory == NULL ||
        metadata_output == NULL)
    {

        fprintf(
            stderr,
            "Error: Invalid input arguments.\n");

        return -1;
    }

    /*
     * Verify input directory exists.
     */
    if (stat(input_directory, &input_info) != 0)
    {

        fprintf(
            stderr,
            "Error: Cannot access input directory '%s': %s\n",
            input_directory,
            strerror(errno));

        return -1;
    }

    if (!S_ISDIR(input_info.st_mode))
    {

        fprintf(
            stderr,
            "Error: '%s' is not a directory.\n",
            input_directory);

        return -1;
    }

    /*
     * Create output CSV.
     */
    output = fopen(metadata_output, "w");

    if (output == NULL)
    {

        fprintf(
            stderr,
            "Error: Cannot create output file '%s': %s\n",
            metadata_output,
            strerror(errno));

        return -1;
    }

    /*
     * Required CSV header.
     */
    fprintf(
        output,
        "path,filename,extension,size_bytes,modified_time,permissions,is_hidden\n");

    /*
     * Recursively scan input directory.
     */
    files_processed = scan_directory(
        input_directory,
        "",
        output);

    if (files_processed < 0)
    {
        fclose(output);
        return -1;
    }

    /*
     * Make sure all output data is written.
     */
    if (fclose(output) != 0)
    {

        fprintf(
            stderr,
            "Error: Failed to close output file '%s'.\n",
            metadata_output);

        return -1;
    }

    return files_processed;
}

/* ------------------------------------------------------------
 * Command-line program
 *
 * This main function is excluded when compiling M1 tests:
 *
 * gcc -DM1_TEST ...
 * ------------------------------------------------------------ */
#ifndef M1_TEST

int main(int argc, char *argv[])
{
    int files_processed;

    if (argc != 3)
    {

        fprintf(
            stderr,
            "Usage: %s <input_directory> <metadata_output>\n",
            argv[0]);

        fprintf(
            stderr,
            "Example: %s data/small intermediate/metadata.csv\n",
            argv[0]);

        return 1;
    }

    files_processed = analyze_directory(
        argv[1],
        argv[2]);

    if (files_processed < 0)
    {
        return 1;
    }

    printf("File analysis completed.\n");
    printf("Files processed: %d\n", files_processed);
    printf("Metadata output: %s\n", argv[2]);

    return 0;
}

#endif