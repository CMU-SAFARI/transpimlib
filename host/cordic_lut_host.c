#include <math.h>
#include <stdio.h>
#ifdef MEASURE
    #include <time.h>
#endif

// Precision
#define FIXED_POINT_PLACES_AFTER_COMMA 28 // This needs to match on CPU and DPU side!
#define TABLE_LENGTH 28 // This needs to match on CPU and DPU side!

#define M_PI 3.14159265358979323846

int _unused_zero_address; // For some LUTs it is not needed to save the zero address (because it is zero anyway, in that case, we just point to this value)

/*
 * Fills a table on the host side and makes it ready to transmit
 * Inputs
 * xLower: Lower end of the table
 * xUpper: Upper end of the table
 * original_x(): original function that should be tabularized for the x value of cordic
 * original_y(): original function that should be tabularized for the y value of cordic
 * stretch_factor: additional multiplication to the original function to account for further cordic stretching
 * => The values that get saved in the table are: stretch_factor * original_x()
 *
 * Outputs
 * size: array size (in entries)
 * table_x[]: Array to use for the table for x values
 * table_y[]: Array to use for the table for y values
 * zero_address & granularity_exponent: values that the DPU uses to correctly assign inputs to table addresses
 */
void fill_table(
        float xLower,
        float xUpper,
        double (*original_x)(),
        double (*original_y)(),
        double stretch_factor,
        int size,
        int table_x[],
        int table_y[],
        int *zero_address,
        int *granularity_exponent
) {

    float x_granularity = (xUpper - xLower) / (float) size;

    // Do some bit twiddling to figure out the exponent
    unsigned int x_granularity_binary;
    x_granularity_binary = * ( unsigned int * ) &x_granularity;
    *granularity_exponent = (x_granularity_binary >> 23) - 126; // Get the Exponent for int32

    // From that exponent figure out the best possible spacing in x
    float x_granularity_rounded = ldexpf(1.0f, *granularity_exponent);
    *zero_address = (int) (-xLower / (xUpper - xLower) * size);

    // printf("Gran: %f, Round Gran: %f: Gran Exp: %d, Zero Addr: %d\n", x_granularity, x_granularity_rounded, lut_granularity_exponent, lut_zero_address);

    // Double loop from 0 so there is no added up floating point error
    float x = 0;
    for(int i = 0; i<size - *zero_address ; ++i){
        // printf("Address: %d, Value: %f \n", i, x);
        table_x[*zero_address + i] = (int) (ldexpf(original_x(x) * stretch_factor, FIXED_POINT_PLACES_AFTER_COMMA) + 0.5f);
        table_y[*zero_address + i] = (int) (ldexpf(original_y(x) * stretch_factor, FIXED_POINT_PLACES_AFTER_COMMA) + 0.5f);
        x += x_granularity_rounded;
    }

    x = - x_granularity_rounded;;
    for(int i = 0; i<*zero_address ; ++i){
        // printf("Address: %d, Value: %f \n", i, x);
        table_x[*zero_address - i - 1] = (int) (ldexpf(original_x(x) * stretch_factor, FIXED_POINT_PLACES_AFTER_COMMA) + 0.5f);
        table_y[*zero_address - i - 1] = (int) (ldexpf(original_y(x) * stretch_factor, FIXED_POINT_PLACES_AFTER_COMMA) + 0.5f);
        x -= x_granularity_rounded;
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
    *   CIRCULAR ROTATION MODE
    */
    #define CIRCULAR_ROTATION_LUT_PRECISION 8 // This needs to match on CPU and DPU side!
    int cordic_arc_tan[TABLE_LENGTH];
    int cordic_circular_rotation_x[1<<CIRCULAR_ROTATION_LUT_PRECISION];
    int cordic_circular_rotation_y[1<<CIRCULAR_ROTATION_LUT_PRECISION];
    int sin_cos_tan_exponent;

    // Create Table for Circular operation
    long powr = 1;
    for (int i = 0; i < TABLE_LENGTH; i++)
    {
        cordic_arc_tan[i] = (int) ((atan(1.0 / (double) powr) * (1 << FIXED_POINT_PLACES_AFTER_COMMA) + 0.5));
        powr <<= 1;
    }

    powr = 2 << (2 * CIRCULAR_ROTATION_LUT_PRECISION);
    double cordic_circular_stretch_factor = 1.0;
    for (int i = CIRCULAR_ROTATION_LUT_PRECISION; i < PRECISION; i++){
        cordic_circular_stretch_factor /= (double) (powr + 1) / (double) powr;
        powr <<= 2;
    }

    DPU_ASSERT(dpu_broadcast_to(set, "cordic_arc_tan", 0, &cordic_arc_tan, sizeof(cordic_arc_tan), DPU_XFER_DEFAULT));
    fill_table(0, M_PI/2, cos, sin, cordic_circular_stretch_factor, 1 << CIRCULAR_ROTATION_LUT_PRECISION, cordic_circular_rotation_x, cordic_circular_rotation_y,  &_unused_zero_address, &sin_cos_tan_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "sin_cos_tan_granularity_exponent", 0, &sin_cos_tan_exponent, sizeof(sin_cos_tan_exponent), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_circular_rotation_x", 0, &cordic_circular_rotation_x, sizeof(cordic_circular_rotation_x), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_circular_rotation_y", 0, &cordic_circular_rotation_y, sizeof(cordic_circular_rotation_y), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 1
    end = clock();
    printf("Circular Setup Time:                      %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "cordic-lut_%s_circular, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);

    // Start Timing 2
    start = clock();
#endif

    /***********************************************************
    *   HYPERBOLIC ROTATION MODE
    */
    #define HYPERBOLIC_ROTATION_LUT_PRECISION 8 // This needs to match on CPU and DPU side!
    int cordic_arc_tanh[TABLE_LENGTH];
    int cordic_hyperbolic_rotation_x[1<<HYPERBOLIC_ROTATION_LUT_PRECISION];
    int cordic_hyperbolic_rotation_y[1<<HYPERBOLIC_ROTATION_LUT_PRECISION];
    int hyperbolic_granularity_exponent;

    // Create Table for Hyperbolic Operation
    powr = 1;
    for (int i = 0; i < TABLE_LENGTH; i++)
    {
        cordic_arc_tanh[i] = (int) ((atanh(1.0 / (double) powr) * (1 << FIXED_POINT_PLACES_AFTER_COMMA) + 0.5));
        powr <<= 1;
    }

    // Figure out the normalization constants
    // Some iterations get skipped at the start because we have them in the table
    // mindfull that we skip some iterations at the start
    double cordic_hyperbolic_stretch_factor = 1.0;
    int repeat_iteration = 4;
    for (int i = 1; i < TABLE_LENGTH + 1 ; i++){
        if (i >= HYPERBOLIC_ROTATION_LUT_PRECISION) cordic_hyperbolic_stretch_factor /= sqrt(1.0 - ldexpf(1.0, - 2 * i));
        if (repeat_iteration == i) {
            if (i >= HYPERBOLIC_ROTATION_LUT_PRECISION) cordic_hyperbolic_stretch_factor /= sqrt(1.0 - ldexpf(1.0, - 2 * i));
            repeat_iteration = 3 * repeat_iteration + 1;
        }
    }

    DPU_ASSERT(dpu_broadcast_to(set, "cordic_arc_tanh", 0, &cordic_arc_tanh, sizeof(cordic_arc_tanh), DPU_XFER_DEFAULT));
    fill_table(0, 1, cosh, sinh, cordic_hyperbolic_stretch_factor, 1 << CIRCULAR_ROTATION_LUT_PRECISION, cordic_hyperbolic_rotation_x, cordic_hyperbolic_rotation_y, &_unused_zero_address, &hyperbolic_granularity_exponent);
    DPU_ASSERT(dpu_broadcast_to(set, "hyperbolic_granularity_exponent", 0, &hyperbolic_granularity_exponent, sizeof(hyperbolic_granularity_exponent), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_hyperbolic_rotation_x", 0, &cordic_hyperbolic_rotation_x, sizeof(cordic_hyperbolic_rotation_x), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, "cordic_hyperbolic_rotation_y", 0, &cordic_hyperbolic_rotation_y, sizeof(cordic_hyperbolic_rotation_y), DPU_XFER_DEFAULT));

#ifdef MEASURE
    // End Timing 2
    end = clock();
    printf("Hyperbolic Setup Time:                    %.2e secs.\n", (end - start) / CLOCKS_PER_SEC );
    fprintf(out_file, "cordic-lut_%s_hyperbolic, %d, %e\n", storage, PRECISION, (end - start) / CLOCKS_PER_SEC);
#endif
}
