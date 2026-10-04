// M3: OpenMP parallel computation
#ifndef PARALLEL_ANALYZER_H
#define PARALLEL_ANALYZER_H

#include <stddef.h>

#define MAX_PATH_LEN 4096
#define MAX_FIELD_LEN 256

typedef struct {
    char path[MAX_PATH_LEN];
    char extension[MAX_FIELD_LEN];
    long long size_bytes;
    char permissions[MAX_FIELD_LEN];
    int is_hidden;
} MetadataRecord;

typedef struct {
    char path[MAX_PATH_LEN];
    char extension_category[MAX_FIELD_LEN];
    char size_category[MAX_FIELD_LEN];
    int score;
    char risk_label[MAX_FIELD_LEN];
} AnalysisResult;

/* CSV handling */
int read_metadata_csv(
    const char *filename,
    MetadataRecord **records,
    size_t *count
);

/* Analysis functions */
const char *categorize_extension(const char *extension);
const char *categorize_size(long long size_bytes);
int calculate_score(
    const MetadataRecord *record,
    const char *extension_category,
    const char *size_category
);
const char *calculate_risk_label(int score);

/* Parallel analysis */
int analyze_parallel(
    const MetadataRecord *records,
    AnalysisResult *results,
    size_t count,
    int num_threads
);

/* Output */
int write_results_csv(
    const char *filename,
    const AnalysisResult *results,
    size_t count
);

/* Memory */
void free_metadata(MetadataRecord *records);

#endif
