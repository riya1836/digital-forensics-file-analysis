#include "parallel_analyzer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>
#include <time.h>

#define INITIAL_CAPACITY 1024
#define MB (1024LL * 1024LL)

/* ============================================================
   Utility Functions
   ============================================================ */

static void trim_newline(char *str)
{
    size_t len = strlen(str);

    while (len > 0 &&
           (str[len - 1] == '\n' ||
            str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static void copy_string_safe(char *destination,
                             const char *source,
                             size_t destination_size)
{
    if (destination_size == 0)
        return;

    strncpy(destination, source, destination_size - 1);
    destination[destination_size - 1] = '\0';
}

/*
 * Convert a string to lowercase.
 */
static void lowercase_string(const char *input,
                             char *output,
                             size_t output_size)
{
    size_t i;

    if (output_size == 0)
        return;

    for (i = 0; i < output_size - 1 && input[i] != '\0'; i++) {
        output[i] = (char)tolower((unsigned char)input[i]);
    }

    output[i] = '\0';
}


/* ============================================================
   CSV Parser
   ============================================================ */

/*
 * Parse one CSV line.

 * Expected columns:
 *
 * path,
 * filename,
 * extension,
 * size_bytes,
 * modified_time,
 * permissions,
 * is_hidden
 *
 * We only need:
 *   path
 *   extension
 *   size_bytes
 *   permissions
 *   is_hidden
 *
 * The parser supports quoted CSV fields and commas inside
 * quoted fields.
 */
static int parse_csv_line(const char *line,
                          MetadataRecord *record)
{
    char fields[7][MAX_PATH_LEN];
    int field_index = 0;
    int char_index = 0;
    int in_quotes = 0;

    const char *p = line;

    memset(fields, 0, sizeof(fields));
    memset(record, 0, sizeof(MetadataRecord));

    while (*p != '\0' && *p != '\n' && *p != '\r') {

        if (*p == '"') {

            if (in_quotes && *(p + 1) == '"') {
                if (field_index < 7 &&
                    char_index < MAX_PATH_LEN - 1) {
                    fields[field_index][char_index++] = '"';
                }

                p += 2;
                continue;
            }

            in_quotes = !in_quotes;
            p++;
            continue;
        }

        if (*p == ',' && !in_quotes) {

            if (field_index >= 6)
                return 0;

            fields[field_index][char_index] = '\0';
            field_index++;
            char_index = 0;

            p++;
            continue;
        }

        if (field_index < 7 &&
            char_index < MAX_PATH_LEN - 1) {

            fields[field_index][char_index++] = *p;
        }

        p++;
    }

    if (field_index >= 7)
        return 0;

    fields[field_index][char_index] = '\0';
    field_index++;

    if (field_index != 7)
        return 0;

    copy_string_safe(
        record->path,
        fields[0],
        sizeof(record->path)
    );

    copy_string_safe(
        record->extension,
        fields[2],
        sizeof(record->extension)
    );

    record->size_bytes = atoll(fields[3]);

    copy_string_safe(
        record->permissions,
        fields[5],
        sizeof(record->permissions)
    );

    record->is_hidden = atoi(fields[6]);

    return 1;
}


/* ============================================================
   Read Metadata CSV
   ============================================================ */

int read_metadata_csv(const char *filename,
                      MetadataRecord **records,
                      size_t *count)
{
    FILE *file;
    char line[16384];

    size_t capacity = INITIAL_CAPACITY;
    size_t current_count = 0;

    MetadataRecord *data;

    file = fopen(filename, "r");

    if (file == NULL) {
        perror("Error opening metadata CSV");
        return 0;
    }

    data = malloc(capacity * sizeof(MetadataRecord));

    if (data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(file);
        return 0;
    }

    /*
     * Skip CSV header.
     */
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Metadata CSV is empty.\n");

        free(data);
        fclose(file);

        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {

        MetadataRecord record;

        trim_newline(line);

        if (strlen(line) == 0)
            continue;

        if (!parse_csv_line(line, &record)) {
            fprintf(stderr,
                    "Warning: could not parse CSV line. Skipping.\n");
            continue;
        }

        /*
         * Expand array if necessary.
         */
        if (current_count >= capacity) {

            size_t new_capacity = capacity * 2;

            MetadataRecord *temp =
                realloc(
                    data,
                    new_capacity * sizeof(MetadataRecord)
                );

            if (temp == NULL) {

                fprintf(stderr,
                        "Memory allocation failed.\n");

                free(data);
                fclose(file);

                return 0;
            }

            data = temp;
            capacity = new_capacity;
        }

        data[current_count] = record;
        current_count++;
    }

    fclose(file);

    *records = data;
    *count = current_count;

    return 1;
}


/* ============================================================
   Extension Categorization
   ============================================================ */

const char *categorize_extension(const char *extension)
{
    char ext[MAX_FIELD_LEN];

    lowercase_string(
        extension,
        ext,
        sizeof(ext)
    );

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


/* ============================================================
   Size Categorization
   ============================================================ */

const char *categorize_size(long long size_bytes)
{
    if (size_bytes < MB) {
        return "SMALL";
    }

    if (size_bytes < 100 * MB) {
        return "MEDIUM";
    }

    return "LARGE";
}


/* ============================================================
   Permission Weight
   ============================================================ */

/*
 * Permission weight:
 *
 * 1 = owner has write permission
 * 0 = otherwise
 *
 * Metadata permissions are expected in forms such as:
 *
 * 644
 * 664
 * 755
 *
 * We check the owner-write bit.
 */
static int get_permission_weight(const char *permissions)
{
    long permission_value;

    if (permissions == NULL ||
        permissions[0] == '\0') {

        return 0;
    }

    permission_value = strtol(
        permissions,
        NULL,
        8
    );

    if ((permission_value & 0200) != 0) {
        return 1;
    }

    return 0;
}


/* ============================================================
   Score Calculation
   ============================================================ */

int calculate_score(const MetadataRecord *record,
                    const char *extension_category,
                    const char *size_category)
{
    int category_weight = 1;
    int size_weight = 1;
    int hidden_weight = 0;
    int permission_weight = 0;

    /*
     * Category weight
     */
    if (strcmp(extension_category, "DOCUMENT") == 0) {
        category_weight = 1;
    }
    else if (strcmp(extension_category, "IMAGE") == 0) {
        category_weight = 2;
    }
    else if (strcmp(extension_category, "AUDIO") == 0) {
        category_weight = 2;
    }
    else if (strcmp(extension_category, "VIDEO") == 0) {
        category_weight = 3;
    }
    else if (strcmp(extension_category, "ARCHIVE") == 0) {
        category_weight = 3;
    }
    else if (strcmp(extension_category, "CODE") == 0) {
        category_weight = 2;
    }
    else {
        category_weight = 1;
    }

    /*
     * Size weight
     */
    if (strcmp(size_category, "SMALL") == 0) {
        size_weight = 1;
    }
    else if (strcmp(size_category, "MEDIUM") == 0) {
        size_weight = 2;
    }
    else {
        size_weight = 3;
    }

    /*
     * Hidden-file weight
     */
    if (record->is_hidden) {
        hidden_weight = 2;
    }

    /*
     * Permission weight
     */
    permission_weight =
        get_permission_weight(record->permissions);

    return category_weight +
           size_weight +
           hidden_weight +
           permission_weight;
}


/* ============================================================
   Risk Label
   ============================================================ */

const char *calculate_risk_label(int score)
{
    if (score <= 3) {
        return "LOW";
    }

    if (score <= 6) {
        return "MEDIUM";
    }

    return "HIGH";
}


/* ============================================================
   Parallel Analysis
   ============================================================ */

int analyze_parallel(const MetadataRecord *records,
                     AnalysisResult *results,
                     size_t count,
                     int num_threads)
{
    size_t i;

    if (records == NULL ||
        results == NULL ||
        count == 0) {

        return 0;
    }

    if (num_threads <= 0) {
        fprintf(stderr,
                "Invalid thread count: %d\n",
                num_threads);

        return 0;
    }

    /*
     * OpenMP parallelization:
     *
     * Every metadata record can be analyzed independently.
     *
     * Each iteration writes only to results[i].
     * Therefore there is no shared-write race between iterations.
     */
    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(static)
    for (i = 0; i < count; i++) {

        const char *extension_category;
        const char *size_category;
        int score;

        extension_category =
            categorize_extension(records[i].extension);

        size_category =
            categorize_size(records[i].size_bytes);

        score =
            calculate_score(
                &records[i],
                extension_category,
                size_category
            );

        copy_string_safe(
            results[i].path,
            records[i].path,
            sizeof(results[i].path)
        );

        copy_string_safe(
            results[i].extension_category,
            extension_category,
            sizeof(results[i].extension_category)
        );

        copy_string_safe(
            results[i].size_category,
            size_category,
            sizeof(results[i].size_category)
        );

        results[i].score = score;

        copy_string_safe(
            results[i].risk_label,
            calculate_risk_label(score),
            sizeof(results[i].risk_label)
        );
    }

    return 1;
}


/* ============================================================
   CSV Output
   ============================================================ */

static void write_csv_field(FILE *file,
                            const char *field)
{
    int needs_quotes = 0;

    const char *p = field;

    while (*p != '\0') {

        if (*p == ',' ||
            *p == '"' ||
            *p == '\n' ||
            *p == '\r') {

            needs_quotes = 1;
            break;
        }

        p++;
    }

    if (!needs_quotes) {

        fprintf(file, "%s", field);
        return;
    }

    fprintf(file, "\"");

    p = field;

    while (*p != '\0') {

        if (*p == '"') {
            fprintf(file, "\"\"");
        }
        else {
            fputc(*p, file);
        }

        p++;
    }

    fprintf(file, "\"");
}


int write_results_csv(const char *filename,
                      const AnalysisResult *results,
                      size_t count)
{
    FILE *file;
    size_t i;

    file = fopen(filename, "w");

    if (file == NULL) {
        perror("Error opening output CSV");
        return 0;
    }

    /*
     * Required M3 output schema.
     */
    fprintf(
        file,
        "path,extension_category,size_category,score,risk_label\n"
    );

    for (i = 0; i < count; i++) {

        write_csv_field(
            file,
            results[i].path
        );

        fprintf(file, ",");

        write_csv_field(
            file,
            results[i].extension_category
        );

        fprintf(file, ",");

        write_csv_field(
            file,
            results[i].size_category
        );

        fprintf(
            file,
            ",%d,",
            results[i].score
        );

        write_csv_field(
            file,
            results[i].risk_label
        );

        fprintf(file, "\n");
    }

    fclose(file);

    return 1;
}


/* ============================================================
   Memory Cleanup
   ============================================================ */

void free_metadata(MetadataRecord *records)
{
    free(records);
}


/* ============================================================
   Timing
   ============================================================ */

static double get_time_seconds(void)
{
    struct timespec ts;

    clock_gettime(
        CLOCK_MONOTONIC,
        &ts
    );

    return (double)ts.tv_sec +
           (double)ts.tv_nsec / 1e9;
}


/* ============================================================
   Main
   ============================================================ */

int main(int argc, char *argv[])
{
    const char *input_file;
    const char *output_file;

    int num_threads;

    MetadataRecord *records = NULL;
    AnalysisResult *results = NULL;

    size_t record_count = 0;

    double start_time;
    double end_time;
    double execution_time;

    /*
     * Expected:
     *
     * ./parallel_analyzer metadata.csv parallel.csv 4
     */
    if (argc != 4) {

        fprintf(
            stderr,
            "Usage: %s <metadata.csv> <output.csv> <threads>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    input_file = argv[1];
    output_file = argv[2];

    num_threads = atoi(argv[3]);

    if (num_threads <= 0) {

        fprintf(
            stderr,
            "Error: number of threads must be greater than 0.\n"
        );

        return EXIT_FAILURE;
    }

    /*
     * Read metadata.
     *
     * This happens before timing so that the benchmark focuses
     * on the computational analysis rather than CSV loading.
     */
    if (!read_metadata_csv(
            input_file,
            &records,
            &record_count)) {

        return EXIT_FAILURE;
    }

    if (record_count == 0) {

        fprintf(
            stderr,
            "Error: no metadata records found.\n"
        );

        free_metadata(records);

        return EXIT_FAILURE;
    }

    /*
     * Allocate result array.
     *
     * Each input record has exactly one corresponding result.
     */
    results = malloc(
        record_count * sizeof(AnalysisResult)
    );

    if (results == NULL) {

        fprintf(
            stderr,
            "Error: could not allocate result memory.\n"
        );

        free_metadata(records);

        return EXIT_FAILURE;
    }

    /*
     * Measure ONLY the parallel analysis.
     *
     * CSV reading and output writing are outside this timing
     * section to keep the comparison focused on the algorithm.
     */
    start_time = get_time_seconds();

    if (!analyze_parallel(
            records,
            results,
            record_count,
            num_threads)) {

        fprintf(
            stderr,
            "Parallel analysis failed.\n"
        );

        free(results);
        free_metadata(records);

        return EXIT_FAILURE;
    }

    end_time = get_time_seconds();

    execution_time =
        end_time - start_time;

    /*
     * Write output after timing.
     */
    if (!write_results_csv(
            output_file,
            results,
            record_count)) {

        free(results);
        free_metadata(records);

        return EXIT_FAILURE;
    }

    /*
     * Program summary.
     *
     * M4 can capture this execution time later.
     */
    printf("Parallel analysis completed successfully.\n");
    printf("Records processed : %zu\n", record_count);
    printf("Threads used      : %d\n", num_threads);
    printf("Execution time    : %.9f seconds\n",
           execution_time);

    free(results);
    free_metadata(records);

    return EXIT_SUCCESS;
}
