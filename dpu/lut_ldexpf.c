#include "_ldexpf.c"
#include "_quadrants.c"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "UnusedLocalVariable"
#pragma ide diagnostic ignored "UnusedValue"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

/********************************************************************************
 * Basic Usage of Helper Functions:
 *
 *  unsigned int address = float_to_address_roundup_ldexpf(input, table_exponent);
 *  return table[address];
 */

// Helper Function
static inline unsigned int float_to_address_roundup_ldexpf(float x, int exponent) {
    return (((int) ldexpf(x, - exponent + 1)) + 1) >> 1;
}

// Helper Function
static inline unsigned int fixed_to_address_roundup_ldexpf(int x, int exponent) {
    return ((x >> (FIXED_FRACTION_BITS + exponent - 1)) + 1) >> 1;
}


/***********************************************************
*   SIN / COS / TAN
*/

// Address generation function parameters
#define SIN_COS_TAN_PRECISION PRECISION // This needs to match on CPU and DPU side!

// Storage
#ifndef SIN_COS_TAN_STORE_IN_WRAM
#define SIN_COS_TAN_STORE_IN_WRAM 0
#endif

__host int sin_cos_tan_granularity_exponent;

#if SIN_COS_TAN_STORE_IN_WRAM > 0
__host float sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#else
__mram_noinit float sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#endif

// Functions
float sinf(float x) {
    int quadrant;
    unsigned int sin_x_fixed_point_offset = fixed_to_address_roundup_ldexpf(sin_cos_tan_in(x, &quadrant), sin_cos_tan_granularity_exponent);
    float sin_y = sin_cos_tan_table[sin_x_fixed_point_offset];
    return sin_float_out(sin_y, &quadrant);
}

float cosf(float x) {
    int quadrant;
    unsigned int cos_x_fixed_point_offset = fixed_to_address_roundup_ldexpf(cos_to_sin_in(x, &quadrant), sin_cos_tan_granularity_exponent);
    float cos_y = sin_cos_tan_table[cos_x_fixed_point_offset];
    return sin_float_out(cos_y, &quadrant);
}

float tanf(float x) {
    int quadrant;
    unsigned int cos_x_fixed_point_offset = fixed_to_address_roundup_ldexpf(cos_to_sin_in(x, &quadrant), sin_cos_tan_granularity_exponent);
    unsigned int sin_x_fixed_point_offset = fixed_to_address_roundup_ldexpf(sin_cos_tan_in(x, &quadrant), sin_cos_tan_granularity_exponent);

    float cos_y = sin_cos_tan_table[cos_x_fixed_point_offset];
    float sin_y = sin_cos_tan_table[sin_x_fixed_point_offset];
    return tan_float_out(cos_y, sin_y, &quadrant);
}


/***********************************************************
*   SINH / COSH / TANH
*/

// Address generation function parameters
#define SINH_COSH_TANH_PRECISION PRECISION // This needs to match on CPU and DPU side!

// Storage
#ifndef SINH_COSH_TANH_STORE_IN_WRAM
#define SINH_COSH_TANH_STORE_IN_WRAM 0
#endif

__host int sinh_cosh_tanh_granularity_exponent;

#if SINH_COSH_TANH_STORE_IN_WRAM > 0
__host float sinh_table[1 << SINH_COSH_TANH_PRECISION];
#else
__mram_noinit float sinh_table[1 << SINH_COSH_TANH_PRECISION];
#endif

#if SINH_COSH_TANH_STORE_IN_WRAM > 0
__host float cosh_table[1 << SINH_COSH_TANH_PRECISION];
#else
__mram_noinit float cosh_table[1 << SINH_COSH_TANH_PRECISION];
#endif

// Functions
// TODO


/***********************************************************
*   EXP
*/

// Address generation function parameters
#define EXP_PRECISION PRECISION // This needs to match on CPU and DPU side!

// Storage
#ifndef EXP_STORE_IN_WRAM
#define EXP_STORE_IN_WRAM 0
#endif

__host int exp_granularity_exponent;

#if EXP_STORE_IN_WRAM > 0
__host float exp_table[1 << EXP_PRECISION];
#else
__mram_noinit float exp_table[1 << EXP_PRECISION];
#endif

// Functions
float expf (float x) {
    #ifdef NOWRAP
        int offset_from_zero = float_to_address_roundup_ldexpf(x, exp_granularity_exponent);
        return exp_table[offset_from_zero];
    #else
        int extra_data;
        int offset_from_zero = float_to_address_roundup_ldexpf(exp_range_extension_in(x, &extra_data), exp_granularity_exponent);
        return exp_range_extension_out(exp_table[offset_from_zero], &extra_data);
    #endif
}


/***********************************************************
*   LOG
*/

// Address generation function parameters
#define LOG_PRECISION PRECISION // This needs to match on CPU and DPU side!

// Storage
#ifndef LOG_STORE_IN_WRAM
#define LOG_STORE_IN_WRAM 0
#endif

__host int log_granularity_exponent;

#if LOG_STORE_IN_WRAM > 0
__host float log_table[1 << LOG_PRECISION];
#else
__mram_noinit float log_table[1 << LOG_PRECISION];
#endif

// Function
float logf(float x) {
    #ifdef NOWRAP
        int offset_from_zero = float_to_address_roundup_ldexpf(x, log_granularity_exponent);
        return log_table[offset_from_zero];
    #else
        int extra_data;
        int offset_from_zero = float_to_address_roundup_ldexpf(log_range_extension_in(x, &extra_data), log_granularity_exponent);
        return log_range_extension_out(log_table[offset_from_zero], &extra_data);
    #endif
}


/***********************************************************
*   SQRT
*/

// Address generation function parameters
#define SQRT_PRECISION PRECISION // This needs to match on CPU and DPU side!

// Storage
#ifndef SQRT_STORE_IN_WRAM
#define SQRT_STORE_IN_WRAM 0
#endif

__host int sqrt_granularity_exponent;

#if SQRT_STORE_IN_WRAM > 0
__host float sqrt_table[1 << SQRT_PRECISION];
#else
__mram_noinit float sqrt_table[1 << SQRT_PRECISION];
#endif

// Function
float sqrtf (float x) {
    #ifdef NOWRAP
        int offset_from_zero = float_to_address_roundup_ldexpf(x, sqrt_granularity_exponent);
        return sqrt_table[offset_from_zero];
    #else
        int extra_data;
        int offset_from_zero = float_to_address_roundup_ldexpf(sqrt_range_extension_in(x, &extra_data), sqrt_granularity_exponent);
        return sqrt_range_extension_out(sqrt_table[offset_from_zero], &extra_data);
    #endif
}
