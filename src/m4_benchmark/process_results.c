#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 512

typedef struct
{
    char input_size[50];
    int threads;
    char mode[50];
    double execution_time;
} BenchmarkResult;


/*
 * Find the sequential execution time for a
 * particular input size.
 */
double find_sequential_time(BenchmarkResult results[],
                            int count,
                            const char *input_size)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(results[i].input_size, input_size) == 0 &&
            strcmp(results[i].mode, "sequential") == 0)
        {
            return results[i].execution_time;
        }
    }

    return -1.0;
}


/*
 * Calculate speedup:
 *
 * Speedup = Sequential Time / Parallel Time
 */
double calculate_speedup(double sequential_time,
                         double parallel_time)
{
    if (parallel_time <= 0)
    {
        return -1.0;
    }

    return sequential_time / parallel_time;
}


/*
 * Calculate parallel efficiency:
 *
 * Efficiency = Speedup / Number of Threads
 */
double calculate_efficiency(double speedup,
                            int threads)
{
    if (threads <= 0)
    {
        return -1.0;
    }

    return speedup / threads;
}


int main(void)
{
    FILE *input_file;
    FILE *output_file;

    BenchmarkResult results[1000];
    int count = 0;

    char line[MAX_LINE_LENGTH];

    /*
     * Open raw benchmark results.
     */
    input_file = fopen(
        "results/raw/benchmark_results.csv",
        "r"
    );

    if (input_file == NULL)
    {
        printf("Error: Could not open benchmark_results.csv\n");
        return 1;
    }

    /*
     * Skip CSV header.
     */
    fgets(line, sizeof(line), input_file);

    /*
     * Read benchmark results.
     */
    while (fgets(line, sizeof(line), input_file))
    {
        if (count >= 1000)
        {
            printf("Too many benchmark records.\n");
            break;
        }

        if (sscanf(line,
                   "%49[^,],%d,%49[^,],%lf",
                   results[count].input_size,
                   &results[count].threads,
                   results[count].mode,
                   &results[count].execution_time) == 4)
        {
            count++;
        }
    }

    fclose(input_file);

    /*
     * Create processed-results directory.
     */
    system("mkdir -p results/processed");

    /*
     * Open output CSV.
     */
    output_file = fopen(
        "results/processed/performance_summary.csv",
        "w"
    );

    if (output_file == NULL)
    {
        printf("Error: Could not create performance_summary.csv\n");
        return 1;
    }

    /*
     * Write CSV header.
     */
    fprintf(output_file,
            "input_size,threads,sequential_time,"
            "parallel_time,speedup,efficiency\n");

    /*
     * Process each parallel result.
     */
    for (int i = 0; i < count; i++)
    {
        if (strcmp(results[i].mode, "parallel") != 0)
        {
            continue;
        }

        double sequential_time =
            find_sequential_time(
                results,
                count,
                results[i].input_size
            );

        if (sequential_time <= 0)
        {
            printf(
                "Warning: No sequential result found "
                "for %s\n",
                results[i].input_size
            );

            continue;
        }

        double speedup =
            calculate_speedup(
                sequential_time,
                results[i].execution_time
            );

        double efficiency =
            calculate_efficiency(
                speedup,
                results[i].threads
            );

        fprintf(
            output_file,
            "%s,%d,%.6f,%.6f,%.6f,%.6f\n",
            results[i].input_size,
            results[i].threads,
            sequential_time,
            results[i].execution_time,
            speedup,
            efficiency
        );
    }

    fclose(output_file);

    printf("\n========================================\n");
    printf("Performance analysis completed.\n");
    printf("Results saved to:\n");
    printf("results/processed/performance_summary.csv\n");
    printf("========================================\n");

    return 0;
}