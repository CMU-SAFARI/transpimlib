#include "_range_extensions.c"
#include "_quadrants.c"

#ifndef PRECISION
#define PRECISION 22
#endif

#define MAIN_TABLE_LENGTH 28 // This needs to match on CPU and DPU side!

// Cordic Modes
#define CIRCULAR_ROTATION  1
#define HYPERBOLIC_ROTATION 2
#define HYPERBOLIC_VECTOR 3
#define SQRT_MINIMAL 4

// Storage
__host int cordic_x_init_circular;
__host int cordic_x_init_hyperbolic;
__host float cordic_sqrt_ratio;

__host int cordic_arc_tan[MAIN_TABLE_LENGTH];
__host int cordic_arc_tanh[MAIN_TABLE_LENGTH];


// Helper Function
static inline void cordic(int x, int y, int angle, int coordinate_system, int total_precision, int *x_ret, int *y_ret, int *angle_ret) {
    int flag = 0;
    int iteration = 0;
    int repeat = 4;
    int x1, y1;

    if (coordinate_system == CIRCULAR_ROTATION) {
        while (iteration < total_precision) {
            if (angle < 0) {
                // Counter-clockwise rotation.
                angle += cordic_arc_tan[iteration];
                y1 = y + (x >> iteration);
                x1 = x - (y >> iteration);
            } else {
                // Clockwise rotation.
                angle -= cordic_arc_tan[iteration];
                y1 = y - (x >> iteration);
                x1 = x + (y >> iteration);
            }

            x = x1;
            y = y1;
            iteration += 1;
        }

    }
    else if (coordinate_system == HYPERBOLIC_ROTATION) {
        iteration = 1;

        while (iteration < total_precision) {
            if (angle > 0) {
                angle -= cordic_arc_tanh[iteration];
                y1 = y + (x >> iteration);
                x1 = x + (y >> iteration);
            }
            else {
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
    else if (coordinate_system == HYPERBOLIC_VECTOR) {
        iteration = 1;

        while (iteration < total_precision) {
            if (y < 0) {
                angle -= cordic_arc_tanh[iteration];
                y1 = y + (x >> iteration);
                x1 = x + (y >> iteration);
            }
            else {
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
    else if (coordinate_system == SQRT_MINIMAL){
        iteration = 1;

        while (iteration < total_precision) {
            if (y < 0) {
                y1 = y + (x >> iteration);
                x1 = x + (y >> iteration);
            }
            else {
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
    cordic(cordic_x_init_circular, 0, -angle_int, CIRCULAR_ROTATION, PRECISION, &cos, &sin, &z_cordic);
    return sin_out(sin, &quadrant);
}


float cosf(float angle) {
    int cos, sin, z_cordic, quadrant;
    int angle_int = sin_cos_tan_in(angle, &quadrant);
    cordic(cordic_x_init_circular, 0, -angle_int, CIRCULAR_ROTATION, PRECISION, &cos, &sin, &z_cordic);
    return cos_out(cos, &quadrant);
}

float tanf(float angle) {
    int cos, sin, z_cordic, quadrant;
    int angle_int = sin_cos_tan_in(angle, &quadrant);
    cordic(cordic_x_init_circular, 0, -angle_int, CIRCULAR_ROTATION, PRECISION, &cos, &sin, &z_cordic);
    return tan_out(cos, sin, &quadrant);
}

float coshf(float x) {
    int sinh, cosh, z_cordic;
    cordic(cordic_x_init_hyperbolic, 0, floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return fixed_to_floating(cosh);
}

float sinhf(float x) {
    int sinh, cosh, z_cordic;
    cordic(cordic_x_init_hyperbolic, 0, floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return fixed_to_floating(sinh);
}

float tanhf(float x) {
    int sinh, cosh, z_cordic;
    cordic(cordic_x_init_hyperbolic, 0, floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return (float) sinh / (float) cosh;  // Todo: Think about replacing this division
}

float expf(float x) {
    int sinh, cosh, z_cordic;
#ifdef NOWRAP
    cordic(cordic_x_init_hyperbolic, 0, floating_to_fixed(x), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
        return fixed_to_floating(sinh + cosh);
#else
    int extra_data;
    cordic(cordic_x_init_hyperbolic, 0, floating_to_fixed(exp_range_extension_in(x, &extra_data)), HYPERBOLIC_ROTATION, PRECISION, &cosh, &sinh, &z_cordic);
    return exp_range_extension_out(fixed_to_floating(sinh + cosh), &extra_data);
#endif
}

float logf(float x) {
    int x_cordic, y_cordic, z_cordic;
#ifdef NOWRAP
    int x_int = floating_to_fixed(x);
    cordic(x_int + (1<<FIXED_FRACTION_BITS), x_int - (1<<FIXED_FRACTION_BITS), 0, HYPERBOLIC_VECTOR, PRECISION, &x_cordic, &y_cordic, &z_cordic);
    return fixed_to_floating(z_cordic << 1);
#else
    int extra_data;
    int x_int = floating_to_fixed(log_range_extension_in(x, &extra_data));
    cordic(x_int + (1<<FIXED_FRACTION_BITS), x_int - (1<<FIXED_FRACTION_BITS), 0, HYPERBOLIC_VECTOR, PRECISION, &x_cordic, &y_cordic, &z_cordic);
    return log_range_extension_out(fixed_to_floating(z_cordic << 1), &extra_data);
#endif
}

float sqrtf(float x) {
    int x_cordic, y_cordic, z_cordic;
#ifdef NOWRAP
    int x_int = floating_to_fixed(x);
        cordic(x_int + (1<<(FIXED_FRACTION_BITS-2)), x_int - (1<<(FIXED_FRACTION_BITS-2)), 0, SQRT_MINIMAL, PRECISION, &x_cordic, &y_cordic, &z_cordic);
        return fixed_to_floating(x_cordic) * cordic_sqrt_ratio;
#else
    int extra_data;
    int x_int = floating_to_fixed(sqrt_range_extension_in(x, &extra_data));
    cordic(x_int + (1<<(FIXED_FRACTION_BITS-2)), x_int - (1<<(FIXED_FRACTION_BITS-2)), 0, SQRT_MINIMAL, PRECISION, &x_cordic, &y_cordic, &z_cordic);
    return sqrt_range_extension_out(fixed_to_floating(x_cordic) * cordic_sqrt_ratio, &extra_data);
#endif
}

