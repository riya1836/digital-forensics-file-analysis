#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECORDS 20000
#define MAX_LINE_LENGTH 16384
#define MAX_FIELD_LENGTH 512
#define CSV_FIELD_COUNT 5

typedef struct
{
    char path[MAX_FIELD_LENGTH];
    char extension_category[MAX_FIELD_LENGTH];
    char size_category[MAX_FIELD_LENGTH];
    char score[MAX_FIELD_LENGTH];
    char risk_label[MAX_FIELD_LENGTH];

} FileRecord;


/* ---------------------------------------------------------
 * Remove newline characters
 * --------------------------------------------------------- */

void remove_newline(char *str)
{
    if (str != NULL)
    {
        size_t len = strlen(str);

        while (len > 0 &&
               (str[len - 1] == '\r' ||
                str[len - 1] == '\n'))
        {
            str[len - 1] = '\0';
            len--;
        }
    }
}


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
    char fields[CSV_FIELD_COUNT][MAX_FIELD_LENGTH]
)
{
    int field = 0;
    size_t pos = 0;

    if (line == NULL || fields == NULL)
        return 0;

    for (int i = 0; i < CSV_FIELD_COUNT; i++)
    {
        fields[i][0] = '\0';
    }

    while (field < CSV_FIELD_COUNT)
    {
        size_t out = 0;
        int quoted = 0;

        /*
         * Empty field.
         *
         * Example:
         * a,,c,d,e
         */
        if (line[pos] == ',')
        {
            fields[field][0] = '\0';

            field++;
            pos++;

            continue;
        }

        /*
         * Quoted field.
         */
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
                /*
                 * Escaped quote: ""
                 */
                if (c == '"')
                {
                    if (line[pos + 1] == '"')
                    {
                        if (out < MAX_FIELD_LENGTH - 1)
                        {
                            fields[field][out++] = '"';
                        }

                        pos += 2;
                        continue;
                    }

                    /*
                     * Closing quote.
                     */
                    quoted = 0;
                    pos++;

                    continue;
                }

                if (out < MAX_FIELD_LENGTH - 1)
                {
                    fields[field][out++] = c;
                }

                pos++;
            }
            else
            {
                /*
                 * Comma ends the field.
                 */
                if (c == ',')
                {
                    break;
                }

                /*
                 * End of line.
                 */
                if (c == '\r' || c == '\n')
                {
                    break;
                }

                if (out < MAX_FIELD_LENGTH - 1)
                {
                    fields[field][out++] = c;
                }

                pos++;
            }
        }

        /*
         * If the quoted field never closed,
         * the CSV record is invalid.
         */
        if (quoted)
        {
            return 0;
        }

        fields[field][out] = '\0';

        field++;

        /*
         * Move past comma.
         */
        if (line[pos] == ',')
        {
            pos++;
            continue;
        }

        /*
         * End of line.
         */
        if (line[pos] == '\0' ||
            line[pos] == '\r' ||
            line[pos] == '\n')
        {
            break;
        }
    }

    /*
     * Exactly five fields are required.
     */
    return field == CSV_FIELD_COUNT;
}


/* ---------------------------------------------------------
 * Read records from CSV
 *
 * Expected format:
 * path,extension_category,size_category,score,risk_label
 * --------------------------------------------------------- */

int read_csv(
    const char *filename,
    FileRecord records[]
)
{
    FILE *file;
    char line[MAX_LINE_LENGTH];

    int count = 0;

    file = fopen(filename, "r");

    if (file == NULL)
    {
        printf(
            "Error: Could not open %s\n",
            filename
        );

        return -1;
    }

    /*
     * Read and skip header.
     */
    if (fgets(
            line,
            sizeof(line),
            file) == NULL)
    {
        fclose(file);
        return 0;
    }

    /*
     * Read data records.
     */
    while (fgets(
               line,
               sizeof(line),
               file) != NULL)
    {
        char fields[CSV_FIELD_COUNT][MAX_FIELD_LENGTH];

        /*
         * Remove CR/LF.
         */
        remove_newline(line);

        /*
         * Ignore empty lines.
         */
        if (strlen(line) == 0)
        {
            continue;
        }

        /*
         * Protect against too many records.
         */
        if (count >= MAX_RECORDS)
        {
            printf(
                "Error: Maximum number of records exceeded.\n"
            );

            fclose(file);

            return -1;
        }

        /*
         * Parse CSV row.
         */
        if (!parse_csv_line(
                line,
                fields))
        {
            printf(
                "Warning: Invalid CSV record in %s:\n%s\n",
                filename,
                line
            );

            continue;
        }

        /*
         * Copy parsed fields into record.
         */
        snprintf(
            records[count].path,
            sizeof(records[count].path),
            "%s",
            fields[0]
        );

        snprintf(
            records[count].extension_category,
            sizeof(records[count].extension_category),
            "%s",
            fields[1]
        );

        snprintf(
            records[count].size_category,
            sizeof(records[count].size_category),
            "%s",
            fields[2]
        );

        snprintf(
            records[count].score,
            sizeof(records[count].score),
            "%s",
            fields[3]
        );

        snprintf(
            records[count].risk_label,
            sizeof(records[count].risk_label),
            "%s",
            fields[4]
        );

        count++;
    }

    fclose(file);

    return count;
}


/* ---------------------------------------------------------
 * Find record by path
 * --------------------------------------------------------- */

int find_record_by_path(
    FileRecord records[],
    int count,
    const char *path
)
{
    if (path == NULL)
    {
        return -1;
    }

    for (int i = 0; i < count; i++)
    {
        if (strcmp(
                records[i].path,
                path) == 0)
        {
            return i;
        }
    }

    return -1;
}


/* ---------------------------------------------------------
 * Compare classification-related fields
 * --------------------------------------------------------- */

int records_match(
    const FileRecord *sequential,
    const FileRecord *parallel
)
{
    if (sequential == NULL ||
        parallel == NULL)
    {
        return 0;
    }

    if (strcmp(
            sequential->extension_category,
            parallel->extension_category) != 0)
    {
        return 0;
    }

    if (strcmp(
            sequential->size_category,
            parallel->size_category) != 0)
    {
        return 0;
    }

    if (strcmp(
            sequential->score,
            parallel->score) != 0)
    {
        return 0;
    }

    if (strcmp(
            sequential->risk_label,
            parallel->risk_label) != 0)
    {
        return 0;
    }

    return 1;
}


/* ---------------------------------------------------------
 * Main
 * --------------------------------------------------------- */

int main(void)
{
    /*
     * static prevents these large arrays from being
     * allocated on the limited stack.
     */
    static FileRecord sequential_records[MAX_RECORDS];
    static FileRecord parallel_records[MAX_RECORDS];

    int sequential_count;
    int parallel_count;

    printf(
        "========================================\n"
    );

    printf(
        " Sequential vs Parallel Comparison\n"
    );

    printf(
        "========================================\n\n"
    );

    /*
     * Read sequential results.
     */
    sequential_count = read_csv(
        "outputs/sequential.csv",
        sequential_records
    );

    if (sequential_count < 0)
    {
        return 1;
    }

    /*
     * Read parallel results.
     */
    parallel_count = read_csv(
        "outputs/parallel.csv",
        parallel_records
    );

    if (parallel_count < 0)
    {
        return 1;
    }

    printf(
        "Sequential records : %d\n",
        sequential_count
    );

    printf(
        "Parallel records   : %d\n\n",
        parallel_count
    );

    /*
     * If either file has no records, comparison
     * cannot be performed yet.
     */
    if (sequential_count == 0 ||
        parallel_count == 0)
    {
        printf(
            "WARNING: One or both output files contain no records.\n"
        );

        printf(
            "Comparison cannot be performed yet.\n"
        );

        printf(
            "Run M2 and M3 first to generate their output CSV files.\n"
        );

        return 0;
    }

    int matching_records = 0;
    int mismatched_records = 0;
    int missing_in_parallel = 0;
    int missing_in_sequential = 0;

    /*
     * Compare every sequential record with its
     * corresponding parallel record.
     */
    for (int i = 0;
         i < sequential_count;
         i++)
    {
        int parallel_index =
            find_record_by_path(
                parallel_records,
                parallel_count,
                sequential_records[i].path
            );

        if (parallel_index == -1)
        {
            printf(
                "Missing in parallel results: %s\n",
                sequential_records[i].path
            );

            missing_in_parallel++;

            continue;
        }

        if (records_match(
                &sequential_records[i],
                &parallel_records[parallel_index]))
        {
            matching_records++;
        }
        else
        {
            mismatched_records++;

            printf(
                "Mismatch: %s\n",
                sequential_records[i].path
            );
        }
    }

    /*
     * Check for records that exist in parallel
     * but not in sequential results.
     */
    for (int i = 0;
         i < parallel_count;
         i++)
    {
        int sequential_index =
            find_record_by_path(
                sequential_records,
                sequential_count,
                parallel_records[i].path
            );

        if (sequential_index == -1)
        {
            printf(
                "Missing in sequential results: %s\n",
                parallel_records[i].path
            );

            missing_in_sequential++;
        }
    }

    /*
     * Display comparison summary.
     */
    printf(
        "\n========================================\n"
    );

    printf(
        " Comparison Summary\n"
    );

    printf(
        "========================================\n"
    );

    printf(
        "Sequential records    : %d\n",
        sequential_count
    );

    printf(
        "Parallel records      : %d\n",
        parallel_count
    );

    printf(
        "Matching records      : %d\n",
        matching_records
    );

    printf(
        "Mismatched records    : %d\n",
        mismatched_records
    );

    printf(
        "Missing in parallel   : %d\n",
        missing_in_parallel
    );

    printf(
        "Missing in sequential : %d\n",
        missing_in_sequential
    );

    printf("\n");

    /*
     * Final correctness result.
     */
    if (mismatched_records == 0 &&
        missing_in_parallel == 0 &&
        missing_in_sequential == 0 &&
        sequential_count == parallel_count)
    {
        printf("RESULT: PASS\n");

        printf(
            "Sequential and parallel results are logically equivalent.\n"
        );

        return 0;
    }

    printf("RESULT: FAIL\n");

    printf(
        "Sequential and parallel results are not equivalent.\n"
    );

    return 1;
}