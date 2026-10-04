#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "../../src/m1_file_analysis/file_analyzer.h"

static int tests_run = 0;
static int tests_passed = 0;

#define CHECK(condition, message)                    \
    do {                                             \
        tests_run++;                                 \
        if (condition) {                             \
            tests_passed++;                         \
            printf("PASS: %s\n", message);           \
        } else {                                     \
            printf("FAIL: %s\n", message);           \
        }                                            \
    } while (0)


static void create_directory(const char *path)
{
    CreateDirectoryA(path, NULL);
}


static void create_test_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return;
    }

    fputs(content, file);
    fclose(file);
}


static int file_exists(const char *path)
{
    DWORD attributes = GetFileAttributesA(path);

    return attributes != INVALID_FILE_ATTRIBUTES;
}


static int read_csv_header(const char *csv,
                           char *header,
                           size_t header_size)
{
    FILE *file;
    size_t length;

    file = fopen(csv, "r");

    if (file == NULL) {
        return 0;
    }

    if (fgets(header, (int)header_size, file) == NULL) {
        fclose(file);
        return 0;
    }

    fclose(file);

    length = strlen(header);

    if (length > 0 && header[length - 1] == '\n') {
        header[length - 1] = '\0';
    }

    if (length > 1 && header[length - 2] == '\r') {
        header[length - 2] = '\0';
    }

    return 1;
}


static int csv_has_data_row(const char *csv)
{
    FILE *file;
    char buffer[4096];

    file = fopen(csv, "r");

    if (file == NULL) {
        return 0;
    }

    /* Skip header. */
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        return 0;
    }

    /* Check for at least one data row. */
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        return 0;
    }

    fclose(file);

    return 1;
}


int main(void)
{
    const char *test_directory =
        "tests\\m1_tests\\test_data";

    const char *nested_directory =
        "tests\\m1_tests\\test_data\\nested";

    const char *test_file =
        "tests\\m1_tests\\test_data\\sample.txt";

    const char *nested_file =
        "tests\\m1_tests\\test_data\\nested\\nested.txt";

    const char *output_csv =
        "tests\\m1_tests\\test_metadata.csv";

    const char *missing_directory =
        "tests\\m1_tests\\does_not_exist";

    char header[512];

    printf("Running M1 tests...\n\n");


    /*
     * ---------------------------------------------------------
     * Test setup
     * ---------------------------------------------------------
     */

    create_directory("tests\\m1_tests");
    create_directory(test_directory);
    create_directory(nested_directory);

    create_test_file(
        test_file,
        "M1 test file\n"
    );

    create_test_file(
        nested_file,
        "Nested M1 test file\n"
    );


    /*
     * ---------------------------------------------------------
     * Test 1: Metadata extraction
     * ---------------------------------------------------------
     */

    {
        FileMetadata metadata;

        int result = extract_file_metadata(
            test_file,
            "sample.txt",
            &metadata
        );

        CHECK(
            result == 1,
            "metadata extraction succeeds for an existing file"
        );

        CHECK(
            strcmp(metadata.path, "sample.txt") == 0,
            "path is extracted correctly"
        );

        CHECK(
            strcmp(metadata.filename, "sample.txt") == 0,
            "filename is extracted correctly"
        );

        CHECK(
            strcmp(metadata.extension, ".txt") == 0,
            "extension is extracted correctly"
        );

        CHECK(
            metadata.size_bytes > 0,
            "size_bytes is extracted correctly"
        );

        CHECK(
            strlen(metadata.modified_time) > 0,
            "modified_time is extracted"
        );

        CHECK(
            metadata.permissions == 0 ||
            metadata.permissions == 1,
            "permissions has a valid value"
        );

        CHECK(
            metadata.is_hidden == 0 ||
            metadata.is_hidden == 1,
            "is_hidden is 0 or 1"
        );
    }


    /*
     * ---------------------------------------------------------
     * Test 2: Recursive directory scanning
     * ---------------------------------------------------------
     */

    remove(output_csv);

    {
        int result = analyze_directory(
            test_directory,
            output_csv
        );

        CHECK(
            result >= 2,
            "directory scan finds files recursively"
        );

        CHECK(
            file_exists(output_csv),
            "metadata CSV is created"
        );
    }


    /*
     * ---------------------------------------------------------
     * Test 3: Required CSV header
     * ---------------------------------------------------------
     */

    memset(header, 0, sizeof(header));

    CHECK(
        read_csv_header(
            output_csv,
            header,
            sizeof(header)
        ),
        "CSV header can be read"
    );

    CHECK(
        strcmp(
            header,
            "path,filename,extension,size_bytes,modified_time,permissions,is_hidden"
        ) == 0,
        "CSV header matches the required M1 format"
    );


    /*
     * ---------------------------------------------------------
     * Test 4: CSV contains metadata records
     * ---------------------------------------------------------
     */

    CHECK(
        csv_has_data_row(output_csv),
        "CSV contains at least one metadata record"
    );


    /*
     * ---------------------------------------------------------
     * Test 5: Missing directory error handling
     * ---------------------------------------------------------
     */

    {
        int result = analyze_directory(
            missing_directory,
            output_csv
        );

        CHECK(
            result == 0,
            "missing input directory is handled without crashing"
        );
    }


    /*
     * ---------------------------------------------------------
     * Test 6: Missing file error handling
     * ---------------------------------------------------------
     */

    {
        FileMetadata metadata;

        int result = extract_file_metadata(
            "tests\\m1_tests\\test_data\\missing.txt",
            "missing.txt",
            &metadata
        );

        CHECK(
            result == 0,
            "missing file is handled without crashing"
        );
    }


    /*
     * ---------------------------------------------------------
     * Test results
     * ---------------------------------------------------------
     */

    printf("\n");
    printf(
        "M1 tests: %d/%d passed.\n",
        tests_passed,
        tests_run
    );


    /*
     * ---------------------------------------------------------
     * Cleanup
     * ---------------------------------------------------------
     */

    remove(test_file);
    remove(nested_file);
    remove(output_csv);

    RemoveDirectoryA(nested_directory);
    RemoveDirectoryA(test_directory);

    /*
     * Return 0 only when every test passes.
     */
    if (tests_passed == tests_run) {
        return 0;
    }

    return 1;
}