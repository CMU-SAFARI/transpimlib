#include "_quadrants_fixed.c"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "UnusedLocalVariable"
#pragma ide diagnostic ignored "UnusedValue"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

// Looking at https://www.fefe.de/intof.html, casting the first part to long to not lose precision, then shifting and casting back
#ifndef  MULT
#define MULT(x, y) ((int)(((long)x * y) >> FIXED_FRACTION_BITS))
#endif

/******************************************************************************************************************
 * Basic Usage of Helper Functions:
 * *
 * unsigned int lower_address = fixed_to_address(x_fixed_point_offset, granularity_exponent);
 * int diff = fixed_to_diff(x_fixed_point_offset, granularity_exponent);
 * int base = table[lower_address];
 * return base + (table[lower_address + 1] - base) * diff;
 */

//Helper Function
static inline unsigned int fixed_to_address(unsigned int x, int exponent) {
    return x >> (FIXED_FRACTION_BITS + exponent);
}

//Helper Function
static inline int fixed_to_diff(unsigned int x, int exponent) {
    return (x & ((1<<(FIXED_FRACTION_BITS + exponent))-1)) << -exponent;
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
__host int sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#else
__mram_noinit int sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#endif

// Functions
int sini(int x) {
    int quadrant;
    unsigned int x_fixed_point_offset = sin_cos_tan_in(x, &quadrant);
    unsigned int offset_addr_down = fixed_to_address(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    int diff = fixed_to_diff(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    int base = sin_cos_tan_table[offset_addr_down];
    int y = base +  MULT((sin_cos_tan_table[offset_addr_down + 1] - base), diff);
    return sin_out(y, &quadrant);
}

int cosi(int x) {
    int quadrant;
    unsigned int x_fixed_point_offset = cos_to_sin_in(x, &quadrant);
    unsigned int offset_addr_down = fixed_to_address(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    int diff = fixed_to_diff(x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    int base = sin_cos_tan_table[offset_addr_down];
    int y = base +  MULT((sin_cos_tan_table[offset_addr_down + 1] - base), diff);
    return sin_out(y, &quadrant);
}

int tani(int x) {
    int quadrant;

    unsigned int cos_x_fixed_point_offset = cos_to_sin_in(x, &quadrant);
    unsigned int cos_addr_down = fixed_to_address(cos_x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    unsigned int sin_x_fixed_point_offset = sin_cos_tan_in(x, &quadrant);
    unsigned int sin_addr_down =  fixed_to_address(sin_x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    int cos_diff = fixed_to_diff(cos_x_fixed_point_offset, sin_cos_tan_granularity_exponent);
    int sin_diff = fixed_to_diff(sin_x_fixed_point_offset, sin_cos_tan_granularity_exponent);

    int cos_base = sin_cos_tan_table[cos_addr_down];
    int cos_y = cos_base +  MULT((sin_cos_tan_table[cos_addr_down + 1] - cos_base), cos_diff);
    int sin_base = sin_cos_tan_table[sin_addr_down];
    int sin_y = sin_base +  MULT((sin_cos_tan_table[sin_addr_down + 1] - sin_base), sin_diff);

    return tan_out(cos_y, sin_y, &quadrant);
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
__host int sinh_table[1 << SINH_COSH_TANH_PRECISION];
#else
__mram_noinit int sinh_table[1 << SINH_COSH_TANH_PRECISION];
#endif

#if SINH_COSH_TANH_STORE_IN_WRAM > 0
__host int cosh_table[1 << SINH_COSH_TANH_PRECISION];
#else
__mram_noinit int cosh_table[1 << SINH_COSH_TANH_PRECISION];
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
__host int exp_table[1 << EXP_PRECISION];
#else
__mram_noinit int exp_table[1 << EXP_PRECISION];
#endif

// Functions
int expi (int x) {
    unsigned int lower_address = fixed_to_address(x, exp_granularity_exponent);
    int diff = fixed_to_diff(x, exp_granularity_exponent);
    int base = exp_table[lower_address];
    return base + MULT((exp_table[lower_address + 1] - base), diff);
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
__host int log_table[1 << LOG_PRECISION];
#else
__mram_noinit int log_table[1 << LOG_PRECISION];
#endif

// Function
int logi (int x) {
    unsigned int lower_address = fixed_to_address(x, log_granularity_exponent);
    int diff = fixed_to_diff(x, log_granularity_exponent);
    int base = log_table[lower_address];
    return base + MULT((log_table[lower_address + 1] - base), diff);
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
__host int sqrt_table[1 << SQRT_PRECISION];
#else
__mram_noinit int sqrt_table[1 << SQRT_PRECISION];
#endif

// Function
int sqrti (int x) {
    unsigned int lower_address = fixed_to_address(x, sqrt_granularity_exponent);
    int diff = fixed_to_diff(x, sqrt_granularity_exponent);
    int base = sqrt_table[lower_address];
    return base + MULT((sqrt_table[lower_address + 1] - base), diff);
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
int cndfi (int x) {
    if (x > 0) {
        unsigned int lower_address = fixed_to_address(x, cndf_granularity_exponent);
        int diff = fixed_to_diff(x, cndf_granularity_exponent);
        int base = cndf_table[lower_address];
        return base + MULT((cndf_table[lower_address + 1] - base), diff);
    } else {
        unsigned int lower_address = fixed_to_address(-x, cndf_granularity_exponent);
        int diff = fixed_to_diff(x, cndf_granularity_exponent);
        int base = cndf_table[lower_address];
        return (1 << (FIXED_FRACTION_BITS - 1)) - base - MULT((cndf_table[lower_address + 1] - base), diff);
    }
}