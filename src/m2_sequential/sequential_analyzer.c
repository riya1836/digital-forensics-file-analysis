#include "sequential_analyzer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE 16384
#define CSV_FIELD_COUNT 7

/* ---------------------------------------------------------
 * Robust CSV parser
 *
 * Supports:
 * - empty fields
 * - quoted fields
 * - commas inside quoted fields
 * - escaped quotes ("")
 * --------------------------------------------------------- */

static int parse_csv_line(
    const char *line,
    char fields[CSV_FIELD_COUNT][LINE_SIZE]
)
{
    int field = 0;
    size_t pos = 0;

    if (line == NULL || fields == NULL)
        return 0;

    for (int i = 0; i < CSV_FIELD_COUNT; i++)
        fields[i][0] = '\0';

    while (field < CSV_FIELD_COUNT)
    {
        size_t out = 0;
        int quoted = 0;

        if (line[pos] == ',')
        {
            fields[field][0] = '\0';
            field++;
            pos++;
            continue;
        }

        if (line[pos] == '"')
        {
            quoted = 1;
            pos++;
        }

        while (line[pos] != '\0')
        {
            char c = line[pos];

            if (quoted)
            {
                if (c == '"')
                {
                    if (line[pos + 1] == '"')
                    {
                        if (out < LINE_SIZE - 1)
                            fields[field][out++] = '"';

                        pos += 2;
                        continue;
                    }

                    quoted = 0;
                    pos++;
                    continue;
                }

                if (out < LINE_SIZE - 1)
                    fields[field][out++] = c;

                pos++;
            }
            else
            {
                if (c == ',')
                    break;

                if (c == '\r' || c == '\n')
                    break;

                if (out < LINE_SIZE - 1)
                    fields[field][out++] = c;

                pos++;
            }
        }

        fields[field][out] = '\0';
        field++;

        if (line[pos] == ',')
        {
            pos++;
            continue;
        }

        if (line[pos] == '\0' ||
            line[pos] == '\r' ||
            line[pos] == '\n')
        {
            break;
        }
    }

    return field == CSV_FIELD_COUNT;
}

/* ---------------------------------------------------------
 * CSV output escaping
 *
 * Fields containing comma, quote, CR or LF are wrapped
 * in double quotes. Existing quotes are doubled.
 * --------------------------------------------------------- */

static void write_csv_field(
    FILE *output,
    const char *field
)
{
    int needs_quotes = 0;

    if (field == NULL)
        field = "";

    for (size_t i = 0; field[i] != '\0'; i++)
    {
        if (field[i] == ',' ||
            field[i] == '"' ||
            field[i] == '\r' ||
            field[i] == '\n')
        {
            needs_quotes = 1;
            break;
        }
    }

    if (!needs_quotes)
    {
        fputs(field, output);
        return;
    }

    fputc('"', output);

    for (size_t i = 0; field[i] != '\0'; i++)
    {
        if (field[i] == '"')
            fputc('"', output);

        fputc(field[i], output);
    }

    fputc('"', output);
}

/* ---------------------------------------------------------
 * Permission weight
 * --------------------------------------------------------- */

static int get_permission_weight(const char *permissions)
{
    long permission_value;

    if (permissions == NULL || permissions[0] == '\0')
        return 0;

    permission_value = strtol(
        permissions,
        NULL,
        8
    );

    if ((permission_value & 0200L) != 0)
        return 1;

    if (strchr(permissions, 'w') != NULL)
        return 1;

    return 0;
}

/* ---------------------------------------------------------
 * Extension classification
 * --------------------------------------------------------- */

static const char *classify_extension(const char *extension)
{
    if (extension == NULL || extension[0] == '\0')
        return "OTHER";

    if (strcmp(extension, ".txt") == 0 ||
        strcmp(extension, ".pdf") == 0 ||
        strcmp(extension, ".doc") == 0 ||
        strcmp(extension, ".docx") == 0 ||
        strcmp(extension, ".xls") == 0 ||
        strcmp(extension, ".xlsx") == 0 ||
        strcmp(extension, ".ppt") == 0 ||
        strcmp(extension, ".pptx") == 0)
    {
        return "DOCUMENT";
    }

    if (strcmp(extension, ".jpg") == 0 ||
        strcmp(extension, ".jpeg") == 0 ||
        strcmp(extension, ".png") == 0 ||
        strcmp(extension, ".gif") == 0 ||
        strcmp(extension, ".bmp") == 0)
    {
        return "IMAGE";
    }

    if (strcmp(extension, ".mp3") == 0 ||
        strcmp(extension, ".wav") == 0 ||
        strcmp(extension, ".flac") == 0)
    {
        return "AUDIO";
    }

    if (strcmp(extension, ".mp4") == 0 ||
        strcmp(extension, ".avi") == 0 ||
        strcmp(extension, ".mkv") == 0 ||
        strcmp(extension, ".mov") == 0)
    {
        return "VIDEO";
    }

    if (strcmp(extension, ".zip") == 0 ||
        strcmp(extension, ".rar") == 0 ||
        strcmp(extension, ".7z") == 0 ||
        strcmp(extension, ".tar") == 0 ||
        strcmp(extension, ".gz") == 0)
    {
        return "ARCHIVE";
    }

    if (strcmp(extension, ".c") == 0 ||
        strcmp(extension, ".h") == 0 ||
        strcmp(extension, ".cpp") == 0 ||
        strcmp(extension, ".java") == 0 ||
        strcmp(extension, ".py") == 0 ||
        strcmp(extension, ".js") == 0)
    {
        return "CODE";
    }

    return "OTHER";
}

/* ---------------------------------------------------------
 * Size classification
 * --------------------------------------------------------- */

static const char *classify_size(long long size)
{
    if (size < 1024LL * 1024LL)
        return "SMALL";

    if (size < 100LL * 1024LL * 1024LL)
        return "MEDIUM";

    return "LARGE";
}

/* ---------------------------------------------------------
 * Extension weight
 * --------------------------------------------------------- */

static int get_extension_weight(const char *category)
{
    if (strcmp(category, "DOCUMENT") == 0)
        return 1;

    if (strcmp(category, "IMAGE") == 0)
        return 2;

    if (strcmp(category, "AUDIO") == 0)
        return 2;

    if (strcmp(category, "VIDEO") == 0)
        return 3;

    if (strcmp(category, "ARCHIVE") == 0)
        return 3;

    if (strcmp(category, "CODE") == 0)
        return 2;

    return 1;
}

/* ---------------------------------------------------------
 * Size weight
 * --------------------------------------------------------- */

static int get_size_weight(const char *size_category)
{
    if (strcmp(size_category, "SMALL") == 0)
        return 1;

    if (strcmp(size_category, "MEDIUM") == 0)
        return 2;

    return 3;
}

/* ---------------------------------------------------------
 * Analyze one metadata record
 * --------------------------------------------------------- */

void analyze_record(
    const MetadataRecord *record,
    AnalysisResult *result
)
{
    const char *extension_category;
    const char *size_category;

    int score = 0;

    if (record == NULL || result == NULL)
        return;

    extension_category =
        classify_extension(record->extension);

    size_category =
        classify_size(record->size_bytes);

    score += get_extension_weight(
        extension_category
    );

    score += get_size_weight(
        size_category
    );

    if (record->is_hidden)
        score += 2;

    score += get_permission_weight(
        record->permissions
    );

    snprintf(
        result->extension_category,
        sizeof(result->extension_category),
        "%s",
        extension_category
    );

    snprintf(
        result->size_category,
        sizeof(result->size_category),
        "%s",
        size_category
    );

    result->score = score;

    if (score <= 3)
    {
        snprintf(
            result->risk_label,
            sizeof(result->risk_label),
            "LOW"
        );
    }
    else if (score <= 6)
    {
        snprintf(
            result->risk_label,
            sizeof(result->risk_label),
            "MEDIUM"
        );
    }
    else
    {
        snprintf(
            result->risk_label,
            sizeof(result->risk_label),
            "HIGH"
        );
    }
}

/* ---------------------------------------------------------
 * Process metadata CSV sequentially
 * --------------------------------------------------------- */

int process_metadata(
    const char *input_csv,
    const char *output_csv,
    double *execution_time_seconds
)
{
    FILE *input;
    FILE *output;

    char line[LINE_SIZE];

    MetadataRecord *records = NULL;
    AnalysisResult *results = NULL;

    size_t record_count = 0;
    size_t capacity = 1024;

    int line_number = 0;

    clock_t start_time;
    clock_t end_time;

    if (input_csv == NULL ||
        output_csv == NULL)
    {
        return 0;
    }

    input = fopen(input_csv, "r");

    if (input == NULL)
    {
        perror("Error opening input CSV");
        return 0;
    }

    records = malloc(
        capacity * sizeof(MetadataRecord)
    );

    if (records == NULL)
    {
        fclose(input);
        return 0;
    }

    /*
     * Skip header.
     */
    if (fgets(line, sizeof(line), input) == NULL)
    {
        fclose(input);
        free(records);
        return 0;
    }

    line_number++;

    start_time = clock();

    while (fgets(line, sizeof(line), input) != NULL)
    {
        char fields[CSV_FIELD_COUNT][LINE_SIZE];

        line_number++;

        if (!parse_csv_line(line, fields))
        {
            fprintf(
                stderr,
                "Warning: invalid record at line %d\n",
                line_number
            );

            continue;
        }

        if (record_count >= capacity)
        {
            size_t new_capacity = capacity * 2;

            MetadataRecord *temp =
                realloc(
                    records,
                    new_capacity *
                    sizeof(MetadataRecord)
                );

            if (temp == NULL)
            {
                fclose(input);
                free(records);
                return 0;
            }

            records = temp;
            capacity = new_capacity;
        }

        snprintf(
            records[record_count].path,
            sizeof(records[record_count].path),
            "%s",
            fields[0]
        );

        snprintf(
            records[record_count].filename,
            sizeof(records[record_count].filename),
            "%s",
            fields[1]
        );

        snprintf(
            records[record_count].extension,
            sizeof(records[record_count].extension),
            "%s",
            fields[2]
        );

        records[record_count].size_bytes =
            atoll(fields[3]);

        snprintf(
            records[record_count].modified_time,
            sizeof(records[record_count].modified_time),
            "%s",
            fields[4]
        );

        snprintf(
            records[record_count].permissions,
            sizeof(records[record_count].permissions),
            "%s",
            fields[5]
        );

        records[record_count].is_hidden =
            atoi(fields[6]);

        record_count++;
    }

    fclose(input);

    results = malloc(
        record_count * sizeof(AnalysisResult)
    );

    if (results == NULL)
    {
        free(records);
        return 0;
    }

    /*
     * Sequential analysis.
     */
    for (size_t i = 0; i < record_count; i++)
    {
        analyze_record(
            &records[i],
            &results[i]
        );
    }

    end_time = clock();

    if (execution_time_seconds != NULL)
    {
        *execution_time_seconds =
            (double)(end_time - start_time) /
            CLOCKS_PER_SEC;
    }

    output = fopen(output_csv, "w");

    if (output == NULL)
    {
        perror("Error opening output CSV");

        free(records);
        free(results);

        return 0;
    }

    fprintf(
        output,
        "path,extension_category,size_category,score,risk_label\n"
    );

    /*
     * Write results.
     *
     * The path is escaped as a proper CSV field.
     */
    for (size_t i = 0; i < record_count; i++)
    {
        write_csv_field(
            output,
            records[i].path
        );

        fprintf(
            output,
            ",%s,%s,%d,%s\n",
            results[i].extension_category,
            results[i].size_category,
            results[i].score,
            results[i].risk_label
        );
    }

    fclose(output);

    free(records);
    free(results);

    return 1;
}

/* ---------------------------------------------------------
 * Main
 * --------------------------------------------------------- */

#ifndef M2_NO_MAIN

int main(int argc, char *argv[])
{
    double execution_time = 0.0;

    if (argc != 3)
    {
        fprintf(
            stderr,
            "Usage: %s <metadata.csv> <output.csv>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    if (!process_metadata(
            argv[1],
            argv[2],
            &execution_time))
    {
        fprintf(
            stderr,
            "Sequential analysis failed.\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Sequential analysis completed successfully.\n"
    );

    printf(
        "Input : %s\n",
        argv[1]
    );

    printf(
        "Output: %s\n",
        argv[2]
    );

    printf(
        "Execution time: %.6f seconds\n",
        execution_time
    );

    return EXIT_SUCCESS;
}

#endif