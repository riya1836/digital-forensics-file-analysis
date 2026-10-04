/*
 * M4: Benchmark Automation
 *
 * Runs the project pipeline for:
 *   - small
 *   - medium
 *   - large
 *
 * For each input size:
 *   1. Generate metadata using M1
 *   2. Run M2 sequential implementation
 *   3. Run M3 with 1, 2, 4 and 8 threads
 *   4. Record execution times
 *
 * Raw results are stored in:
 *   results/raw/benchmark_results.csv
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_DATASETS 3
#define NUM_THREAD_COUNTS 4

const char *dataset_names[NUM_DATASETS] =
{
    "small",
    "medium",
    "large"
};

const int thread_counts[NUM_THREAD_COUNTS] =
{
    1,
    2,
    4,
    8
};


/*
 * Get high-resolution monotonic time.
 */
double get_time_seconds(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (double)ts.tv_sec +
           (double)ts.tv_nsec / 1000000000.0;
}


/*
 * Execute a command and measure its execution time.
 *
 * Returns:
 *   execution time in seconds if successful
 *   -1.0 if the command fails
 */
double run_and_measure(const char *command)
{
    double start_time;
    double end_time;

    printf("\nRunning: %s\n", command);

    start_time = get_time_seconds();

    int return_code = system(command);

    end_time = get_time_seconds();

    if (return_code != 0)
    {
        printf("ERROR: Command failed.\n");
        printf("Return code: %d\n", return_code);

        return -1.0;
    }

    double elapsed = end_time - start_time;

    printf("Execution time: %.6f seconds\n", elapsed);

    return elapsed;
}


/*
 * Create required result directories.
 */
void create_result_directories(void)
{
    system("mkdir -p results/raw");
    system("mkdir -p results/processed");
    system("mkdir -p results/graphs");
}


/*
 * Create a new raw benchmark CSV.
 */
int initialize_results_file(void)
{
    FILE *file;

    file = fopen(
        "results/raw/benchmark_results.csv",
        "w"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not create "
            "results/raw/benchmark_results.csv\n"
        );

        return 0;
    }

    fprintf(
        file,
        "input_size,threads,mode,execution_time_seconds\n"
    );

    fclose(file);

    return 1;
}


/*
 * Append one benchmark result.
 */
void save_result(
    const char *input_size,
    int threads,
    const char *mode,
    double execution_time
)
{
    FILE *file;

    file = fopen(
        "results/raw/benchmark_results.csv",
        "a"
    );

    if (file == NULL)
    {
        printf("ERROR: Could not open benchmark results file.\n");
        return;
    }

    fprintf(
        file,
        "%s,%d,%s,%.6f\n",
        input_size,
        threads,
        mode,
        execution_time
    );

    fclose(file);
}


/*
 * Generate metadata for one dataset using M1.
 */
int run_m1(const char *dataset)
{
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "./file_analyzer data/%s intermediate/metadata.csv",
        dataset
    );

    printf("\n----------------------------------------\n");
    printf("M1: Generating metadata for %s dataset\n", dataset);
    printf("----------------------------------------\n");

    int result = system(command);

    if (result != 0)
    {
        printf("ERROR: M1 failed for dataset: %s\n", dataset);
        return 0;
    }

    return 1;
}


/*
 * Run the sequential implementation.
 */
double run_sequential(const char *dataset)
{
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "./sequential_analyzer "
        "intermediate/metadata.csv "
        "outputs/sequential.csv"
    );

    printf("\n----------------------------------------\n");
    printf("Dataset : %s\n", dataset);
    printf("Mode    : sequential\n");
    printf("Threads : 1\n");
    printf("----------------------------------------\n");

    return run_and_measure(command);
}


/*
 * Run the parallel implementation.
 */
double run_parallel(
    const char *dataset,
    int threads
)
{
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "./parallel_analyzer "
        "intermediate/metadata.csv "
        "outputs/parallel.csv "
        "%d",
        threads
    );

    printf("\n----------------------------------------\n");
    printf("Dataset : %s\n", dataset);
    printf("Mode    : parallel\n");
    printf("Threads : %d\n", threads);
    printf("----------------------------------------\n");

    return run_and_measure(command);
}


int main(void)
{
    printf("========================================\n");
    printf(" Digital Forensics - M4 Benchmarking\n");
    printf("========================================\n");

    create_result_directories();

    if (!initialize_results_file())
    {
        return 1;
    }

    /*
     * Process each dataset.
     */
    for (int d = 0; d < NUM_DATASETS; d++)
    {
        const char *dataset = dataset_names[d];

        printf("\n\n");
        printf("========================================\n");
        printf(" Dataset: %s\n", dataset);
        printf("========================================\n");

        /*
         * Step 1:
         * Generate metadata using M1.
         */
        if (!run_m1(dataset))
        {
            printf(
                "Skipping dataset %s because M1 failed.\n",
                dataset
            );

            continue;
        }

        /*
         * Step 2:
         * Run sequential implementation.
         */
        double sequential_time =
            run_sequential(dataset);

        if (sequential_time < 0)
        {
            printf(
                "Skipping sequential result for %s.\n",
                dataset
            );
        }
        else
        {
            save_result(
                dataset,
                1,
                "sequential",
                sequential_time
            );
        }

        /*
         * Step 3:
         * Run parallel implementation for
         * each requested thread count.
         */
        for (int t = 0; t < NUM_THREAD_COUNTS; t++)
        {
            int threads = thread_counts[t];

            double parallel_time =
                run_parallel(dataset, threads);

            if (parallel_time < 0)
            {
                printf(
                    "Skipping %d-thread result for %s.\n",
                    threads,
                    dataset
                );

                continue;
            }

            save_result(
                dataset,
                threads,
                "parallel",
                parallel_time
            );
        }
    }

    printf("\n========================================\n");
    printf(" M4 Benchmarking Completed\n");
    printf("========================================\n");

    printf("\nRaw benchmark results saved to:\n");
    printf("results/raw/benchmark_results.csv\n");

    printf("\nNext steps:\n");
    printf("1. Calculate speedup and efficiency.\n");
    printf("2. Generate performance graphs.\n");
    printf("3. Analyse performance bottlenecks.\n");

    return 0;
}