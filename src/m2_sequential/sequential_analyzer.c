#include "sequential_analyzer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE 4096

#define ONE_MB 1048576LL
#define ONE_HUNDRED_MB 104857600LL

/* ---------------------------------------------------------
   Utility functions
   --------------------------------------------------------- */

static void trim_newline(char *str)
{
    size_t len;

    if (str == NULL) {
        return;
    }

    len = strlen(str);

    while (len > 0 &&
           (str[len - 1] == '\n' ||
            str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static void trim_spaces(char *str)
{
    char *start;
    char *end;

    if (str == NULL || *str == '\0') {
        return;
    }

    start = str;

    while (*start != '\0' &&
           isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    end = str + strlen(str);

    while (end > str &&
           isspace((unsigned char)*(end - 1))) {
        end--;
    }

    *end = '\0';
}

/* ---------------------------------------------------------
   CSV parsing
   --------------------------------------------------------- */

static int split_csv(char *line,
                     char *fields[],
                     int max_fields)
{
    int count = 0;
    char *token;

    token = strtok(line, ",");

    while (token != NULL && count < max_fields) {
        trim_spaces(token);
        fields[count++] = token;
        token = strtok(NULL, ",");
    }

    return count;
}

/* ---------------------------------------------------------
   Extension categorization
   --------------------------------------------------------- */

static void to_lower_string(char *str)
{
    size_t i;

    if (str == NULL) {
        return;
    }

    for (i = 0; str[i] != '\0'; i++) {
        str[i] = (char)tolower(
            (unsigned char)str[i]
        );
    }
}

static const char *get_extension_category(const char *extension)
{
    char ext[64];

    if (extension == NULL) {
        return "OTHER";
    }

    snprintf(ext, sizeof(ext), "%s", extension);
    to_lower_string(ext);

    /* DOCUMENT */
    if (strcmp(ext, ".txt") == 0 ||
        strcmp(ext, ".pdf") == 0 ||
        strcmp(ext, ".doc") == 0 ||
        strcmp(ext, ".docx") == 0 ||
        strcmp(ext, ".xls") == 0 ||
        strcmp(ext, ".xlsx") == 0 ||
        strcmp(ext, ".ppt") == 0 ||
        strcmp(ext, ".pptx") == 0 ||
        strcmp(ext, ".csv") == 0) {
        return "DOCUMENT";
    }

    /* IMAGE */
    if (strcmp(ext, ".jpg") == 0 ||
        strcmp(ext, ".jpeg") == 0 ||
        strcmp(ext, ".png") == 0 ||
        strcmp(ext, ".gif") == 0 ||
        strcmp(ext, ".bmp") == 0 ||
        strcmp(ext, ".tiff") == 0 ||
        strcmp(ext, ".webp") == 0) {
        return "IMAGE";
    }

    /* AUDIO */
    if (strcmp(ext, ".mp3") == 0 ||
        strcmp(ext, ".wav") == 0 ||
        strcmp(ext, ".flac") == 0 ||
        strcmp(ext, ".aac") == 0 ||
        strcmp(ext, ".ogg") == 0) {
        return "AUDIO";
    }

    /* VIDEO */
    if (strcmp(ext, ".mp4") == 0 ||
        strcmp(ext, ".avi") == 0 ||
        strcmp(ext, ".mkv") == 0 ||
        strcmp(ext, ".mov") == 0 ||
        strcmp(ext, ".wmv") == 0 ||
        strcmp(ext, ".webm") == 0) {
        return "VIDEO";
    }

    /* ARCHIVE */
    if (strcmp(ext, ".zip") == 0 ||
        strcmp(ext, ".tar") == 0 ||
        strcmp(ext, ".gz") == 0 ||
        strcmp(ext, ".bz2") == 0 ||
        strcmp(ext, ".7z") == 0 ||
        strcmp(ext, ".rar") == 0) {
        return "ARCHIVE";
    }

    /* CODE */
    if (strcmp(ext, ".c") == 0 ||
        strcmp(ext, ".h") == 0 ||
        strcmp(ext, ".cpp") == 0 ||
        strcmp(ext, ".hpp") == 0 ||
        strcmp(ext, ".java") == 0 ||
        strcmp(ext, ".py") == 0 ||
        strcmp(ext, ".js") == 0 ||
        strcmp(ext, ".ts") == 0 ||
        strcmp(ext, ".html") == 0 ||
        strcmp(ext, ".css") == 0) {
        return "CODE";
    }

    return "OTHER";
}

/* ---------------------------------------------------------
   Size categorization
   --------------------------------------------------------- */

static const char *get_size_category(long long size_bytes)
{
    if (size_bytes < ONE_MB) {
        return "SMALL";
    }

    if (size_bytes < ONE_HUNDRED_MB) {
        return "MEDIUM";
    }

    return "LARGE";
}

/* ---------------------------------------------------------
   Category weights
   --------------------------------------------------------- */

static int get_extension_weight(const char *category)
{
    if (strcmp(category, "DOCUMENT") == 0) {
        return 1;
    }

    if (strcmp(category, "IMAGE") == 0) {
        return 2;
    }

    if (strcmp(category, "AUDIO") == 0) {
        return 2;
    }

    if (strcmp(category, "VIDEO") == 0) {
        return 3;
    }

    if (strcmp(category, "ARCHIVE") == 0) {
        return 3;
    }

    if (strcmp(category, "CODE") == 0) {
        return 2;
    }

    return 1;
}

static int get_size_weight(const char *category)
{
    if (strcmp(category, "SMALL") == 0) {
        return 1;
    }

    if (strcmp(category, "MEDIUM") == 0) {
        return 2;
    }

    return 3;
}

/* ---------------------------------------------------------
   Permission weight
   --------------------------------------------------------- */

static int get_permission_weight(const char *permissions)
{
    int value;

    if (permissions == NULL ||
        *permissions == '\0') {
        return 0;
    }

    /*
     * Permissions are normally represented as
     * Unix-style values such as 644, 664, 600.
     *
     * Convert decimal representation to octal.
     */
    value = atoi(permissions);

    /*
     * Owner-write bit is 0200 in Unix permissions.
     */
    if ((value & 0200) != 0) {
        return 1;
    }

    /*
     * Some datasets may provide symbolic permissions,
     * e.g. -rw-r--r--.
     */
    if (strchr(permissions, 'w') != NULL) {
        return 1;
    }

    return 0;
}

/* ---------------------------------------------------------
   Risk classification
   --------------------------------------------------------- */

static const char *get_risk_label(int score)
{
    if (score <= 3) {
        return "LOW";
    }

    if (score <= 6) {
        return "MEDIUM";
    }

    return "HIGH";
}

/* ---------------------------------------------------------
   Analyze one record
   --------------------------------------------------------- */

void analyze_record(const MetadataRecord *record,
                    AnalysisResult *result)
{
    const char *extension_category;
    const char *size_category;

    int extension_weight;
    int size_weight;
    int hidden_weight;
    int permission_weight;

    if (record == NULL || result == NULL) {
        return;
    }

    extension_category =
        get_extension_category(record->extension);

    size_category =
        get_size_category(record->size_bytes);

    extension_weight =
        get_extension_weight(extension_category);

    size_weight =
        get_size_weight(size_category);

    hidden_weight =
        record->is_hidden ? 2 : 0;

    permission_weight =
        get_permission_weight(record->permissions);

    snprintf(result->extension_category,
             sizeof(result->extension_category),
             "%s",
             extension_category);

    snprintf(result->size_category,
             sizeof(result->size_category),
             "%s",
             size_category);

    result->score =
        extension_weight +
        size_weight +
        hidden_weight +
        permission_weight;

    snprintf(result->risk_label,
             sizeof(result->risk_label),
             "%s",
             get_risk_label(result->score));
}

/* ---------------------------------------------------------
   Sequential processing
   --------------------------------------------------------- */

int process_metadata(const char *input_csv,
                     const char *output_csv,
                     double *execution_time_seconds)
{
    FILE *input;
    FILE *output;

    char line[LINE_SIZE];

    int first_line = 1;
    int line_number = 0;

    clock_t start;
    clock_t end;

    input = fopen(input_csv, "r");

    if (input == NULL) {
        perror("Error opening input CSV");
        return -1;
    }

    output = fopen(output_csv, "w");

    if (output == NULL) {
        perror("Error opening output CSV");
        fclose(input);
        return -1;
    }

    fprintf(output,
            "path,extension_category,size_category,score,risk_label\n");

    start = clock();

    while (fgets(line, sizeof(line), input) != NULL) {

        char *fields[7];

        MetadataRecord record;
        AnalysisResult result;

        int field_count;

        line_number++;

        trim_newline(line);

        /*
         * Skip CSV header.
         */
        if (first_line) {
            first_line = 0;
            continue;
        }

        /*
         * Skip empty lines.
         */
        if (strlen(line) == 0) {
            continue;
        }

        field_count =
            split_csv(line, fields, 7);

        if (field_count != 7) {
            fprintf(stderr,
                    "Warning: invalid record at line %d\n",
                    line_number);
            continue;
        }

        memset(&record, 0, sizeof(record));

        snprintf(record.path,
                 sizeof(record.path),
                 "%s",
                 fields[0]);

        snprintf(record.filename,
                 sizeof(record.filename),
                 "%s",
                 fields[1]);

        snprintf(record.extension,
                 sizeof(record.extension),
                 "%s",
                 fields[2]);

        record.size_bytes =
            atoll(fields[3]);

        snprintf(record.modified_time,
                 sizeof(record.modified_time),
                 "%s",
                 fields[4]);

        snprintf(record.permissions,
                 sizeof(record.permissions),
                 "%s",
                 fields[5]);

        record.is_hidden =
            atoi(fields[6]);

        analyze_record(&record, &result);

        fprintf(output,
                "%s,%s,%s,%d,%s\n",
                record.path,
                result.extension_category,
                result.size_category,
                result.score,
                result.risk_label);
    }

    end = clock();

    fclose(input);
    fclose(output);

    if (execution_time_seconds != NULL) {
        *execution_time_seconds =
            (double)(end - start) /
            (double)CLOCKS_PER_SEC;
    }

    return 0;
}

/* ---------------------------------------------------------
   Main program
   --------------------------------------------------------- */

#ifndef M2_NO_MAIN

int main(int argc, char *argv[])
{
    const char *input_csv;
    const char *output_csv;

    double execution_time = 0.0;

    if (argc == 1) {

        input_csv =
            "intermediate/metadata.csv";

        output_csv =
            "outputs/sequential.csv";

    } else if (argc == 3) {

        input_csv = argv[1];
        output_csv = argv[2];

    } else {

        fprintf(stderr,
                "Usage: %s [input_csv output_csv]\n",
                argv[0]);

        return EXIT_FAILURE;
    }

    if (process_metadata(input_csv,
                         output_csv,
                         &execution_time) != 0) {

        return EXIT_FAILURE;
    }

    printf("Sequential analysis completed successfully.\n");
    printf("Input : %s\n", input_csv);
    printf("Output: %s\n", output_csv);
    printf("Execution time: %.9f seconds\n",
           execution_time);

    return EXIT_SUCCESS;
}

#endif// M2: sequential computation