#include <math.h>
#ifdef MEASURE
    #include <time.h>
#endif

// Precision overall - could be externally defined
#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

int _unused_zero_address; // For some LUTs it is not needed to save the zero address (because it is zero anyway, in that case, we just point to this value)

#define M_PI 3.14159265358979323846 // We use this for some table setups

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
 * zero_address & granularity_exponent: values that the DPU uses to correctly assign inputs to table addresses
 */
void fill_table(float xLower, float xUpper, double (*original)(), int size, float table[], int *zero_address, int *granularity_exponent) {
    float x_granularity = (xUpper - xLower) / (float) size;

    frexpf(x_granularity, granularity_exponent);

    // From that exponent figure out the best possible spacing in x
    float x_granularity_rounded = ldexpf(1.0f, *granularity_exponent);
    *zero_address = (int) (-xLower / (xUpper - xLower) * size);

    for(int i = 0; i<size; ++i){
        table[i] = (float) original(x_granularity_rounded * (i - *zero_address));
    }
}

// Generates and Broadcasts all tables to the DPU
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
    int sin_cos_tan_granularity_exponent;

    fill_table(0, M_PI/2, sin, 1 << SIN_COS_TAN_PRECISION, sin_cos_tan_table, &_unused_zero_address, &sin_cos_tan_granularity_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "sin_cos_tan_table", 0, &sin_cos_tan_table, sizeof(sin_cos_tan_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "sin_cos_tan_granularity_exponent", 0, &sin_cos_tan_granularity_exponent, sizeof(sin_cos_tan_granularity_exponent), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 1
    end = clock();
    printf("SIN COS TAN Setup Time:                   %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-ldexpf_%s_sin-cos-tan, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 2
    start = clock();
#endif

    /***********************************************************
    *   SINH / COSH / TANH
    */
    #define SINH_COSH_TANH_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float sinh_table[1 << SINH_COSH_TANH_PRECISION];
    float cosh_table[1 << SINH_COSH_TANH_PRECISION];
    int sinh_cosh_tanh_granularity_exponent;

    DPU_ASSERT(dpu_broadcast_to(set, "sinh_table", 0, &sinh_table, sizeof(sinh_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cosh_table", 0, &cosh_table, sizeof(cosh_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "sinh_cosh_tanh_granularity_exponent", 0, &sinh_cosh_tanh_granularity_exponent, sizeof(sinh_cosh_tanh_granularity_exponent), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 2
    end = clock();
    printf("SINH COSH TANH Setup Time:                %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-ldexpf_%s_sinh-cosh-tanh, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 3
    start = clock();
#endif

    /***********************************************************
    *   EXP
    */
    #define EXP_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float exp_table[1 << EXP_PRECISION];
    int exp_granularity_exponent;

    fill_table(0, log(2), exp, 1 << EXP_PRECISION, exp_table, &_unused_zero_address, &exp_granularity_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "exp_table", 0, &exp_table, sizeof(exp_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "exp_granularity_exponent", 0, &exp_granularity_exponent, sizeof(exp_granularity_exponent), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 3
    end = clock();
    printf("EXP Setup Time:                           %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-ldexpf_%s_exp, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 4
    start = clock();
#endif

    /***********************************************************
    *   LOG
    */
    #define LOG_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float log_table[1 << LOG_PRECISION];
    int log_granularity_exponent;

    fill_table(0, 2, log, 1 << LOG_PRECISION, log_table, &_unused_zero_address, &log_granularity_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "log_table", 0, &log_table, sizeof(log_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "log_granularity_exponent", 0, &log_granularity_exponent, sizeof(log_granularity_exponent), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 4
    end = clock();
    printf("LOG Setup Time:                           %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-ldexpf_%s_log, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 5
    start = clock();
#endif

    /***********************************************************
    *   SQRT
    */
    #define SQRT_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float sqrt_table[1 << SQRT_PRECISION];
    int sqrt_granularity_exponent;

    fill_table(0, 9, sqrt, 1 << SQRT_PRECISION, sqrt_table, &_unused_zero_address, &sqrt_granularity_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "sqrt_table", 0, &sqrt_table, sizeof(sqrt_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "sqrt_granularity_exponent", 0, &sqrt_granularity_exponent, sizeof(sqrt_granularity_exponent), DPU_XFER_DEFAULT));


#ifdef MEASURE
    // End Timing 5
    end = clock();
    printf("SQRT Setup Time:                          %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-ldexpf_%s_sqrt, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 6
    start = clock();
#endif

#ifdef LUT_LDEXPF_INTERPOLATE
    /***********************************************************
    *   CNDF
    */
    #define CNDF_PRECISION PRECISION // This needs to match on CPU and DPU side!
    float cndf_table[1 << CNDF_PRECISION];
    int cndf_granularity_exponent;

    fill_table(0, 9, cndf, 1 << CNDF_PRECISION, cndf_table, &_unused_zero_address, &cndf_granularity_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "cndf_table", 0, &cndf_table, sizeof(cndf_table), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cndf_granularity_exponent", 0, &cndf_granularity_exponent, sizeof(cndf_granularity_exponent), DPU_XFER_DEFAULT));
#endif

#ifdef MEASURE
    // End Timing 6
    end = clock();
    printf("CNDF Setup Time:                          %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-ldexpf_%s_sqrt, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);
#endif
}
