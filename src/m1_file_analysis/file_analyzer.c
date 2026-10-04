#include "file_analyzer.h"

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/*
 * Convert Windows FILETIME to a readable local timestamp.
 *
 * Output format:
 * YYYY-MM-DD HH:MM:SS
 */
static void format_modified_time(FILETIME file_time,
                                 char *buffer,
                                 size_t buffer_size)
{
    FILETIME local_file_time;
    SYSTEMTIME system_time;

    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    if (!FileTimeToLocalFileTime(&file_time, &local_file_time)) {
        snprintf(buffer, buffer_size, "N/A");
        return;
    }

    if (!FileTimeToSystemTime(&local_file_time, &system_time)) {
        snprintf(buffer, buffer_size, "N/A");
        return;
    }

    snprintf(buffer,
             buffer_size,
             "%04d-%02d-%02d %02d:%02d:%02d",
             (int)system_time.wYear,
             (int)system_time.wMonth,
             (int)system_time.wDay,
             (int)system_time.wHour,
             (int)system_time.wMinute,
             (int)system_time.wSecond);
}


/*
 * Get the filename from a path.
 */
static const char *get_filename_from_path(const char *path)
{
    const char *last_backslash;
    const char *last_slash;
    const char *filename;

    last_backslash = strrchr(path, '\\');
    last_slash = strrchr(path, '/');

    if (last_backslash != NULL && last_slash != NULL) {
        filename = (last_backslash > last_slash)
                     ? last_backslash + 1
                     : last_slash + 1;
    }
    else if (last_backslash != NULL) {
        filename = last_backslash + 1;
    }
    else if (last_slash != NULL) {
        filename = last_slash + 1;
    }
    else {
        filename = path;
    }

    return filename;
}


/*
 * Get file extension.
 *
 * Example:
 * document.pdf -> .pdf
 * photo.jpg    -> .jpg
 * README       -> empty string
 */
static void get_extension(const char *filename,
                          char *extension,
                          size_t extension_size)
{
    const char *dot;
    const char *last_slash;

    if (extension == NULL || extension_size == 0) {
        return;
    }

    extension[0] = '\0';

    if (filename == NULL) {
        return;
    }

    last_slash = strrchr(filename, '\\');

    if (last_slash == NULL) {
        last_slash = strrchr(filename, '/');
    }

    dot = strrchr(filename, '.');

    if (dot == NULL) {
        return;
    }

    /*
     * Ignore a dot that occurs before the filename itself.
     */
    if (last_slash != NULL && dot < last_slash) {
        return;
    }

    /*
     * A filename such as ".gitignore" is treated as having
     * no extension.
     */
    if (dot == filename) {
        return;
    }

    strncpy(extension, dot, extension_size - 1);
    extension[extension_size - 1] = '\0';
}


/*
 * Determine whether the file is hidden in Windows.
 */
static int is_hidden_file(const char *full_path)
{
    DWORD attributes;

    attributes = GetFileAttributesA(full_path);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }

    if ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0) {
        return 1;
    }

    return 0;
}


/*
 * Determine whether the current user has write access to the file.
 *
 * The README requires a permissions field and the later modules
 * interpret this as whether the file is writable.
 *
 * 1 = writable
 * 0 = not writable
 */
static unsigned int get_permissions(const char *full_path)
{
    if (_access(full_path, 2) == 0) {
        return 1;
    }

    return 0;
}


/*
 * Get the file size using Windows file information.
 */
static unsigned long long get_file_size(
    const WIN32_FILE_ATTRIBUTE_DATA *file_data)
{
    ULARGE_INTEGER file_size;

    file_size.LowPart = file_data->nFileSizeLow;
    file_size.HighPart = file_data->nFileSizeHigh;

    return (unsigned long long)file_size.QuadPart;
}


/*
 * Extract metadata for a single file.
 */
int extract_file_metadata(const char *full_path,
                          const char *relative_path,
                          FileMetadata *metadata)
{
    WIN32_FILE_ATTRIBUTE_DATA file_data;
    const char *filename;

    if (full_path == NULL ||
        relative_path == NULL ||
        metadata == NULL) {
        return 0;
    }

    if (!GetFileAttributesExA(full_path,
                              GetFileExInfoStandard,
                              &file_data)) {
        return 0;
    }

    /*
     * Make sure the path refers to a file and not a directory.
     */
    if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return 0;
    }

    memset(metadata, 0, sizeof(FileMetadata));

    /*
     * Path
     */
    strncpy(metadata->path,
            relative_path,
            PATH_BUFFER_SIZE - 1);

    metadata->path[PATH_BUFFER_SIZE - 1] = '\0';

    /*
     * Filename
     */
    filename = get_filename_from_path(relative_path);

    strncpy(metadata->filename,
            filename,
            PATH_BUFFER_SIZE - 1);

    metadata->filename[PATH_BUFFER_SIZE - 1] = '\0';

    /*
     * Extension
     */
    get_extension(metadata->filename,
                  metadata->extension,
                  EXTENSION_BUFFER_SIZE);

    /*
     * File size
     */
    metadata->size_bytes = get_file_size(&file_data);

    /*
     * Modified time
     */
    format_modified_time(file_data.ftLastWriteTime,
                          metadata->modified_time,
                          TIME_BUFFER_SIZE);

    /*
     * Permissions
     */
    metadata->permissions = get_permissions(full_path);

    /*
     * Hidden flag
     */
    metadata->is_hidden = is_hidden_file(full_path);

    return 1;
}


/*
 * Write one CSV field.
 *
 * If a field contains a comma, quote, or newline,
 * it is enclosed in double quotes.
 *
 * A double quote inside the field is represented by
 * two double quotes.
 */
void write_csv_field(FILE *output, const char *value)
{
    const char *current;
    int needs_quotes;

    if (output == NULL || value == NULL) {
        return;
    }

    needs_quotes = 0;
    current = value;

    while (*current != '\0') {
        if (*current == ',' ||
            *current == '"' ||
            *current == '\n' ||
            *current == '\r') {
            needs_quotes = 1;
            break;
        }

        current++;
    }

    if (!needs_quotes) {
        fputs(value, output);
        return;
    }

    fputc('"', output);

    current = value;

    while (*current != '\0') {
        if (*current == '"') {
            fputc('"', output);
            fputc('"', output);
        }
        else {
            fputc(*current, output);
        }

        current++;
    }

    fputc('"', output);
}


/*
 * Write one metadata record to the CSV file.
 */
void write_metadata_csv(FILE *output, const FileMetadata *metadata)
{
    if (output == NULL || metadata == NULL) {
        return;
    }

    write_csv_field(output, metadata->path);
    fputc(',', output);

    write_csv_field(output, metadata->filename);
    fputc(',', output);

    write_csv_field(output, metadata->extension);
    fputc(',', output);

    fprintf(output,
            "%llu,",
            metadata->size_bytes);

    write_csv_field(output, metadata->modified_time);
    fputc(',', output);

    fprintf(output,
            "%u,",
            metadata->permissions);

    fprintf(output,
            "%d\n",
            metadata->is_hidden);
}


/*
 * Recursively scan a directory.
 */
static int scan_directory(const char *directory,
                          const char *relative_directory,
                          FILE *output)
{
    char search_pattern[PATH_BUFFER_SIZE];
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    int files_processed;

    files_processed = 0;

    /*
     * Build:
     * directory\*
     */
    snprintf(search_pattern,
             sizeof(search_pattern),
             "%s\\*",
             directory);

    find_handle = FindFirstFileA(search_pattern, &find_data);

    if (find_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr,
                "Warning: unable to access directory: %s\n",
                directory);
        return 0;
    }

    do {
        char full_path[PATH_BUFFER_SIZE];
        char relative_path[PATH_BUFFER_SIZE];

        /*
         * Ignore "." and "..".
         */
        if (strcmp(find_data.cFileName, ".") == 0 ||
            strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        /*
         * Build full path.
         */
        snprintf(full_path,
                 sizeof(full_path),
                 "%s\\%s",
                 directory,
                 find_data.cFileName);

        /*
         * Build path relative to the input directory.
         */
        if (relative_directory == NULL ||
            relative_directory[0] == '\0') {

            snprintf(relative_path,
                     sizeof(relative_path),
                     "%s",
                     find_data.cFileName);
        }
        else {
            snprintf(relative_path,
                     sizeof(relative_path),
                     "%s\\%s",
                     relative_directory,
                     find_data.cFileName);
        }

        /*
         * If this is a directory, recursively scan it.
         */
        if ((find_data.dwFileAttributes &
             FILE_ATTRIBUTE_DIRECTORY) != 0) {

            /*
             * Skip reparse points such as symbolic links/junctions.
             * This prevents accidental recursive loops.
             */
            if ((find_data.dwFileAttributes &
                 FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
                continue;
            }

            files_processed += scan_directory(full_path,
                                               relative_path,
                                               output);
        }
        else {
            FileMetadata metadata;

            if (extract_file_metadata(full_path,
                                      relative_path,
                                      &metadata)) {

                write_metadata_csv(output, &metadata);
                files_processed++;
            }
            else {
                fprintf(stderr,
                        "Warning: unable to analyze file: %s\n",
                        full_path);
            }
        }

    } while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);

    return files_processed;
}


/*
 * Main M1 analysis function.
 */
int analyze_directory(const char *input_directory,
                      const char *output_file)
{
    FILE *output;
    DWORD attributes;
    int files_processed;

    if (input_directory == NULL ||
        output_file == NULL) {
        fprintf(stderr,
                "Error: invalid input or output path.\n");
        return 0;
    }

    /*
     * Verify that the input path exists.
     */
    attributes = GetFileAttributesA(input_directory);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr,
                "Error: input directory does not exist: %s\n",
                input_directory);
        return 0;
    }

    /*
     * Verify that the input path is actually a directory.
     */
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        fprintf(stderr,
                "Error: input path is not a directory: %s\n",
                input_directory);
        return 0;
    }

    /*
     * Open output CSV.
     */
    output = fopen(output_file, "w");

    if (output == NULL) {
        fprintf(stderr,
                "Error: unable to create output file: %s\n",
                output_file);
        return 0;
    }

    /*
     * Required CSV header from the README.
     */
    fprintf(output,
            "path,filename,extension,size_bytes,modified_time,permissions,is_hidden\n");

    /*
     * Recursively scan all files.
     */
    files_processed = scan_directory(input_directory,
                                      "",
                                      output);

    fclose(output);

    return files_processed;
}


/*
 * Program entry point.
 *
 * Usage:
 *     file_analyzer <input_directory> <metadata_output>
 *
 * Example:
 *     file_analyzer data/small intermediate/metadata.csv
 */
#ifndef M1_TEST

int main(int argc, char *argv[])
{
    int files_processed;

    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <input_directory> <metadata_output>\n",
                argv[0]);

        fprintf(stderr,
                "Example: %s data\\small intermediate\\metadata.csv\n",
                argv[0]);

        return 1;
    }

    files_processed = analyze_directory(argv[1], argv[2]);

    if (files_processed < 0) {
        return 1;
    }

    printf("File analysis completed.\n");
    printf("Files processed: %d\n", files_processed);
    printf("Metadata output: %s\n", argv[2]);

    return 0;
}

#endif