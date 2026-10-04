#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECORDS 20000
#define MAX_LINE_LENGTH 2048
#define MAX_FIELD_LENGTH 512

typedef struct
{
    char path[MAX_FIELD_LENGTH];
    char extension_category[MAX_FIELD_LENGTH];
    char size_category[MAX_FIELD_LENGTH];
    char score[MAX_FIELD_LENGTH];
    char risk_label[MAX_FIELD_LENGTH];

} FileRecord;


/* Remove newline characters from a string */
void remove_newline(char *str)
{
    if (str != NULL)
    {
        str[strcspn(str, "\r\n")] = '\0';
    }
}


/*
 * Read records from a CSV file.
 *
 * Expected format:
 * path,extension_category,size_category,score,risk_label
 */
int read_csv(const char *filename, FileRecord records[])
{
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int count = 0;

    file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("Error: Could not open %s\n", filename);
        return -1;
    }

    /* Read and skip header */
    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return 0;
    }

    /* Read data records */
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (count >= MAX_RECORDS)
        {
            printf("Error: Maximum number of records exceeded.\n");
            break;
        }

        remove_newline(line);

        /* Ignore empty lines */
        if (strlen(line) == 0)
        {
            continue;
        }

        int fields = sscanf(
            line,
            "%511[^,],%511[^,],%511[^,],%511[^,],%511[^\n]",
            records[count].path,
            records[count].extension_category,
            records[count].size_category,
            records[count].score,
            records[count].risk_label
        );

        if (fields == 5)
        {
            count++;
        }
    }

    fclose(file);

    return count;
}


/*
 * Find a record using its path.
 *
 * This allows comparison even if the row order
 * differs between sequential and parallel files.
 */
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
        if (strcmp(records[i].path, path) == 0)
        {
            return i;
        }
    }

    return -1;
}


/*
 * Compare classification-related fields.
 */
int records_match(
    const FileRecord *sequential,
    const FileRecord *parallel
)
{
    if (sequential == NULL || parallel == NULL)
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


int main(void)
{
    /*
     * static prevents these large arrays from being
     * allocated on the limited stack.
     */
    static FileRecord sequential_records[MAX_RECORDS];
    static FileRecord parallel_records[MAX_RECORDS];

    printf("========================================\n");
    printf(" Sequential vs Parallel Comparison\n");
    printf("========================================\n\n");

    /*
     * Read sequential results.
     */
    int sequential_count = read_csv(
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
    int parallel_count = read_csv(
        "outputs/parallel.csv",
        parallel_records
    );

    if (parallel_count < 0)
    {
        return 1;
    }

    printf("Sequential records : %d\n", sequential_count);
    printf("Parallel records   : %d\n\n", parallel_count);

    /*
     * If either file has no records, comparison
     * cannot be performed yet.
     */
    if (sequential_count == 0 || parallel_count == 0)
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
    for (int i = 0; i < sequential_count; i++)
    {
        int parallel_index = find_record_by_path(
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
    for (int i = 0; i < parallel_count; i++)
    {
        int sequential_index = find_record_by_path(
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
    printf("\n========================================\n");
    printf(" Comparison Summary\n");
    printf("========================================\n");

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