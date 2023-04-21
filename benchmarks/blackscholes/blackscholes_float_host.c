#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#ifdef CORDIC_F2F
    #include "../../host/cordic_f2f_host.c"
    char method[]="cordic_f2f";
#elif defined CORDIC_LUT
    #include "../../host/cordic_lut_host.c"
    char method[]="cordic_lut";
#elif defined LUT_LDEXPF
    #include "../../host/lut_ldexpf_host.c"
    char method[]="lut_ldexpf_nointerpolate";
#elif defined LUT_LDEXPF_INTERPOLATE
    #include "../../host/lut_ldexpf_host.c"
    char method[]="lut_ldexpf_interpolate";
#elif defined LUT_MULTI
    #include "../../host/lut_multi_host.c"
    char method[]="lut_multi_nointerpolate";
#elif defined LUT_MULTI_INTERPOLATE
    #include "../../host/lut_multi_host.c"
    char method[]="lut_multi_interpolate";
#elif defined LUT_DIRECT
    #include "../../host/lut_direct_host.c"
    char method[]="lut_direct_interpolate";
#elif defined POLYNOMIAL
    char method[]="polynomial";
#endif

#ifndef DPU_BINARY
#define DPU_BINARY "bin/blackscholes_float"
#endif

#define PAD 2560*24

#define fptype float

typedef struct OptionData_ {
    fptype s;          // spot price
    fptype strike;     // strike price
    fptype r;          // risk-free interest rate
    fptype divq;       // dividend rate
    fptype v;          // volatility
    fptype t;          // time to maturity or option expiration in years
    //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)
    char OptionType;   // Option type.  "P"=PUT, "C"=CALL
    fptype divs;       // dividend vals (not used in this test)
    fptype DGrefval;   // DerivaGem Reference Value
} OptionData;

OptionData *data;
fptype *prices;
int numOptions;

int    * otype;
fptype * sptprice;
fptype * strike;
fptype * rate;
fptype * volatility;
fptype * otime;
int numError = 0;
int nThreads;


int main (int argc, char **argv)
{
    FILE *file;
    int i;
    int loopnum;
    int numOptions;
    fptype * buffer;
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
    prices = (fptype*)malloc((numOptions+PAD)*sizeof(fptype));
    for ( loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
        rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[loopnum].s, &data[loopnum].strike, &data[loopnum].r, &data[loopnum].divq, &data[loopnum].v, &data[loopnum].t, &data[loopnum].OptionType, &data[loopnum].divs, &data[loopnum].DGrefval);
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

    DPU_ASSERT(dpu_broadcast_to(set, "used_rows", 0, &used_rows, sizeof(float), DPU_XFER_DEFAULT));



    DPU_FOREACH(set, dpu, i) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &data[i * used_rows]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "option", 0, sizeof(OptionData) * used_rows, DPU_XFER_DEFAULT));

    gettimeofday(&begin_inner, 0);

#ifndef POLYNOMIAL
    broadcast_tables(set);
#endif

    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    gettimeofday(&end_inner, 0);

    DPU_FOREACH(set, dpu, i) {
        // DPU_ASSERT(dpu_log_read(dpu, stdout));
        DPU_ASSERT(dpu_copy_from(dpu, "price", 0, &prices[i * used_rows], sizeof(float) * used_rows));
    }

    DPU_ASSERT(dpu_free(set));

    // Stop measuring time and calculate the elapsed time
    gettimeofday(&end, 0);
    double inner_time = (end_inner.tv_sec - begin_inner.tv_sec) + (end_inner.tv_usec - begin_inner.tv_usec)*1e-6;
    double total_time = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6;

    printf("--- Blackscholes on DPU ---\n");
    printf("Method:                       %s\n", method);
    printf("Computation Time:             %.2e secs.\n", inner_time);
    printf("Total Time:                   %.2e secs.\n", total_time);
    printf("Size of data:                 %d\n", numOptions);
    printf("DPUs:                         %d\n", dpu_amount);
    printf("Tasklets:                     %d\n", NR_TASKLETS);
    printf("Rows per DPU:                 %d\n", used_rows);

    // Save Output to File
    FILE *out_file = fopen("output/runs.csv", "a");
    fprintf(out_file, "blackscholes, float_%s, %d, %d, %d, %f, %f\n", method, numOptions, dpu_amount, NR_TASKLETS, total_time, inner_time);

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
        rv = fprintf(file, "%.18f\n", prices[i]);
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
