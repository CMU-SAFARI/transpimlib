#include "_ldexpf.c"
#include "_quadrants.c"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "UnusedLocalVariable"
#pragma ide diagnostic ignored "UnusedValue"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

/**************************************************************************
 * Basic Usage of Helper Functions:
 *
 *  unsigned int address = fixed_to_address_roundup(input, table_spacing);
 *  return table[address];
 */

// Helper Function
static inline unsigned int float_to_address_roundup(float x, float spacing) {
    return (int) (x * spacing + 0.5);
}

// Helper Function
static inline unsigned int fixed_to_address_roundup(int x, float spacing) {
    return (int) (fixed_to_floating(x) * spacing + 0.5);
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

__host float sin_cos_tan_spacing;

#if SIN_COS_TAN_STORE_IN_WRAM > 0
__host float sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#else
__mram_noinit float sin_cos_tan_table[1 << SIN_COS_TAN_PRECISION];
#endif

// Function
float sinf(float x) {
    int quadrant;
    unsigned int address = fixed_to_address_roundup(sin_cos_tan_in(x, &quadrant), sin_cos_tan_spacing);
    float sin_y = sin_cos_tan_table[address];
    return sin_float_out(sin_y, &quadrant);
}

float cosf(float x) {
    int quadrant;
    unsigned int address = fixed_to_address_roundup(cos_to_sin_in(x, &quadrant), sin_cos_tan_spacing);
    float cos_y = sin_cos_tan_table[address];
    return sin_float_out(cos_y, &quadrant);
}

float tanf(float x) {
    int quadrant;
    unsigned int cos_address = fixed_to_address_roundup(cos_to_sin_in(x, &quadrant), sin_cos_tan_spacing);
    unsigned int sin_address = fixed_to_address_roundup(sin_cos_tan_in(x, &quadrant), sin_cos_tan_spacing);

    float cos_y = sin_cos_tan_table[cos_address];
    float sin_y = sin_cos_tan_table[sin_address];
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

__host float sinh_cosh_tanh_spacing;


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

// Function
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

__host float exp_spacing;

#if EXP_STORE_IN_WRAM > 0
__host float exp_table[1 << EXP_PRECISION];
#else
__mram_noinit float exp_table[1 << EXP_PRECISION];
#endif

// Function
float expf (float x) {
    #ifdef NOWRAP
        int address = float_to_address_roundup(x, exp_spacing);
        return exp_table[address];
    #else
        int extra_data;
        int address = float_to_address_roundup(exp_range_extension_in(x, &extra_data), exp_spacing);
        return exp_range_extension_out(exp_table[address], &extra_data);
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

__host float log_spacing;

#if LOG_STORE_IN_WRAM > 0
__host float log_table[1 << LOG_PRECISION];
#else
__mram_noinit float log_table[1 << LOG_PRECISION];
#endif

// Function
float logf(float x) {
    #ifdef NOWRAP
        int address = float_to_address_roundup(x, log_spacing);
        return log_table[address];
    #else
        int extra_data;
        int address = float_to_address_roundup(log_range_extension_in(x, &extra_data), log_spacing);
        return log_range_extension_out(log_table[address], &extra_data);
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

__host float sqrt_spacing;

#if SQRT_STORE_IN_WRAM > 0
__host float sqrt_table[1 << SQRT_PRECISION];
#else
__mram_noinit float sqrt_table[1 << SQRT_PRECISION];
#endif

// Function
float sqrtf (float x) {
    #ifdef NOWRAP
        int offset_from_zero = float_to_address_roundup(x, sqrt_spacing);
        return sqrt_table[offset_from_zero];
    #else
        int extra_data;
        int offset_from_zero = float_to_address_roundup(sqrt_range_extension_in(x, &extra_data), sqrt_spacing);
        return sqrt_range_extension_out(sqrt_table[offset_from_zero], &extra_data);
    #endif
}
