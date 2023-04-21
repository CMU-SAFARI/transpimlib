#include <math.h>
#include <stdio.h>
#ifdef MEASURE
    #include <time.h>
#endif

// Precision
#define FIXED_FRACTION_BITS 28 // This needs to match on CPU and DPU side!
#define TABLE_LENGTH 28 // This needs to match on CPU and DPU side!

#define M_PI 3.14159265358979323846


void broadcast_tables(struct dpu_set_t set) {
#ifdef MEASURE
    double start, end;
    FILE *out_file = fopen("../microbenchmarks/output/setup.csv", "a");
    // Start Timing 1
    start = clock();
#endif
    /***********************************************************
    *   CIRCULAR ROTATION MODE
    */
    int cordic_arc_tan[TABLE_LENGTH];

    // Create Table for Circular operation
    long powr = 1;
    for (int i = 0; i < TABLE_LENGTH; i++)
    {
        cordic_arc_tan[i] = (int) ((atan(1.0 / (double) powr) * (1 << FIXED_FRACTION_BITS) + 0.5));
        powr <<= 1;
    }

    // Figure out the normalization constant
    float stretch = 1.0f;
    powr = 1;
    for (int i = 0; i < TABLE_LENGTH; i++){
        stretch *= (float) (powr + 1) / (float) powr;
        powr <<= 2;
    }
    int cordic_x_init_circular = (int) (ldexpf(1.0/sqrt(stretch), FIXED_FRACTION_BITS) + 0.5);

    DPU_ASSERT(dpu_broadcast_to(set, "cordic_x_init_circular", 0, &cordic_x_init_circular, sizeof(cordic_x_init_circular), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_arc_tan", 0, &cordic_arc_tan, sizeof(cordic_arc_tan), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 1
    end = clock();
    printf("Circular Setup Time:                      %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "cordic_%s_circular, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 2
    start = clock();
#endif

    /***********************************************************
    *   HYPERBOLIC ROTATION MODE
    */
    int cordic_arc_tanh[TABLE_LENGTH];

    // Create Table for Hyperbolic Operation
    powr = 1;
    for (int i = 0; i < TABLE_LENGTH; i++)
    {
        cordic_arc_tanh[i] = (int) ((atanh(1.0 / (double) powr) * (1 << FIXED_FRACTION_BITS) + 0.5));
        powr <<= 1;
    }

    // Figure out the normalization constants
    float cordic_sqrt_ratio = 1.0f;
    int p = 4;
    for (int i = 1; i < TABLE_LENGTH + 1 ; i++){
        cordic_sqrt_ratio /= sqrt(1.0 - ldexpf(1.0, - 2 * i));
        if (p == i) {
            cordic_sqrt_ratio /= sqrt(1.0 - ldexpf(1.0, - 2 * i));
            p = 3 * i + 1;
        }
    }
    int cordic_x_init_hyperbolic = (int) (ldexpf( cordic_sqrt_ratio, FIXED_FRACTION_BITS) + 0.5);

    printf("%f", cordic_sqrt_ratio);

    // Broadcast Table
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_x_init_hyperbolic", 0, &cordic_x_init_hyperbolic, sizeof(cordic_x_init_hyperbolic), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_sqrt_ratio", 0, &cordic_sqrt_ratio, sizeof(cordic_sqrt_ratio), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_arc_tanh", 0, &cordic_arc_tanh, sizeof(cordic_arc_tanh), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 2
    end = clock();
    printf("Hyperbolic Setup Time:                    %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "cordic_%s_hyperbolic, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);
#endif
}
