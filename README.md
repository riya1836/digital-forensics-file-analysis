# Digital Forensics File Analysis Using Sequential and Parallel Computing

## 1. Project Overview

This project implements a **digital-forensics-oriented file analysis system in C**.

The system processes a collection of files, extracts selected file metadata, performs a defined deterministic analysis on the extracted records, and compares:

- a **sequential implementation**, and
- a **parallel implementation using OpenMP**.

The main focus of the project is **Design and Analysis of Algorithms (DAA)**, especially:

- computational problem formulation
- algorithm design
- sequential processing
- parallel processing
- OpenMP
- correctness verification
- execution-time measurement
- speedup
- parallel efficiency
- scalability
- bottleneck analysis

**Machine learning is not a core part of this project.**

The project is deliberately divided into four independent components so that the four team members can develop and test their code separately.

The components communicate through **well-defined files and data formats**, rather than depending on one another's internal source code.

---

# 2. Problem Statement

Digital forensic analysis may require processing a large collection of files and extracting useful information from every file.

When the number of files becomes large, processing every file sequentially can take considerable time.

The computational problem addressed by this project is:

> **Given a collection of files, extract defined metadata for every file, perform a deterministic analysis on those records, and compare the performance of sequential processing with OpenMP-based parallel processing.**

The project must determine:

1. Whether the parallel implementation produces the same logical results as the sequential implementation.
2. Whether parallel processing improves execution time.
3. How input size affects execution time.
4. How the number of OpenMP threads affects performance.
5. Where performance bottlenecks occur.

---

# 3. Project Objectives

The project aims to:

1. Build a file-analysis pipeline.
2. Extract consistent metadata from a collection of files.
3. Perform a deterministic computational analysis on the metadata.
4. Implement the analysis sequentially.
5. Implement the same analysis using OpenMP.
6. Verify sequential and parallel correctness.
7. Measure execution time.
8. Study the effect of input size.
9. Study the effect of thread count.
10. Calculate speedup.
11. Calculate parallel efficiency.
12. Identify performance bottlenecks.
13. Generate tables and graphs from actual measurements.
14. Use LLMs responsibly as development assistants and document their use.

---

# 4. Technology Stack

| Component | Choice |
|---|---|
| Programming Language | C |
| Parallel Framework | OpenMP |
| IDE | Visual Studio Code |
| Compiler | GCC or another C compiler supporting OpenMP |
| Input | File collections |
| Intermediate data format | CSV |
| Output data format | CSV |
| Version Control | Git/GitHub recommended |

---

# 5. Overall Project Architecture

The complete project is divided into four main modules.

```text
                         INPUT FILE COLLECTION
                                  |
                                  v
                    +-----------------------------+
                    |            M1               |
                    |      File Analysis          |
                    |  Metadata Extraction        |
                    +-------------+---------------+
                                  |
                                  v
                           metadata.csv
                                  |
                    +-------------+-------------+
                    |                           |
                    v                           v
          +---------------------+     +---------------------+
          |         M2          |     |         M3          |
          |    Sequential      |     |      Parallel       |
          |    Processing      |     |      OpenMP         |
          +----------+----------+     +----------+----------+
                     |                           |
                     v                           v
              sequential.csv              parallel.csv
                     |                           |
                     +-------------+-------------+
                                   |
                                   v
                    +-----------------------------+
                    |            M4               |
                    | Benchmarking & Performance  |
                    +-------------+---------------+
                                  |
                                  v
                         Results + Graphs
```

## Core principle

Each member develops their component independently.

A member does **not** need to know how another member implemented their code.

Instead, each component follows a fixed **input/output contract** defined in this README.

Therefore:

> **This README is the common contract between all four components.**

---

# 6. Why File-Based Communication Is Used

The four members are intentionally working independently.

Instead of making one member's C program directly depend on another member's source code, the project uses files as interfaces.

For example:

```text
M1
 |
 | produces
 v
metadata.csv
 |
 +----------+
 |          |
 v          v
M2         M3
 |          |
 v          v
sequential.csv    parallel.csv
 |          |
 +----------+
      |
      v
     M4
```

This has several advantages:

- Members can develop independently.
- Members can test their code without having the other member's source code.
- Interfaces are easy to inspect.
- Integration problems are easier to identify.
- The project has clear module boundaries.

---

# 7. Why CSV Is Used

CSV is used as the intermediate and output data format because:

- it is simple to generate in C
- it is simple to read in C
- it is human-readable
- it can be opened in spreadsheet software
- it makes debugging easier
- it does not require a database
- M2 and M3 can independently read the same input
- M4 can easily process the generated results

CSV is therefore an **implementation choice for communication between independent modules**, not a requirement of the computational problem itself.

---

# 8. Complete Folder Structure

The following is the planned final project structure.

```text
digital-forensics-file-analysis/
│
├── README.md
│
├── src/
│   │
│   ├── m1_file_analysis/                    # M1
│   │   ├── file_analyzer.c                  # M1: scans files and creates metadata
│   │   └── file_analyzer.h                  # M1: optional internal declarations
│   │
│   ├── m2_sequential/                       # M2
│   │   ├── sequential_analyzer.c            # M2: sequential computation
│   │   └── sequential_analyzer.h            # M2: optional internal declarations
│   │
│   ├── m3_parallel/                         # M3
│   │   ├── parallel_analyzer.c              # M3: OpenMP parallel computation
│   │   └── parallel_analyzer.h              # M3: optional internal declarations
│   │
│   └── m4_benchmark/                        # M4
│       ├── benchmark.c                      # M4: benchmark execution
│       ├── benchmark.h                      # M4: optional internal declarations
│       └── compare_results.c                # M4: correctness/performance comparison
│
├── data/                                    # M4 prepares; ALL use
│   ├── small/
│   ├── medium/
│   └── large/
│
├── intermediate/                            # M1 produces; M2/M3 consume
│   └── metadata.csv
│
├── outputs/                                 # M2/M3 produce
│   ├── sequential.csv                       # M2
│   └── parallel.csv                         # M3
│
├── results/                                 # M4
│   ├── raw/
│   │   └── benchmark_results.csv            # M4: raw measured timings
│   │
│   ├── processed/
│   │   └── performance_summary.csv          # M4: calculated metrics
│   │
│   └── graphs/                              # M4: generated graphs
│
├── tests/
│   ├── m1_tests/                             # M1: file-analysis tests
│   ├── m2_tests/                             # M2: sequential tests
│   ├── m3_tests/                             # M3: parallel tests
│   └── m4_tests/                             # M4: benchmark tests
│
├── docs/
│   ├── llm_usage_log.md                     # ALL: actual LLM usage
│   └── contribution.md                      # ALL: contribution record
│
└── .gitignore
```

---

# 9. Folder/File Ownership and Responsibilities

## M1 — File Analysis

### Owns

```text
src/m1_file_analysis/
tests/m1_tests/
```

### Creates

```text
intermediate/metadata.csv
```

### Uses

```text
data/small/
data/medium/
data/large/
```

### Main work

- scan the input file collection
- identify files
- extract metadata
- write the metadata CSV
- handle expected file-system errors
- test metadata extraction

---

## M2 — Sequential Processing

### Owns

```text
src/m2_sequential/
tests/m2_tests/
```

### Reads

```text
intermediate/metadata.csv
```

### Creates

```text
outputs/sequential.csv
```

### Main work

- read metadata
- perform the required deterministic analysis sequentially
- calculate results for each record
- produce the required CSV
- measure execution time
- test correctness

---

## M3 — Parallel Processing

### Owns

```text
src/m3_parallel/
tests/m3_tests/
```

### Reads

```text
intermediate/metadata.csv
```

### Creates

```text
outputs/parallel.csv
```

### Main work

- read the same metadata as M2
- perform the same computation
- parallelize suitable independent work using OpenMP
- support configurable thread counts
- avoid race conditions
- produce equivalent results
- measure execution time
- test correctness

---

## M4 — Benchmarking and Performance Analysis

### Owns

```text
src/m4_benchmark/
tests/m4_tests/
results/
```

### Reads

```text
outputs/sequential.csv
outputs/parallel.csv
```

and timing/configuration information from experiments.

### Creates

```text
results/raw/
results/processed/
results/graphs/
```

### Main work

- prepare/organize benchmark datasets
- execute experiments
- record raw timings
- calculate speedup
- calculate efficiency
- compare configurations
- generate graphs
- analyse bottlenecks

---

# 10. Ownership vs Usage

The following table is the definitive ownership map.

| File/Directory | Created/Managed By | Used By |
|---|---|---|
| `README.md` | ALL | ALL |
| `src/m1_file_analysis/` | M1 | M1 |
| `src/m2_sequential/` | M2 | M2 |
| `src/m3_parallel/` | M3 | M3 |
| `src/m4_benchmark/` | M4 | M4 |
| `data/` | M4/ALL | M1, M2/M3 indirectly, M4 |
| `intermediate/metadata.csv` | M1 | M2, M3 |
| `outputs/sequential.csv` | M2 | M4 |
| `outputs/parallel.csv` | M3 | M4 |
| `results/raw/` | M4 | M4, final report |
| `results/processed/` | M4 | M4, final report |
| `results/graphs/` | M4 | ALL, final report |
| `tests/m1_tests/` | M1 | M1 |
| `tests/m2_tests/` | M2 | M2 |
| `tests/m3_tests/` | M3 | M3 |
| `tests/m4_tests/` | M4 | M4 |
| `docs/llm_usage_log.md` | ALL | ALL |
| `docs/contribution.md` | ALL | ALL |

---

# 11. File Metadata Specification

M1 must extract the following metadata for every regular file.

| Field | Meaning |
|---|---|
| `path` | Path relative to the input directory |
| `filename` | File name |
| `extension` | File extension |
| `size_bytes` | File size in bytes |
| `modified_time` | Last modification timestamp |
| `permissions` | File permission representation |
| `is_hidden` | Whether the file is considered hidden |

The project is metadata-oriented. It should not collect unnecessary personal information or file contents.

---

# 12. `metadata.csv` Specification

M1 must produce:

```text
intermediate/metadata.csv
```

The exact column order is:

```text
path,filename,extension,size_bytes,modified_time,permissions,is_hidden
```

Example:

```csv
documents/report.pdf,report.pdf,.pdf,245760,2026-10-01T10:30:00,644,0
images/photo.jpg,photo.jpg,.jpg,1048576,2026-10-01T11:00:00,644,0
scripts/test.c,test.c,.c,4096,2026-10-01T12:00:00,664,0
```

### Rules

- Header is mandatory.
- Column order must not change.
- `size_bytes` is an integer.
- `is_hidden` must be `0` or `1`.
- Paths containing commas must be escaped according to CSV rules.
- Timestamps must use one consistent format.
- Missing values must use one consistent representation.

---

# 13. Computational Analysis

M2 and M3 perform the same deterministic analysis on every metadata record.

The analysis consists of:

1. extension categorization
2. size categorization
3. score calculation
4. final analysis label

The exact rules below are fixed for both implementations.

---

## 13.1 Extension Categories

Extension matching is case-insensitive.

### DOCUMENT

```text
.txt
.pdf
.doc
.docx
.xls
.xlsx
.ppt
.pptx
.csv
```

### IMAGE

```text
.jpg
.jpeg
.png
.gif
.bmp
.tiff
.webp
```

### AUDIO

```text
.mp3
.wav
.flac
.aac
.ogg
```

### VIDEO

```text
.mp4
.avi
.mkv
.mov
.wmv
.webm
```

### ARCHIVE

```text
.zip
.tar
.gz
.bz2
.7z
.rar
```

### CODE

```text
.c
.h
.cpp
.hpp
.java
.py
.js
.ts
.html
.css
```

All other extensions are:

```text
OTHER
```

---

# 14. File Size Categories

Use:

```text
SMALL  : size < 1 MB
MEDIUM : 1 MB <= size < 100 MB
LARGE  : size >= 100 MB
```

For this project:

```text
1 MB = 1024 × 1024 bytes
```

---

# 15. File Analysis Score

The score is:

```text
score =
    category_weight
    + size_weight
    + hidden_weight
    + permission_weight
```

## Category weight

| Category | Weight |
|---|---:|
| DOCUMENT | 1 |
| IMAGE | 2 |
| AUDIO | 2 |
| VIDEO | 3 |
| ARCHIVE | 3 |
| CODE | 2 |
| OTHER | 1 |

## Size weight

| Size | Weight |
|---|---:|
| SMALL | 1 |
| MEDIUM | 2 |
| LARGE | 3 |

## Hidden-file weight

```text
visible = 0
hidden  = 2
```

## Permission weight

```text
1 = owner has write permission
0 = otherwise
```

The score is a **project-defined deterministic analysis score**.

It must not be presented as proof that a file is malicious or as a real-world forensic verdict.

---

# 16. Analysis Label

Use:

```text
score <= 3  → LOW
score <= 6  → MEDIUM
score > 6   → HIGH
```

The same rules must be implemented by M2 and M3.

---

# 17. Sequential Output Specification

M2 must produce:

```text
outputs/sequential.csv
```

Required columns:

```text
path,extension_category,size_category,score,risk_label
```

Example:

```csv
documents/report.pdf,DOCUMENT,SMALL,2,LOW
images/photo.jpg,IMAGE,MEDIUM,4,MEDIUM
scripts/test.c,CODE,SMALL,2,LOW
```

---

# 18. Parallel Output Specification

M3 must produce:

```text
outputs/parallel.csv
```

with exactly the same columns:

```text
path,extension_category,size_category,score,risk_label
```

The logical results must be identical to the sequential implementation.

The row order does **not** have to be identical because parallel execution may complete records in a different order.

Correctness must therefore be checked by matching records using `path`.

---

# 19. Correctness Verification

The fundamental correctness requirement is:

```text
For every input path:

M2 result == M3 result
```

The following fields must match:

```text
extension_category
size_category
score
risk_label
```

The project must compare logical records rather than relying only on byte-for-byte comparison of CSV files.

---

# 20. Parallelization Strategy

M3 must implement the same logical computation as M2 but parallelize independent record processing.

Conceptually:

```text
Sequential:

Record 1 → analyze
Record 2 → analyze
Record 3 → analyze
Record 4 → analyze
...
```

Parallel:

```text
                 Records
                    |
        +-----------+-----------+
        |           |           |
        v           v           v
     Thread 1    Thread 2    Thread 3 ...
        |           |           |
        v           v           v
     Records      Records      Records
        |           |           |
        +-----------+-----------+
                    |
                    v
                 Results
```

The parallel implementation must avoid race conditions.

Each record should be processed independently wherever possible.

Shared resources such as output files must be handled safely.

---

# 21. Input Data

The project does **not require a Kaggle dataset or machine-learning dataset**.

Instead, it uses controlled file collections.

Recommended structure:

```text
data/
├── small/
├── medium/
└── large/
```

Recommended starting sizes:

```text
small  = approximately 1,000 files
medium = approximately 5,000 files
large  = approximately 10,000 files
```

These values may be adjusted based on the machine's capabilities.

The important requirement is to have multiple input sizes that make performance comparisons meaningful.

---

# 22. Test Data Composition

The test collections should contain a mixture of supported file categories:

- documents
- images
- audio
- video
- archives
- code
- other files

Where practical, the datasets should also vary in:

- file size
- extension
- hidden/visible status
- permissions

No private or sensitive personal files should be committed to the repository.

---

# 23. Testing Strategy

Each member must be able to test their component independently.

## M1 Tests

Test:

- valid directories
- multiple file types
- empty files
- different file sizes
- hidden files where supported
- permissions where supported
- inaccessible files/directories where applicable
- correct CSV generation

---

## M2 Tests

Test using a small manually verified `metadata.csv`.

Verify:

- extension category
- size category
- score
- label
- output format
- execution time

---

## M3 Tests

Use the same test `metadata.csv`.

Verify:

- correct output
- one thread
- multiple threads
- no race-related inconsistencies
- equivalence with expected results
- equivalence with M2 after integration

---

## M4 Tests

Verify:

- benchmark execution
- timing collection
- thread configuration
- input-size configuration
- speedup calculation
- efficiency calculation
- result storage
- graph generation

---

# 24. Independent Testing Principle

A member must be able to test their component without requiring another member's source code.

For example, M3 can create a small test `metadata.csv` manually:

```csv
path,filename,extension,size_bytes,modified_time,permissions,is_hidden
a.txt,a.txt,.txt,1000,2026-10-01T10:00:00,644,0
photo.jpg,photo.jpg,.jpg,2000000,2026-10-01T10:00:00,644,0
code.c,code.c,.c,4096,2026-10-01T10:00:00,664,0
```

M3 can then test the OpenMP implementation without having M2's code.

This is a core reason for defining the CSV contract in advance.

---

# 25. Performance Evaluation

The project must evaluate:

- sequential execution time
- parallel execution time
- effect of thread count
- effect of input size
- speedup
- parallel efficiency
- bottlenecks

The measurements must come from actual program execution.

---

# 26. Execution Time

Measure the time required for the computational part being compared.

Record:

```text
T_sequential
T_parallel
```

The same logical workload must be used for both implementations.

---

# 27. Speedup

Use:

```text
Speedup = T_sequential / T_parallel
```

Example:

```text
T_sequential = 8 seconds
T_parallel   = 2 seconds

Speedup = 8 / 2
        = 4
```

---

# 28. Parallel Efficiency

Use:

```text
Efficiency = Speedup / Number_of_Threads
```

or:

```text
Efficiency (%) =
    (Speedup / Number_of_Threads) × 100
```

Example:

```text
Speedup = 4
Threads = 4

Efficiency = 4 / 4
           = 1
           = 100%
```

Actual results are expected to vary.

---

# 29. Thread Experiments

M4 should test suitable thread counts based on the available machine.

A recommended starting configuration is:

```text
1
2
4
8
```

If the machine does not support a useful configuration, an appropriate subset may be used and documented.

The project must not assume that increasing thread count always improves performance.

---

# 30. Input-Size Experiments

At minimum, evaluate:

```text
Small
Medium
Large
```

The objective is to determine whether the benefit of parallel processing changes as the workload increases.

---

# 31. Recommended Experimental Matrix

M4 should aim for a structure such as:

| Input Size | Sequential | 1 Thread | 2 Threads | 4 Threads | 8 Threads |
|---|---:|---:|---:|---:|---:|
| Small | measured | measured | measured | measured | measured |
| Medium | measured | measured | measured | measured | measured |
| Large | measured | measured | measured | measured | measured |

Actual values must be obtained through experiments.

---

# 32. Raw Benchmark Results

M4 should store raw measurements in:

```text
results/raw/benchmark_results.csv
```

Suggested format:

```csv
input_size,threads,mode,execution_time_seconds
small,1,sequential,0.000
small,1,parallel,0.000
small,2,parallel,0.000
small,4,parallel,0.000
medium,1,sequential,0.000
...
```

The numbers shown above are placeholders.

**No benchmark result may be fabricated.**

---

# 33. Processed Performance Results

M4 should create:

```text
results/processed/performance_summary.csv
```

Suggested columns:

```text
input_size,threads,sequential_time,parallel_time,speedup,efficiency
```

All values must be calculated from actual measurements.

---

# 34. Recommended Graphs

The final analysis should include appropriate graphs such as:

### Graph 1 — Execution Time vs Input Size

Compare sequential and parallel execution.

### Graph 2 — Execution Time vs Thread Count

Show how parallel execution changes with thread count.

### Graph 3 — Speedup vs Thread Count

Show the scalability of the parallel solution.

### Graph 4 — Efficiency vs Thread Count

Show how effectively additional threads are being utilized.

Only graphs supported by actual experimental data should be included.

---

# 35. Bottleneck Analysis

The final report should discuss observed performance limitations.

Possible factors include:

- file-system I/O
- disk bandwidth
- OpenMP overhead
- thread creation/scheduling overhead
- synchronization
- workload imbalance
- insufficient computational work
- hardware limitations

Conclusions must be based on measured results.

---

# 36. Compilation

For a normal C program:

```bash
gcc source.c -o program
```

For an OpenMP program:

```bash
gcc -fopenmp source.c -o program
```

For multiple C source files:

```bash
gcc file1.c file2.c file3.c -o program
```

With OpenMP:

```bash
gcc -fopenmp file1.c file2.c file3.c -o program
```

The final project may use separate build commands for each module.

---

# 37. Running M1

Conceptually:

```bash
./file_analyzer <input_directory> <metadata_output>
```

Example:

```bash
./file_analyzer data/small intermediate/metadata.csv
```

---

# 38. Running M2

Conceptually:

```bash
./sequential_analyzer intermediate/metadata.csv outputs/sequential.csv
```

---

# 39. Running M3

Conceptually:

```bash
./parallel_analyzer intermediate/metadata.csv outputs/parallel.csv 4
```

Here `4` represents the requested number of OpenMP threads.

The final implementation should use one consistent thread-configuration method.

---

# 40. Running M4

M4 should eventually automate the benchmark workflow:

```text
For each input size:

    Run sequential implementation
    Record execution time

    For each selected thread count:

        Run parallel implementation
        Record execution time

    Calculate speedup
    Calculate efficiency

Store results
Generate graphs
```

---

# 41. Final Integration Procedure

After independent development:

```text
Step 1:
M1 generates metadata.csv

Step 2:
M2 reads metadata.csv
and generates sequential.csv

Step 3:
M3 reads metadata.csv
and generates parallel.csv

Step 4:
Compare M2 and M3 logical results

Step 5:
M4 runs performance experiments

Step 6:
Store raw benchmark measurements

Step 7:
Calculate speedup and efficiency

Step 8:
Generate graphs

Step 9:
Analyse bottlenecks

Step 10:
Prepare final documentation/report
```

---

# 42. LLM-Assisted Development

LLMs may be used as engineering assistants for:

- understanding C
- understanding OpenMP
- algorithm design
- pseudocode
- code generation
- debugging
- compiler-error analysis
- test-case generation
- code review
- optimization suggestions
- documentation

However:

> **LLM-generated code is not automatically correct.**

Every member is responsible for compiling, testing, reviewing, and understanding the code they use.

---

# 43. LLM Usage Log

The project must maintain:

```text
docs/llm_usage_log.md
```

The log should record meaningful LLM usage during development.

Each entry should contain:

```text
Member:
Date:
Purpose:
Actual Prompt:
Summary of Response:
How the Response Was Used:
Modification / Verification:
```

The log should contain **actual prompts used during development**.

Prompts must not be fabricated after the project is completed.

---

# 44. Critical Evaluation of LLM Output

The project should document at least one genuine case where an LLM-generated suggestion was:

- modified,
- rejected,
- or found unsuitable.

The documentation should explain:

1. What the LLM suggested.
2. Why the suggestion was considered.
3. What was changed or rejected.
4. How the final decision was verified.

---

# 45. Contribution Requirements

Each member must have a clearly identifiable coding contribution.

Current allocation:

| Member | Main Component | Main Coding Contribution |
|---|---|---|
| **M1** | File Analysis | File scanning and metadata extraction |
| **M2** | Sequential | Sequential computational analysis |
| **M3** | Parallel | OpenMP parallel computational analysis |
| **M4** | Benchmarking | Benchmark automation, performance metrics, and graphs |

Members may help each other during integration, debugging, and testing, but the main ownership of each component remains as defined above.

The final contribution record should reflect the **actual work performed**, not merely the planned allocation.

---

# 46. Development Rules

## All members must

- follow this README
- preserve the agreed input/output formats
- test their code
- understand the code they submit
- verify LLM-generated suggestions
- record actual LLM usage
- avoid fabricating experimental results
- communicate interface changes before making them

## Members should not

- independently change the CSV schema
- independently change the scoring rules
- depend on another member's internal source code
- copy another member's implementation
- claim performance improvements without measurements
- fabricate LLM prompts
- commit private or sensitive files
- describe the project score as a real malware detector

---

# 47. Definition of Done

## M1 — File Analysis

- [ ] Input directory scanning works.
- [ ] Required metadata is extracted.
- [ ] `metadata.csv` follows the exact schema.
- [ ] Expected errors are handled.
- [ ] M1 tests pass.

## M2 — Sequential

- [ ] Reads `metadata.csv`.
- [ ] Implements the specified computation sequentially.
- [ ] Produces `sequential.csv`.
- [ ] Output follows the exact schema.
- [ ] Execution time can be measured.
- [ ] M2 tests pass.

## M3 — Parallel

- [ ] Reads `metadata.csv`.
- [ ] Implements the same computation as M2.
- [ ] Uses OpenMP.
- [ ] Supports multiple thread counts.
- [ ] Produces `parallel.csv`.
- [ ] Results match M2 logically.
- [ ] Race conditions have been considered/tested.
- [ ] M3 tests pass.

## M4 — Benchmarking

- [ ] Test datasets are available.
- [ ] Sequential timing is collected.
- [ ] Parallel timing is collected.
- [ ] Multiple thread counts are tested.
- [ ] Multiple input sizes are tested.
- [ ] Raw results are stored.
- [ ] Speedup is calculated.
- [ ] Efficiency is calculated.
- [ ] Graphs are generated.
- [ ] Bottlenecks are analysed.

## Team

- [ ] All components integrate.
- [ ] Sequential and parallel results are verified.
- [ ] Performance experiments are reproducible.
- [ ] LLM usage log is complete.
- [ ] Contribution record is complete.
- [ ] Final documentation is complete.

---

# 48. Final Project Summary

The project can be summarized as:

```text
                    FILE COLLECTION
                          |
                          v
               +----------------------+
               | M1: FILE ANALYSIS    |
               | Metadata Extraction  |
               +----------+-----------+
                          |
                          v
                    metadata.csv
                          |
              +-----------+-----------+
              |                       |
              v                       v
       +--------------+       +--------------+
       | M2           |       | M3           |
       | Sequential   |       | OpenMP       |
       | Processing   |       | Parallel     |
       +------+-------+       +------+-------+
              |                      |
              v                      v
       sequential.csv         parallel.csv
              |                      |
              +----------+-----------+
                         |
                         v
                +------------------+
                | M4               |
                | Benchmarking     |
                | Performance      |
                +--------+---------+
                         |
                         v
             Time / Speedup /
             Efficiency / Graphs
                         |
                         v
                  FINAL ANALYSIS
```

## Core Research Question

> **How effectively can OpenMP parallelism accelerate deterministic analysis of a large collection of file metadata compared with sequential processing, and how do input size and thread count affect the observed performance?**
