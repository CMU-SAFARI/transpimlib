#include "math.h"
#ifdef MEASURE
    #include <time.h>
#endif

// Precision overall - could be externally defined
#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

int _unused_zero_address; // For some LUTs it is not needed to save the zero address (because it is zero anyway, in that case, we just point to this value)

#define M_PI 3.14159265358979323846

// Function for cndf
# define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */
double cndf(double value)
{
    return ldexp(erfc(-value * M_SQRT1_2), -1);
}

/*
 * Fills a table on the host side and makes it ready to transmit
 * Inputs
 * xLower: Lower end of the table
 * xUpper: Upper end of the table
 * original(): original function that should be tabularized
 *
 * Outputs
 * size: array size (in entries)
 * table[]: Array to use for the table
 * zero_address & x_granularity: values that the DPU uses to correctly assign inputs to table addresses
 */
void fill_table(float xLower, float xUpper, double (*original)(), int size, float table[], int *zero_address, float *x_granularity) {
    float distance = (xUpper - xLower) / (float) (size - 1);

    *x_granularity = 1 / distance;
    *zero_address = (int) (-xLower / (xUpper - xLower) * (size - 1));

    for(int i = 0; i<size ; ++i){
        table[i] = (float) original(distance * (i - *zero_address));
    }
}

void broadcast_tables(struct dpu_set_t set) {
#ifdef MEASURE
    double start, end;
    FILE *out_file = fopen("../microbenchmarks/output/setup.csv", "a");
    // Start Timing 1
    start = clock();
#endif

    /***********************************************************
    *   SIN / COS / TAN
    */
    #define SIN_COS_TAN_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
    float sin_cos_tan_spacing;

    fill_table(0, M_PI/2, sin, 1 << SIN_COS_TAN_PRECISION, sin_cos_tan_table, &_unused_zero_address, &sin_cos_tan_spacing);
    DPU_ASSERT(dpu_broadcast_to(set, "sin_cos_tan_table", 0, &sin_cos_tan_table, sizeof(sin_cos_tan_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "sin_cos_tan_spacing", 0, &sin_cos_tan_spacing, sizeof(sin_cos_tan_spacing), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 1
    end = clock();
    printf("SIN COS TAN Setup Time:                   %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-multi_%s_sin-cos-tan, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 2
    start = clock();
#endif

    /***********************************************************
    *   SINH / COSH / TANH
    */
    #define SINH_COSH_TANH_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float sinh_table[1 << SINH_COSH_TANH_PRECISION];
    float cosh_table[1 << SINH_COSH_TANH_PRECISION];
    float sinh_cosh_tanh_spacing;

    DPU_ASSERT(dpu_broadcast_to(set, "sinh_table", 0, &sinh_table, sizeof(sinh_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cosh_table", 0, &cosh_table, sizeof(cosh_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "sinh_cosh_tanh_spacing", 0, &sinh_cosh_tanh_spacing, sizeof(sinh_cosh_tanh_spacing), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 2
    end = clock();
    printf("SINH COSH TANH Setup Time:                %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-multi_%s_sinh-cosh-tanh, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 3
    start = clock();
#endif

    /***********************************************************
    *   EXP
    */
    #define EXP_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float exp_table[1 << EXP_PRECISION];
    float exp_spacing;

    fill_table(0, log(2), exp, 1 << EXP_PRECISION, exp_table, &_unused_zero_address, &exp_spacing);
    DPU_ASSERT(dpu_broadcast_to(set, "exp_table", 0, &exp_table, sizeof(exp_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "exp_spacing", 0, &exp_spacing, sizeof(exp_spacing), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 3
    end = clock();
    printf("EXP Setup Time:                           %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-multi_%s_exp, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 4
    start = clock();
#endif

    /***********************************************************
    *   LOG
    */
    #define LOG_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float log_table[1 << LOG_PRECISION];
    float log_spacing;

    fill_table(0, 2, log, 1 << LOG_PRECISION, log_table, &_unused_zero_address, &log_spacing);
    DPU_ASSERT(dpu_broadcast_to(set, "log_table", 0, &log_table, sizeof(log_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "log_spacing", 0, &log_spacing, sizeof(log_spacing), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 4
    end = clock();
    printf("LOG Setup Time:                           %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-multi_%s_log, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 5
    start = clock();
#endif
    /***********************************************************
    *   SQRT
    */
    #define SQRT_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float sqrt_table[1 << SQRT_PRECISION];
    float sqrt_spacing;

    fill_table(0, 9, sqrt, 1 << SQRT_PRECISION, sqrt_table, &_unused_zero_address, &sqrt_spacing);
    DPU_ASSERT(dpu_broadcast_to(set, "sqrt_table", 0, &sqrt_table, sizeof(sqrt_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "sqrt_spacing", 0, &sqrt_spacing, sizeof(sqrt_spacing), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 5
    end = clock();
    printf("SQRT Setup Time:                           %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-multi_%s_log, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 6
    start = clock();
#endif

#ifdef LUT_MULTI_INTERPOLATE
    /***********************************************************
    *   CNDF
    */
    #define CNDF_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float cndf_table[1 << CNDF_PRECISION];
    float cndf_spacing;

    fill_table(0, 9, cndf, 1 << CNDF_PRECISION, cndf_table, &_unused_zero_address, &cndf_spacing);
    DPU_ASSERT(dpu_broadcast_to(set, "cndf_table", 0, &cndf_table, sizeof(cndf_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cndf_spacing", 0, &cndf_spacing, sizeof(cndf_spacing), DPU_XFER_DEFAULT));
#endif

#ifdef MEASURE
    // End Timing 6
    end = clock();
    printf("CNDF Setup Time:                          %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-multi_%s_sqrt, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);
#endif
};