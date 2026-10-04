#include "../../src/m2_sequential/sequential_analyzer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_INPUT "tests/m2_tests/test_metadata.csv"
#define TEST_OUTPUT "tests/m2_tests/test_sequential.csv"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(condition, message)                    \
    do {                                            \
        tests_run++;                                \
        if (condition) {                            \
            tests_passed++;                        \
            printf("[PASS] %s\n", message);        \
        } else {                                    \
            printf("[FAIL] %s\n", message);        \
        }                                           \
    } while (0)

/* ---------------------------------------------------------
   Create test input
   --------------------------------------------------------- */

static void create_test_csv(void)
{
    FILE *file;

    file = fopen(TEST_INPUT, "w");

    if (file == NULL) {
        perror("Cannot create test input");
        exit(EXIT_FAILURE);
    }

    fprintf(file,
        "path,filename,extension,size_bytes,modified_time,permissions,is_hidden\n"

        "document.txt,document.txt,.txt,1000,"
        "2026-10-01T10:00:00,644,0\n"

        "photo.jpg,photo.jpg,.jpg,2097152,"
        "2026-10-01T10:00:00,644,0\n"

        "sound.mp3,sound.mp3,.mp3,1000,"
        "2026-10-01T10:00:00,644,0\n"

        "movie.mp4,movie.mp4,.mp4,104857600,"
        "2026-10-01T10:00:00,600,0\n"

        "archive.zip,archive.zip,.zip,1000,"
        "2026-10-01T10:00:00,644,0\n"

        "program.c,program.c,.c,1000,"
        "2026-10-01T10:00:00,664,0\n"

        "unknown.xyz,unknown.xyz,.xyz,1000,"
        "2026-10-01T10:00:00,644,0\n"

        "hidden.txt,hidden.txt,.txt,1000,"
        "2026-10-01T10:00:00,644,1\n"

        "medium.bin,medium.bin,.bin,1048576,"
        "2026-10-01T10:00:00,600,0\n"

        "small.bin,small.bin,.bin,1048575,"
        "2026-10-01T10:00:00,600,0\n"

        "video.mkv,video.mkv,.mkv,1048576,"
        "2026-10-01T10:00:00,600,1\n"

        "code.py,code.py,.py,1000,"
        "2026-10-01T10:00:00,600,1\n"

        "archive.rar,archive.rar,.rar,104857600,"
        "2026-10-01T10:00:00,664,1\n"
    );

    fclose(file);
}

/* ---------------------------------------------------------
   Test output
   --------------------------------------------------------- */

static void test_output(void)
{
    FILE *file;

    char line[1024];

    int line_number = 0;

    file = fopen(TEST_OUTPUT, "r");

    TEST(file != NULL,
         "Output file was created");

    if (file == NULL) {
        return;
    }

    /* Header */

    if (fgets(line, sizeof(line), file) != NULL) {

        TEST(
            strcmp(
                line,
                "path,extension_category,size_category,score,risk_label\n"
            ) == 0,
            "Output header is correct"
        );
    }

    while (fgets(line, sizeof(line), file) != NULL) {

        line_number++;

        switch (line_number) {

        case 1:
            TEST(
                strcmp(
                    line,
                    "document.txt,DOCUMENT,SMALL,2,LOW\n"
                ) == 0,
                "DOCUMENT classification"
            );
            break;

        case 2:
            TEST(
                strcmp(
                    line,
                    "photo.jpg,IMAGE,MEDIUM,4,MEDIUM\n"
                ) == 0,
                "IMAGE and MEDIUM classification"
            );
            break;

        case 3:
            TEST(
                strcmp(
                    line,
                    "sound.mp3,AUDIO,SMALL,3,LOW\n"
                ) == 0,
                "AUDIO classification"
            );
            break;

        case 4:
            TEST(
                strcmp(
                    line,
                    "movie.mp4,VIDEO,LARGE,6,MEDIUM\n"
                ) == 0,
                "VIDEO and LARGE classification"
            );
            break;

        case 5:
            TEST(
                strcmp(
                    line,
                    "archive.zip,ARCHIVE,SMALL,5,MEDIUM\n"
                ) == 0,
                "ARCHIVE classification"
            );
            break;

        case 6:
            TEST(
                strcmp(
                    line,
                    "program.c,CODE,SMALL,4,MEDIUM\n"
                ) == 0,
                "CODE and owner-write permission"
            );
            break;

        case 7:
            TEST(
                strcmp(
                    line,
                    "unknown.xyz,OTHER,SMALL,2,LOW\n"
                ) == 0,
                "OTHER classification"
            );
            break;

        case 8:
            TEST(
                strcmp(
                    line,
                    "hidden.txt,DOCUMENT,SMALL,4,MEDIUM\n"
                ) == 0,
                "Hidden-file score"
            );
            break;

        case 9:
            TEST(
                strcmp(
                    line,
                    "medium.bin,OTHER,MEDIUM,3,LOW\n"
                ) == 0,
                "MEDIUM boundary"
            );
            break;

        case 10:
            TEST(
                strcmp(
                    line,
                    "small.bin,OTHER,SMALL,2,LOW\n"
                ) == 0,
                "SMALL boundary"
            );
            break;

        case 11:
            TEST(
                strcmp(
                    line,
                    "video.mkv,VIDEO,MEDIUM,7,HIGH\n"
                ) == 0,
                "HIGH risk classification"
            );
            break;

        case 12:
            TEST(
                strcmp(
                    line,
                    "code.py,CODE,SMALL,5,MEDIUM\n"
                ) == 0,
                "Hidden CODE classification"
            );
            break;

        case 13:
            TEST(
                strcmp(
                    line,
                    "archive.rar,ARCHIVE,LARGE,9,HIGH\n"
                ) == 0,
                "Maximum score classification"
            );
            break;

        default:
            break;
        }
    }

    TEST(
        line_number == 13,
        "Correct number of output records"
    );

    fclose(file);
}

/* ---------------------------------------------------------
   Main test runner
   --------------------------------------------------------- */

int main(void)
{
    double execution_time = 0.0;
    int result;

    printf("========================================\n");
    printf("M2 Sequential Analyzer Tests\n");
    printf("========================================\n\n");

    create_test_csv();

    result =
        process_metadata(
            TEST_INPUT,
            TEST_OUTPUT,
            &execution_time
        );

    TEST(
        result == 0,
        "Sequential processing completed successfully"
    );

    TEST(
        execution_time >= 0.0,
        "Execution time measured"
    );

    if (result == 0) {
        test_output();
    }

    printf("\n========================================\n");
    printf("Tests passed: %d/%d\n",
           tests_passed,
           tests_run);
    printf("========================================\n");

    remove(TEST_INPUT);
    remove(TEST_OUTPUT);

    return (tests_passed == tests_run)
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}