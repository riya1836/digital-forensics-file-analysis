import csv
import os
from collections import defaultdict

import matplotlib.pyplot as plt


INPUT_FILE = "results/processed/performance_summary.csv"
OUTPUT_DIR = "results/graphs"


def read_results():
    data = defaultdict(list)

    if not os.path.exists(INPUT_FILE):
        print(f"ERROR: {INPUT_FILE} not found.")
        return data

    with open(INPUT_FILE, "r", newline="") as file:
        reader = csv.DictReader(file)

        for row in reader:
            try:
                dataset = row["input_size"]

                data[dataset].append({
                    "threads": int(row["threads"]),
                    "sequential_time": float(row["sequential_time"]),
                    "parallel_time": float(row["parallel_time"]),
                    "speedup": float(row["speedup"]),
                    "efficiency": float(row["efficiency"])
                })

            except (ValueError, KeyError):
                continue

    return data


def plot_execution_time_vs_input_size(data):
    datasets = ["small", "medium", "large"]

    sequential_times = []
    parallel_times = []
    labels = []

    for dataset in datasets:
        if dataset not in data:
            continue

        rows = data[dataset]

        if not rows:
            continue

        # Sequential time is the same for all rows of a dataset.
        sequential_time = rows[0]["sequential_time"]

        # Use the 1-thread parallel result for comparison.
        parallel_1_thread = None

        for row in rows:
            if row["threads"] == 1:
                parallel_1_thread = row["parallel_time"]
                break

        if parallel_1_thread is None:
            continue

        labels.append(dataset)
        sequential_times.append(sequential_time)
        parallel_times.append(parallel_1_thread)

    if not labels:
        return

    x = range(len(labels))

    plt.figure()

    plt.plot(
        x,
        sequential_times,
        marker="o",
        label="Sequential"
    )

    plt.plot(
        x,
        parallel_times,
        marker="o",
        label="Parallel (1 thread)"
    )

    plt.xticks(list(x), labels)
    plt.xlabel("Input Size")
    plt.ylabel("Execution Time (seconds)")
    plt.title("Execution Time vs Input Size")
    plt.grid(True)
    plt.legend()

    filename = os.path.join(
        OUTPUT_DIR,
        "execution_time_vs_input_size.png"
    )

    plt.savefig(filename, dpi=300, bbox_inches="tight")
    plt.close()


def plot_execution_time_vs_threads(data):
    for dataset, rows in data.items():

        rows.sort(key=lambda x: x["threads"])

        threads = [r["threads"] for r in rows]
        parallel_times = [r["parallel_time"] for r in rows]

        sequential_time = rows[0]["sequential_time"]

        plt.figure()

        plt.plot(
            threads,
            parallel_times,
            marker="o",
            label="Parallel"
        )

        plt.axhline(
            sequential_time,
            linestyle="--",
            label="Sequential"
        )

        plt.xlabel("Number of Threads")
        plt.ylabel("Execution Time (seconds)")
        plt.title(
            f"Execution Time vs Thread Count - {dataset}"
        )
        plt.xticks(threads)
        plt.grid(True)
        plt.legend()

        filename = os.path.join(
            OUTPUT_DIR,
            f"{dataset}_execution_time_vs_threads.png"
        )

        plt.savefig(filename, dpi=300, bbox_inches="tight")
        plt.close()


def plot_speedup(data):
    for dataset, rows in data.items():

        rows.sort(key=lambda x: x["threads"])

        threads = [r["threads"] for r in rows]
        speedups = [r["speedup"] for r in rows]

        plt.figure()

        plt.plot(
            threads,
            speedups,
            marker="o"
        )

        plt.xlabel("Number of Threads")
        plt.ylabel("Speedup")
        plt.title(f"Speedup vs Thread Count - {dataset}")
        plt.xticks(threads)
        plt.grid(True)

        filename = os.path.join(
            OUTPUT_DIR,
            f"{dataset}_speedup.png"
        )

        plt.savefig(filename, dpi=300, bbox_inches="tight")
        plt.close()


def plot_efficiency(data):
    for dataset, rows in data.items():

        rows.sort(key=lambda x: x["threads"])

        threads = [r["threads"] for r in rows]
        efficiencies = [r["efficiency"] for r in rows]

        plt.figure()

        plt.plot(
            threads,
            efficiencies,
            marker="o"
        )

        plt.xlabel("Number of Threads")
        plt.ylabel("Efficiency")
        plt.title(
            f"Parallel Efficiency vs Thread Count - {dataset}"
        )
        plt.xticks(threads)
        plt.grid(True)

        filename = os.path.join(
            OUTPUT_DIR,
            f"{dataset}_efficiency.png"
        )

        plt.savefig(filename, dpi=300, bbox_inches="tight")
        plt.close()


def main():
    print("========================================")
    print(" M4 Performance Graph Generation")
    print("========================================")

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    data = read_results()

    if not data:
        print("\nNo processed benchmark data available.")
        print(
            "Run the benchmark and process the results "
            "before generating graphs."
        )
        return

    plot_execution_time_vs_input_size(data)
    plot_execution_time_vs_threads(data)
    plot_speedup(data)
    plot_efficiency(data)

    print("\nGraphs generated successfully.")
    print(f"Saved to: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()