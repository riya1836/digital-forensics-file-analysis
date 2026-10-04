#ifndef FILE_ANALYZER_H
#define FILE_ANALYZER_H

#include <stdio.h>

#define PATH_BUFFER_SIZE 4096
#define TIME_BUFFER_SIZE 64
#define EXTENSION_BUFFER_SIZE 256

typedef struct {
    char path[PATH_BUFFER_SIZE];
    char filename[PATH_BUFFER_SIZE];
    char extension[EXTENSION_BUFFER_SIZE];
    unsigned long long size_bytes;
    char modified_time[TIME_BUFFER_SIZE];
    unsigned int permissions;
    int is_hidden;
} FileMetadata;

/* Scan input directory recursively and write metadata to CSV. */
int analyze_directory(const char *input_directory, const char *output_file);

/* Extract metadata for one file. */
int extract_file_metadata(const char *full_path,
                          const char *relative_path,
                          FileMetadata *metadata);

/* Write one metadata record to the CSV file. */
void write_metadata_csv(FILE *output, const FileMetadata *metadata);

/* Escape a value according to CSV rules. */
void write_csv_field(FILE *output, const char *value);

#endif