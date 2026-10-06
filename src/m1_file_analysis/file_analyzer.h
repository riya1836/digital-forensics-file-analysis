#ifndef FILE_ANALYZER_H
#define FILE_ANALYZER_H

#include <stddef.h>

#define MAX_PATH_LENGTH 4096
#define MAX_TIME_LENGTH 32
#define MAX_PERMISSION_LENGTH 16

typedef struct {
    char path[MAX_PATH_LENGTH];
    char filename[MAX_PATH_LENGTH];
    char extension[64];
    unsigned long long size_bytes;
    char modified_time[MAX_TIME_LENGTH];
    int permissions;
    int is_hidden;
} FileMetadata;

/*
 * Extract metadata from one regular file.
 *
 * Returns:
 *   1 on success
 *   0 on failure
 */
int extract_file_metadata(
    const char *full_path,
    const char *relative_path,
    FileMetadata *metadata
);

/*
 * Scan an input directory recursively and create the metadata CSV.
 *
 * Returns:
 *   number of files processed on success
 *   -1 on directory/output error
 */
int analyze_directory(
    const char *input_directory,
    const char *metadata_output
);

#endif