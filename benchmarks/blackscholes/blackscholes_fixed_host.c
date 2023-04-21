#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#if defined FIXED
#include "../../host/lut_fixed_host.c"
char method[]="lut_fixed";
#else
#include "../../host/lut_fixed_host.c"
char method[]="lut_fixed_interpolate";
#endif

#ifndef DPU_BINARY
#define DPU_BINARY "bin/blackscholes_fixed"
#endif

#define PAD 2560*24

#define fptype float

typedef struct OptionData_ {
    int s;          // spot price
    int strike;     // strike price
    int r;          // risk-free interest rate
    int divq;       // dividend rate
    int v;          // volatility
    int t;          // time to maturity or option expiration in years
    //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)
    char OptionType;   // Option type.  "P"=PUT, "C"=CALL
    int divs;       // dividend vals (not used in this test)
    int DGrefval;   // DerivaGem Reference Value
} OptionData;

OptionData *data;
int *prices;
int numOptions;

int    * otype;
int * sptprice;
int * strike;
int * rate;
int * volatility;
int * otime;
int numError = 0;
int nThreads;


int convert(float x){
    return (int) (ldexpf(x, FIXED_FRACTION_BITS) + 0.5f);
}

float reconvert(int x){
    return ldexpf((float) x, -FIXED_FRACTION_BITS);
}


int main (int argc, char **argv)
{
    FILE *file;
    int i;
    int loopnum;
    int numOptions;
    int * buffer;
    int * buffer2;
    int rv;

    if (argc != 3) {
        printf("Usage:\n\t%s <inputFile> <outputFile>\n", argv[0]);
        exit(1);
    }
    char *inputFile = argv[1];
    char *outputFile = argv[2];

    //Read input data from file
    file = fopen(inputFile, "r");

    if(file == NULL) {
        printf("ERROR: Unable to open file `%s'.\n", inputFile);
        exit(1);
    }
    rv = fscanf(file, "%i", &numOptions);
    if(rv != 1) {
        printf("ERROR: Unable to read from file `%s'.\n", inputFile);
        fclose(file);
        exit(1);
    }

    // alloc spaces for the option data
    data = (OptionData*)malloc((numOptions+PAD)*sizeof(OptionData));
    prices = (int*)malloc((numOptions+PAD)*sizeof(int));
    for ( loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
        float t1, t2, t3, t4, t5, t6, t7 ,t8;
        rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &t1, &t2, &t3, &t4, &t5, &t6, &data[loopnum].OptionType, &t7 ,&t8);
        data[loopnum].s = convert(t1);
        data[loopnum].strike = convert(t2);
        data[loopnum].r = convert(t3);
        data[loopnum].divq = convert(t4);
        data[loopnum].v = convert(t5);
        data[loopnum].t = convert(t6);
        data[loopnum].divs = convert(t7);
        data[loopnum].DGrefval = convert(t8);

        // printf("A: %f %f %f %f %f %f %f %f \n", t1, t2, t3, t4, t5, t6, t7 ,t8);
        // printf("B: %f %f %f %f %f %f %f %f \n", reconvert(data[loopnum].s), reconvert(data[loopnum].strike),  reconvert(data[loopnum].r),  reconvert(data[loopnum].divq), reconvert(data[loopnum].v), reconvert(data[loopnum].t),  reconvert(data[loopnum].divs),   reconvert(data[loopnum].DGrefval));


        if(rv != 9) {
            printf("ERROR: Unable to read from file `%s'.\n", inputFile);
            fclose(file);
            exit(1);
        }
    }
    rv = fclose(file);
    if(rv != 0) {
        printf("ERROR: Unable to close file `%s'.\n", inputFile);
        exit(1);
    }

    // Start measuring time
    struct timeval begin, end, begin_inner, end_inner;
    gettimeofday(&begin, 0);

    // Perform Computation
    struct dpu_set_t set, dpu;
    DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    // Distribute Workload
    unsigned int dpu_amount;
    DPU_ASSERT(dpu_get_nr_dpus(set, &dpu_amount));

    int used_rows = (numOptions - 1) / (dpu_amount) + 1;
    used_rows += used_rows % 2; // Round up so transfers are aligned to 8 bytes

    DPU_ASSERT(dpu_broadcast_to(set, "used_rows", 0, &used_rows, sizeof(int), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, i) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &data[i * used_rows]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "option", 0, sizeof(OptionData) * used_rows, DPU_XFER_DEFAULT));

    gettimeofday(&begin_inner, 0);

    broadcast_tables(set);

    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    gettimeofday(&end_inner, 0);

    DPU_FOREACH(set, dpu, i) {
        // DPU_ASSERT(dpu_log_read(dpu, stdout));
        DPU_ASSERT(dpu_copy_from(dpu, "price", 0, &prices[i * used_rows], sizeof(int) * used_rows));
    }

    DPU_ASSERT(dpu_free(set));

    // Stop measuring time and calculate the elapsed time
    gettimeofday(&end, 0);
    double inner_time = (end_inner.tv_sec - begin_inner.tv_sec) + (end_inner.tv_usec - begin_inner.tv_usec)*1e-6;
    double total_time = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6;

    printf("--- Blackscholes-Fixed on DPU ---\n");
    printf("Method:                       %s\n", method);
    printf("Computation Time:             %.2e secs.\n", inner_time);
    printf("Total Time:                   %.2e secs.\n", total_time);
    printf("Size of data:                 %d\n", numOptions);
    printf("DPUs:                         %d\n", dpu_amount);
    printf("Tasklets:                     %d\n", NR_TASKLETS);
    printf("Rows per DPU:                 %d\n", used_rows);

    // Save Output to File
    FILE *out_file = fopen("output/runs.csv", "a");
    fprintf(out_file, "blackscholes, fixed_%s, %d, %d, %d, %f, %f\n", method, numOptions, dpu_amount, NR_TASKLETS, total_time, inner_time);

    //Write prices to output file
    file = fopen(outputFile, "w");
    if(file == NULL) {
        printf("ERROR: Unable to open file `%s'.\n", outputFile);
        exit(1);
    }
    rv = fprintf(file, "%i\n", numOptions);
    if(rv < 0) {
        printf("ERROR: Unable to write to file `%s'.\n", outputFile);
        fclose(file);
        exit(1);
    }
    for(i=0; i<numOptions; i++) {
        rv = fprintf(file, "%.18f\n", reconvert(prices[i]));
        if(rv < 0) {
            printf("ERROR: Unable to write to file `%s'.\n", outputFile);
            fclose(file);
            exit(1);
        }
    }
    rv = fclose(file);
    if(rv != 0) {
        printf("ERROR: Unable to close file `%s'.\n", outputFile);
        exit(1);
    }

    free(data);
    free(prices);

    return 0;
}
