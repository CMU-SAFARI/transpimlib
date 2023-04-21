#include "_quadrants_fixed.c"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "UnusedLocalVariable"
#pragma ide diagnostic ignored "UnusedValue"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

/********************************************************************************
 * Basic Usage of Helper Functions:
 *
 *  unsigned int address = fixed_to_address_roundup(input, table_exponent);
 *  return table[address];
 */

// Helper Function
static inline unsigned int fixed_to_address_roundup(int x, int exponent) {
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
__host int sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#else
__mram_noinit int sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#endif

// Functions
int sini(int x) {
    int quadrant;
    unsigned int sin_x_fixed_point_offset = fixed_to_address_roundup(sin_cos_tan_in(x, &quadrant), sin_cos_tan_granularity_exponent);
    int sin_y = sin_cos_tan_table[sin_x_fixed_point_offset];
    return sin_out(sin_y, &quadrant);
}

int cosi(int x) {
    int quadrant;
    unsigned int cos_x_fixed_point_offset = fixed_to_address_roundup(cos_to_sin_in(x, &quadrant), sin_cos_tan_granularity_exponent);
    int cos_y = sin_cos_tan_table[cos_x_fixed_point_offset];
    return sin_out(cos_y, &quadrant);
}

int tani(int x) {
    int quadrant;
    unsigned int cos_x_fixed_point_offset = fixed_to_address_roundup(cos_to_sin_in(x, &quadrant), sin_cos_tan_granularity_exponent);
    unsigned int sin_x_fixed_point_offset = fixed_to_address_roundup(sin_cos_tan_in(x, &quadrant), sin_cos_tan_granularity_exponent);

    int cos_y = sin_cos_tan_table[cos_x_fixed_point_offset];
    int sin_y = sin_cos_tan_table[sin_x_fixed_point_offset];
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
    int offset_from_zero = fixed_to_address_roundup(x, exp_granularity_exponent);
    return exp_table[offset_from_zero];
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
int logi(int x) {
    int offset_from_zero = fixed_to_address_roundup(x, log_granularity_exponent);
    return log_table[offset_from_zero];
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
    int offset_from_zero = fixed_to_address_roundup(x, sqrt_granularity_exponent);
    return sqrt_table[offset_from_zero];
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
        int offset_from_zero = fixed_to_address_roundup(x, cndf_granularity_exponent);
        return cndf_table[offset_from_zero];
    } else {
        int offset_from_zero = fixed_to_address_roundup(-x, cndf_granularity_exponent);
        return (1 << (FIXED_FRACTION_BITS - 1)) -  cndf_table[offset_from_zero];
    }
}
