#include "_ldexpf.c"
#include "_quadrants.c"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "UnusedLocalVariable"
#pragma ide diagnostic ignored "UnusedValue"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

/******************************************************************************************************************
 * Basic Usage of Helper Functions:
 *
 * float address_with_decimals = float_to_roughaddress_ldexpf(x, table_exponent);
 * int lower_address = (int) address_with_decimals;
 * float base = table[lower_address];
 * return base + (table[lower_address + 1] - base) * (address_with_decimals - (float) lower_address);
 *
 * OR
 *
 * unsigned int lower_address = fixed_to_address_ldexpf(x_fixed_point_offset, sin_cos_tan_granularity_exponent);
 * float diff = fixed_to_diff_ldexpf(x_fixed_point_offset, sin_cos_tan_granularity_exponent);
 * float base = table[lower_address];
 * return base + (table[lower_address + 1] - base) * diff;
 */

//Helper Function
static inline float float_to_roughaddress_ldexpf(float x, int exponent) {
    return ldexpf(x, - exponent);
}

//Helper Function
static inline unsigned int fixed_to_address_ldexpf(unsigned int x, int exponent) {
    return x >> (FIXED_FRACTION_BITS + exponent);
}

//Helper Function
static inline float fixed_to_diff_ldexpf(unsigned int x, int exponent) {
    return fixed_to_floating(x << ((FLOAT_TOTAL_BITS - FIXED_FRACTION_BITS) - exponent) >> (FLOAT_TOTAL_BITS - FIXED_FRACTION_BITS));
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
    unsigned int x_fixed_point_offset = sin_cos_tan_in(x, &quadrant);
    unsigned int offset_addr_down = fixed_to_address_ldexpf(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    float diff = fixed_to_diff_ldexpf(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    float base = sin_cos_tan_table[offset_addr_down];
    float y = base + (sin_cos_tan_table[offset_addr_down + 1] - base) * diff;
    return sin_float_out(y, &quadrant);
}

float cosf(float x) {
    int quadrant;
    unsigned int x_fixed_point_offset = cos_to_sin_in(x, &quadrant);
    unsigned int offset_addr_down = fixed_to_address_ldexpf(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    float diff = fixed_to_diff_ldexpf(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    float base = sin_cos_tan_table[offset_addr_down];
    float y = base + (sin_cos_tan_table[offset_addr_down + 1] - base) * diff;
    return sin_float_out(y, &quadrant);
}

float tanf(float x) {
    int quadrant;

    unsigned int cos_x_fixed_point_offset = cos_to_sin_in(x, &quadrant);
    unsigned int cos_addr_down = fixed_to_address_ldexpf(cos_x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    unsigned int sin_x_fixed_point_offset = sin_cos_tan_in(x, &quadrant);
    unsigned int sin_addr_down =  fixed_to_address_ldexpf(sin_x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    float cos_diff = fixed_to_diff_ldexpf(cos_x_fixed_point_offset, sin_cos_tan_granularity_exponent);
    float sin_diff = fixed_to_diff_ldexpf(sin_x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    float cos_base = sin_cos_tan_table[cos_addr_down];
    float cos_y = cos_base + (sin_cos_tan_table[cos_addr_down + 1] - cos_base) * cos_diff;
    float sin_base = sin_cos_tan_table[sin_addr_down];
    float sin_y = sin_base + (sin_cos_tan_table[sin_addr_down + 1] - sin_base) * sin_diff;

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
        float offset_float = float_to_roughaddress_ldexpf(x, exp_granularity_exponent);
    #else
        int extra_data;
        float offset_float = float_to_roughaddress_ldexpf(exp_range_extension_in(x, &extra_data), exp_granularity_exponent);
    #endif

    int offset_addr_down = (int) offset_float;
    float base = exp_table[offset_addr_down];

    #ifdef NOWRAP
        return base + (exp_table[offset_addr_down + 1] - base) * (offset_float - (float) offset_addr_down);
    #else
        return exp_range_extension_out(base + (exp_table[offset_addr_down + 1] - base) * (offset_float - (float) offset_addr_down), &extra_data);
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
float logf (float x) {
    #ifdef NOWRAP
        float offset_float = float_to_roughaddress_ldexpf(x, log_granularity_exponent);
    #else
        int extra_data;
        float offset_float = float_to_roughaddress_ldexpf(log_range_extension_in(x, &extra_data), log_granularity_exponent);
    #endif

    int base_address = (int) offset_float;
    float base = log_table[base_address];

    #ifdef NOWRAP
        return base + (log_table[base_address + 1] - base) * (offset_float - (float) base_address);
    #else
        return log_range_extension_out(base + (log_table[base_address + 1] - base) * (offset_float - (float) base_address), &extra_data);
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
        float offset_float = float_to_roughaddress_ldexpf(x, sqrt_granularity_exponent);
    #else
        int extra_data;
        float offset_float = float_to_roughaddress_ldexpf(sqrt_range_extension_in(x, &extra_data), sqrt_granularity_exponent);
    #endif

    int offset_addr_down = (int) offset_float;
    float base = sqrt_table[offset_addr_down];

    #ifdef NOWRAP
        return base + (sqrt_table[offset_addr_down + 1] - base) * (offset_float - (float) offset_addr_down);
    #else
        return sqrt_range_extension_out(base + (sqrt_table[offset_addr_down + 1] - base) * (offset_float - (float) offset_addr_down), &extra_data);
    #endif
}

/***********************************************************
*   CNDF
*/

// Address generation function parameters
#define CNDF_PRECISION PRECISION // This needs to match on CPU and DPU side!

// Storage
#ifndef CNDF_STORE_IN_WRAM
#define CNDF_STORE_IN_WRAM 0
#endif

__host int cndf_granularity_exponent;

#if CNDF_STORE_IN_WRAM > 0
__host int cndf_table[1 << SQRT_PRECISION];
#else
__mram_noinit int cndf_table[1 << SQRT_PRECISION];
#endif

// Function
float cndf (float x) {
    if (x > 0) {
        float address_with_decimals = float_to_roughaddress_ldexpf(x, cndf_granularity_exponent);
        int lower_address = (int) address_with_decimals;
        float base = cndf_table[lower_address];
        return 0.5f + base + (cndf_table[lower_address + 1] - base) * (address_with_decimals - (float) lower_address);
    } else {
        float address_with_decimals = float_to_roughaddress_ldexpf(-x, cndf_granularity_exponent);
        int lower_address = (int) address_with_decimals;
        float base = cndf_table[lower_address];
        return 0.5f - base - (cndf_table[lower_address + 1] - base) * (address_with_decimals - (float) lower_address);
    }
}
