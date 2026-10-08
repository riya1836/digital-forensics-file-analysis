#!/bin/bash

# M3 Parallel Analyzer Test
# Verifies that the OpenMP parallel analyzer produces
# logically equivalent results to the sequential analyzer.

set -e

echo "========================================"
echo "       M3 PARALLEL ANALYZER TEST"
echo "========================================"

# Check required executables
if [ ! -f "./sequential_analyzer" ]; then
    echo "[FAIL] sequential_analyzer not found."
    echo "Compile it first:"
    echo "gcc -Wall -Wextra -std=c11 src/m2_sequential/sequential_analyzer.c -o sequential_analyzer"
    exit 1
fi

if [ ! -f "./parallel_analyzer" ]; then
    echo "[FAIL] parallel_analyzer not found."
    echo "Compile it first:"
    echo "gcc -Wall -Wextra -std=c11 -fopenmp src/m3_parallel/parallel_analyzer.c -o parallel_analyzer"
    exit 1
fi

# Check metadata input
if [ ! -f "intermediate/metadata.csv" ]; then
    echo "[FAIL] intermediate/metadata.csv not found."
    echo "Run M1 first."
    exit 1
fi

mkdir -p tests/m3_test_output

echo
echo "[1/4] Running sequential analyzer..."
./sequential_analyzer \
    intermediate/metadata.csv \
    tests/m3_test_output/sequential.csv

echo "[PASS] Sequential analyzer completed."

echo
echo "[2/4] Running parallel analyzer with 1 thread..."
./parallel_analyzer \
    intermediate/metadata.csv \
    tests/m3_test_output/parallel_1.csv \
    1

echo "[PASS] Parallel analyzer completed with 1 thread."

echo
echo "[3/4] Running parallel analyzer with 4 threads..."
./parallel_analyzer \
    intermediate/metadata.csv \
    tests/m3_test_output/parallel_4.csv \
    4

echo "[PASS] Parallel analyzer completed with 4 threads."

echo
echo "[4/4] Comparing results..."

# Remove headers before comparison because the CSV files
# have different output headers.
tail -n +2 tests/m3_test_output/sequential.csv > \
    tests/m3_test_output/sequential_data.csv

tail -n +2 tests/m3_test_output/parallel_1.csv > \
    tests/m3_test_output/parallel_1_data.csv

tail -n +2 tests/m3_test_output/parallel_4.csv > \
    tests/m3_test_output/parallel_4_data.csv

if diff -q \
    tests/m3_test_output/sequential_data.csv \
    tests/m3_test_output/parallel_1_data.csv > /dev/null
then
    echo "[PASS] 1-thread output matches sequential output."
else
    echo "[FAIL] 1-thread output does not match sequential output."
    exit 1
fi

if diff -q \
    tests/m3_test_output/sequential_data.csv \
    tests/m3_test_output/parallel_4_data.csv > /dev/null
then
    echo "[PASS] 4-thread output matches sequential output."
else
    echo "[FAIL] 4-thread output does not match sequential output."
    exit 1
fi

# Verify record counts
seq_count=$(wc -l < tests/m3_test_output/sequential_data.csv)
parallel_1_count=$(wc -l < tests/m3_test_output/parallel_1_data.csv)
parallel_4_count=$(wc -l < tests/m3_test_output/parallel_4_data.csv)

echo
echo "Record counts:"
echo "Sequential : $seq_count"
echo "1 thread   : $parallel_1_count"
echo "4 threads  : $parallel_4_count"

if [ "$seq_count" -eq "$parallel_1_count" ] && \
   [ "$seq_count" -eq "$parallel_4_count" ]
then
    echo "[PASS] Record counts are identical."
else
    echo "[FAIL] Record counts differ."
    exit 1
fi

echo
echo "========================================"
echo "          M3 TEST RESULT: PASS"
echo "========================================"