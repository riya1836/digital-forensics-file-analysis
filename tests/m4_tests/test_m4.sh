#!/bin/bash

echo "========================================"
echo " M4 Tests"
echo "========================================"

echo "[1] Compiling benchmark.c..."
gcc src/m4_benchmark/benchmark.c -o /tmp/m4_benchmark
if [ $? -ne 0 ]; then
    echo "FAIL: benchmark.c compilation"
    exit 1
fi
echo "PASS"

echo "[2] Compiling process_results.c..."
gcc src/m4_benchmark/process_results.c -o /tmp/m4_process_results
if [ $? -ne 0 ]; then
    echo "FAIL: process_results.c compilation"
    exit 1
fi
echo "PASS"

echo "[3] Compiling compare_results.c..."
gcc src/m4_benchmark/compare_results.c -o /tmp/m4_compare_results
if [ $? -ne 0 ]; then
    echo "FAIL: compare_results.c compilation"
    exit 1
fi
echo "PASS"

echo "[4] Checking raw CSV header..."
EXPECTED="input_size,threads,mode,execution_time_seconds"
ACTUAL=$(head -n 1 results/raw/benchmark_results.csv)

if [ "$ACTUAL" != "$EXPECTED" ]; then
    echo "FAIL: Incorrect raw CSV header"
    exit 1
fi
echo "PASS"

echo "[5] Testing graph generation..."
python3 src/m4_benchmark/generate_graphs.py

if [ $? -ne 0 ]; then
    echo "FAIL: graph generation script"
    exit 1
fi
echo "PASS"

echo ""
echo "========================================"
echo " All M4 basic tests passed."
echo "========================================"
