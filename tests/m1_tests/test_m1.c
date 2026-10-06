#include "../../src/m1_file_analysis/file_analyzer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define TEST_DATA_DIR "tests/m1_tests/test_data"
#define TEST_FILE "tests/m1_tests/test_data/sample.txt"
#define TEST_HIDDEN_FILE "tests/m1_tests/test_data/.hidden"
#define TEST_OUTPUT "tests/m1_tests/test_data/metadata.csv"

static int tests_passed = 0;
static int tests_failed = 0;

/* ------------------------------------------------------------
 * Test helper
 * ------------------------------------------------------------ */
static void check_test(
    const char *test_name,
    int condition
)
{
    if (condition) {
        printf("[PASS] %s\n", test_name);
        tests_passed++;
    } else {
        printf("[FAIL] %s\n", test_name);
        tests_failed++;
    }
}

/* ------------------------------------------------------------
 * Create test directory
 * ------------------------------------------------------------ */
static int create_test_directory(void)
{
    struct stat directory_info;

    if (stat(TEST_DATA_DIR, &directory_info) == 0) {
        return S_ISDIR(directory_info.st_mode);
    }

    return 0;
}

/* ------------------------------------------------------------
 * Create normal test file
 * ------------------------------------------------------------ */
static int create_test_file(void)
{
    FILE *file;

    file = fopen(TEST_FILE, "w");

    if (file == NULL) {
        perror("fopen");
        return 0;
    }

    /*
     * 19 bytes:
     * "Digital Forensics\n"
     */
    fputs("Digital Forensics\n", file);

    fclose(file);

    /*
     * Owner read/write, group read, others read.
     */
    if (chmod(TEST_FILE, 0644) != 0) {
        perror("chmod");
        return 0;
    }

    return 1;
}

/* ------------------------------------------------------------
 * Create hidden test file
 * ------------------------------------------------------------ */
static int create_hidden_file(void)
{
    FILE *file;

    file = fopen(TEST_HIDDEN_FILE, "w");

    if (file == NULL) {
        perror("fopen hidden file");
        return 0;
    }

    fputs("Hidden file\n", file);

    fclose(file);

    if (chmod(TEST_HIDDEN_FILE, 0644) != 0) {
        perror("chmod hidden file");
        return 0;
    }

    return 1;
}

/* ------------------------------------------------------------
 * Test metadata extraction
 * ------------------------------------------------------------ */
static void test_metadata_extraction(void)
{
    FileMetadata metadata;
    int result;

    memset(&metadata, 0, sizeof(metadata));

    result = extract_file_metadata(
        TEST_FILE,
        "sample.txt",
        &metadata
    );

    check_test(
        "Metadata extraction succeeds",
        result == 1
    );

    check_test(
        "Path extracted correctly",
        strcmp(metadata.path, "sample.txt") == 0
    );

    check_test(
        "Filename extracted correctly",
        strcmp(metadata.filename, "sample.txt") == 0
    );

    check_test(
        "Extension extracted correctly",
        strcmp(metadata.extension, ".txt") == 0
    );

    check_test(
        "File size extracted correctly",
        metadata.size_bytes == 18
    );

    check_test(
        "Modified time extracted",
        strlen(metadata.modified_time) > 0
    );

    check_test(
        "Permissions extracted correctly",
        metadata.permissions == 1
    );

    check_test(
        "Normal file detected as not hidden",
        metadata.is_hidden == 0
    );
}

/* ------------------------------------------------------------
 * Test hidden file detection
 * ------------------------------------------------------------ */
static void test_hidden_file(void)
{
    FileMetadata metadata;
    int result;

    memset(&metadata, 0, sizeof(metadata));

    result = extract_file_metadata(
        TEST_HIDDEN_FILE,
        ".hidden",
        &metadata
    );

    check_test(
        "Hidden file metadata extraction succeeds",
        result == 1
    );

    check_test(
        "Hidden file detected correctly",
        metadata.is_hidden == 1
    );
}

/* ------------------------------------------------------------
 * Test recursive directory analysis
 * ------------------------------------------------------------ */
static void test_directory_analysis(void)
{
    int result;

    result = analyze_directory(
        TEST_DATA_DIR,
        TEST_OUTPUT
    );

    check_test(
        "Directory analysis succeeds",
        result >= 2
    );
}

/* ------------------------------------------------------------
 * Test CSV output
 * ------------------------------------------------------------ */
static void test_csv_output(void)
{
    FILE *file;
    char line[4096];
    int header_found = 0;
    int data_rows = 0;

    file = fopen(TEST_OUTPUT, "r");

    if (file == NULL) {
        check_test(
            "CSV output file exists",
            0
        );
        return;
    }

    /*
     * Read first line and verify exact header.
     */
    if (fgets(line, sizeof(line), file) != NULL) {

        if (strcmp(
                line,
                "path,filename,extension,size_bytes,modified_time,permissions,is_hidden\n"
            ) == 0) {

            header_found = 1;
        }
    }

    check_test(
        "CSV header is correct",
        header_found
    );

    /*
     * Count data rows.
     */
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strlen(line) > 1) {
            data_rows++;
        }
    }

    fclose(file);

    check_test(
        "CSV contains data rows",
        data_rows >= 2
    );
}

/* ------------------------------------------------------------
 * Test missing directory
 * ------------------------------------------------------------ */
static void test_missing_directory(void)
{
    int result;

    result = analyze_directory(
        "tests/m1_tests/does_not_exist",
        "tests/m1_tests/missing_output.csv"
    );

    check_test(
        "Missing directory is handled correctly",
        result == -1
    );
}

/* ------------------------------------------------------------
 * Test missing file
 * ------------------------------------------------------------ */
static void test_missing_file(void)
{
    FileMetadata metadata;
    int result;

    memset(&metadata, 0, sizeof(metadata));

    result = extract_file_metadata(
        "tests/m1_tests/does_not_exist.txt",
        "does_not_exist.txt",
        &metadata
    );

    check_test(
        "Missing file is handled correctly",
        result == 0
    );
}

/* ------------------------------------------------------------
 * Cleanup test files
 * ------------------------------------------------------------ */
static void cleanup_test_files(void)
{
    remove(TEST_OUTPUT);
    remove(TEST_FILE);
    remove(TEST_HIDDEN_FILE);
    rmdir(TEST_DATA_DIR);
}

/* ------------------------------------------------------------
 * Main test program
 * ------------------------------------------------------------ */
int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("       M1 FILE ANALYSIS TESTS\n");
    printf("========================================\n\n");

    /*
     * Prepare test data.
     */
    check_test(
        "Test directory created",
        create_test_directory()
    );

    check_test(
        "Normal test file created",
        create_test_file()
    );

    check_test(
        "Hidden test file created",
        create_hidden_file()
    );

    /*
     * Metadata tests.
     */
    test_metadata_extraction();

    /*
     * Hidden-file test.
     */
    test_hidden_file();

    /*
     * Directory and CSV tests.
     */
    test_directory_analysis();
    test_csv_output();

    /*
     * Error-handling tests.
     */
    test_missing_directory();
    test_missing_file();

    /*
     * Cleanup.
     */
    cleanup_test_files();

    printf("\n");
    printf("========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    if (tests_failed == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }

    printf("\nSOME TESTS FAILED\n");
    return 1;
}