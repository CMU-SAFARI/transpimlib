#include "math.h"
#ifdef MEASURE
#include <time.h>
#endif

// Precision overall - could be externally defined
#ifndef PRECISION
#define PRECISION 8 // This needs to match on CPU and DPU side!
#endif

// Values needed to support different float sizes
#define FLOAT_MANTISSA_BITS 23
#define FLOAT_EXPONENT_MAKS 0xFF

// Helper Function
float address_to_float(unsigned int in, int precision, int mantissa_size, int min_exponent){
    int exponent = (int) (in >> mantissa_size) & FLOAT_EXPONENT_MAKS;

    if (in < (1 << mantissa_size)) {
        return ldexpf(in, min_exponent - mantissa_size);
    }

    unsigned int ret = (((126 + min_exponent) + exponent) << FLOAT_MANTISSA_BITS) + ((in & ((1 << mantissa_size) - 1)) << (FLOAT_MANTISSA_BITS - mantissa_size));
    return * (  float  * ) &ret;
}


// Helper Function
void fill_table(double (*original)(), float table[], int precision, int mantissa_size, int min_exponent) {
    for(int i = 0; i < (1<<precision) ; ++i){
        table[i] = (float) original(address_to_float(i, precision, mantissa_size, min_exponent));
        // printf("%d -> %f \n", i, address_to_float(i, precision, mantissa_size, min_exponent));
    }
}


// For GeLU, we have to define the function first, gelu_n is just calculating -gelu(x)
double gelu_p(double x) {
    return x * 0.5 * (1 + erf(x / sqrt(2)));
}

double gelu_n(double x) {
    return - x * 0.5 * (1 + erf(-x / sqrt(2)));
}


void broadcast_tables(struct dpu_set_t set) {
#ifdef MEASURE
    FILE *out_file = fopen("../microbenchmarks/output/setup.csv", "a");
    double start, end;
    // Start Timing 1
    start = clock();
#endif

    /***********************************************************
    *   TANH
    */

    #define TANH_PRECISION PRECISION // This needs to match on CPU and DPU side!
    #define TANH_MANTISSA_SIZE (PRECISION - 3) // This needs to match on CPU and DPU side!
    #define TANH_MIN_EXPONENT -4 // This needs to match on CPU and DPU side!

    float tanh_table[1 << TANH_PRECISION];
    fill_table(tanh, tanh_table, TANH_PRECISION, TANH_MANTISSA_SIZE, TANH_MIN_EXPONENT);
    DPU_ASSERT(dpu_broadcast_to(set, "tanh_table", 0, &tanh_table, sizeof(tanh_table), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 1
    end = clock();
    printf("TANH Setup Time:                         %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-direct_%s_tanh, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 2
    start = clock();
#endif

    /***********************************************************
    *   GELU
    */

    // Address generation function parameters
    #define GELU_PRECISION (PRECISION - 1) // This needs to match on CPU and DPU side!
    #define GELU_MANTISSA_SIZE (PRECISION - 4) // This needs to match on CPU and DPU side!
    #define GELU_MIN_EXPONENT -4 // This needs to match on CPU and DPU side!

    float gelu_table_p[1 << GELU_PRECISION];
    float gelu_table_n[1 << GELU_PRECISION];

    fill_table(gelu_p, gelu_table_p, GELU_PRECISION, GELU_MANTISSA_SIZE, GELU_MIN_EXPONENT);
    fill_table(gelu_n, gelu_table_n, GELU_PRECISION, GELU_MANTISSA_SIZE, GELU_MIN_EXPONENT);

    DPU_ASSERT(dpu_broadcast_to(set, "gelu_table_p", 0, &gelu_table_p, sizeof(gelu_table_p), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "gelu_table_n", 0, &gelu_table_n, sizeof(gelu_table_n), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 2
    end = clock();
    printf("GELU Setup Time:                         %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-direct_%s_gelu, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);
#endif

  /***********************************************************
  *   SIN
  */

#define SIN_COS_TAN_PRECISION PRECISION // This needs to match on CPU and DPU side!
#define SIN_COS_TAN_MANTISSA_SIZE (PRECISION - 3) // This needs to match on CPU and DPU side!
#define SIN_COS_TAN_MIN_EXPONENT -6 // This needs to match on CPU and DPU side!

  float sin_table[1 << SIN_COS_TAN_PRECISION];
  fill_table(sin, sin_table, SIN_COS_TAN_PRECISION, SIN_COS_TAN_MANTISSA_SIZE, SIN_COS_TAN_MIN_EXPONENT);

  DPU_ASSERT(dpu_broadcast_to(set, "sin_table", 0, &sin_table, sizeof(sin_table), DPU_XFER_DEFAULT));

#ifdef MEASURE
  // End Timing 1
    end = clock();
    printf("SIN Setup Time:                         %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "lut-direct_%s_sin, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 2
    start = clock();
#endif
}
