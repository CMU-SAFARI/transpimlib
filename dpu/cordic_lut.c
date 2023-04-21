#include <stdio.h>
#include "_range_extensions.c"
#include "_quadrants.c"

#pragma ide diagnostic ignored "UnusedLocalVariable"

#ifndef PRECISION
#define PRECISION 22
#endif

#define MAIN_TABLE_LENGTH 28 // This needs to match on CPU and DPU side!

#define CIRCULAR_ROTATION_LUT_PRECISION 8 // This needs to match on CPU and DPU side!
#ifndef SIN_COS_TAN_STORE_IN_WRAM
#define SIN_COS_TAN_STORE_IN_WRAM 0
#endif

#define HYPERBOLIC_ROTATION_LUT_PRECISION 8 // This needs to match on CPU and DPU side!
#ifndef SINH_COSH_TANH_STORE_IN_WRAM
#define SINH_COSH_TANH_STORE_IN_WRAM 0
#endif

// Cordic Modes
#define CIRCULAR_ROTATION  1
#define HYPERBOLIC_ROTATION 2

__host int cordic_arc_tan[MAIN_TABLE_LENGTH];
__host int cordic_arc_tanh[MAIN_TABLE_LENGTH];

__host int sin_cos_tan_granularity_exponent;
#if SIN_COS_TAN_STORE_IN_WRAM > 0
    __host int cordic_circular_rotation_x[1<<CIRCULAR_ROTATION_LUT_PRECISION];
    __host int cordic_circular_rotation_y[1<<CIRCULAR_ROTATION_LUT_PRECISION];
#else
    __mram_noinit int cordic_circular_rotation_x[1<<CIRCULAR_ROTATION_LUT_PRECISION];
    __mram_noinit int cordic_circular_rotation_y[1<<CIRCULAR_ROTATION_LUT_PRECISION];
#endif

__host int hyperbolic_granularity_exponent;
#if SINH_COSH_TANH_STORE_IN_WRAM > 0
    __host int cordic_hyperbolic_rotation_x[1<<HYPERBOLIC_ROTATION_LUT_PRECISION];
    __host int cordic_hyperbolic_rotation_y[1<<HYPERBOLIC_ROTATION_LUT_PRECISION];
#else
    __mram_noinit int cordic_hyperbolic_rotation_x[1<<HYPERBOLIC_ROTATION_LUT_PRECISION];
    __mram_noinit int cordic_hyperbolic_rotation_y[1<<HYPERBOLIC_ROTATION_LUT_PRECISION];
#endif


// Helper Function
static inline void cordic(int angle, int coordinate_system, int total_precision, int *x_ret, int *y_ret, int *angle_ret) {
    int flag = 0;
    int iteration;
    int repeat = 13; // WARNING: If the table size is <4, then there is another corrective iteration needed
    int x, y, x1, y1;

    if (coordinate_system == CIRCULAR_ROTATION) {
        iteration = -sin_cos_tan_granularity_exponent + 1;

        unsigned int address = ((unsigned int) -angle) >> (FIXED_FRACTION_BITS + sin_cos_tan_granularity_exponent);

        x = cordic_circular_rotation_x[address];
        y = cordic_circular_rotation_y[address];
        angle += address << (FIXED_FRACTION_BITS + sin_cos_tan_granularity_exponent);

        while (iteration < total_precision) {
            if (angle < 0) {
                angle += cordic_arc_tan[iteration];
                y1 = y + (x >> iteration);
                x1 = x - (y >> iteration);
            } else {
                angle -= cordic_arc_tan[iteration];
                y1 = y - (x >> iteration);
                x1 = x + (y >> iteration);
            }

            x = x1;
            y = y1;
            iteration += 1;
        }

    } else if (coordinate_system == HYPERBOLIC_ROTATION) {
        iteration = -hyperbolic_granularity_exponent + 1;

        unsigned int address = ((unsigned int) angle) >> (FIXED_FRACTION_BITS + hyperbolic_granularity_exponent);

        x = cordic_hyperbolic_rotation_x[address];
        y = cordic_hyperbolic_rotation_y[address];
        angle -= address << (FIXED_FRACTION_BITS + hyperbolic_granularity_exponent);

        while (iteration < total_precision) {
            if (angle > 0) {
                angle -= cordic_arc_tanh[iteration];
                y1 = y + (x >> iteration);
                x1 = x + (y >> iteration);
            } else {
                angle += cordic_arc_tanh[iteration];
                y1 = y - (x >> iteration);
                x1 = x - (y >> iteration);
            }

            x = x1;
            y = y1;

            if (iteration == repeat) {
                if (flag == 0) {
                    flag = 1;
                } else {
                    flag = 0;
                    repeat = repeat + (repeat << 1) + 1;
                    iteration++;
                }
            } else {
                iteration++;
            }
        }
    }

    *x_ret = x;
    *y_ret = y;
    *angle_ret = angle;
}

float sinf(float angle) {
    int cos, sin, z_cordic, quadrant;
    int angle_int = sin_cos_tan_in(angle, &quadrant);
    cordic(-angle_int, CIRCULAR_ROTATION, PRECISION, &cos, &sin, &z_cordic);
    return sin_out(sin, &quadrant);
}


float cosf(float angle) {
    int cos, sin, z_cordic, quadrant;
    int angle_int = sin_cos_tan_in(angle, &quadrant);
    cordic(-angle_int, CIRCULAR_ROTATION, PRECISION, &cos, &sin, &z_cordic);
    return cos_out(cos, &quadrant);
}

float tanf(float angle) {
    int cos, sin, z_cordic, quadrant;
    int angle_int = sin_cos_tan_in(angle, &quadrant);
    cordic(-angle_int, CIRCULAR_ROTATION, PRECISION, &cos, &sin, &z_cordic);
    return tan_out(cos, sin, &quadrant);
}


float coshf(float x) {
    int sinh, cosh, z_cordic;
    cordic(floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return fixed_to_floating(cosh);
}

float sinhf(float x) {
    int sinh, cosh, z_cordic;
    cordic(floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return fixed_to_floating(sinh);
}

float tanhf(float x) {
    int sinh, cosh, z_cordic;
    cordic(floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return (float) sinh / (float) cosh;
}

float expf(float x) {
    int sinh, cosh, z_cordic;
#ifdef NOWRAP
    cordic(floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return fixed_to_floating(sinh + cosh);
#else
    int shift;
    cordic(floating_to_fixed(exp_range_extension_in(x, &shift)), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return exp_range_extension_out(fixed_to_floating(sinh + cosh), &shift);
#endif
}