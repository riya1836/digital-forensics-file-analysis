#ifndef SEQUENTIAL_ANALYZER_H
#define SEQUENTIAL_ANALYZER_H

typedef struct {
    char path[1024];
    char filename[256];
    char extension[64];
    long long size_bytes;
    char modified_time[128];
    char permissions[32];
    int is_hidden;
} MetadataRecord;

typedef struct {
    char extension_category[32];
    char size_category[32];
    int score;
    char risk_label[16];
} AnalysisResult;

/* Analyze one metadata record. */
void analyze_record(const MetadataRecord *record,
                    AnalysisResult *result);

/* Process metadata.csv sequentially and create sequential.csv. */
int process_metadata(const char *input_csv,
                     const char *output_csv,
                     double *execution_time_seconds);

#endif