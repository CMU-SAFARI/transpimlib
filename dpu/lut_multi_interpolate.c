#include "_ldexpf.c"
#include "_quadrants.c"

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "UnusedLocalVariable"
#pragma ide diagnostic ignored "UnusedValue"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

/***********************************************************************************************************
 * Basic Usage of Helper Functions:
 *
 * float address_with_decimals = float_to_roughaddress(x, table_spacing);
 * int lower_address = (int) address_with_decimals;
 * float base = table[lower_address];
 * return base + (table[lower_address + 1] - base) * (address_with_decimals - (float) lower_address);
 */

// Helper Function
static inline float float_to_roughaddress(float x, float spacing) {
    return x * spacing;
}

// Helper Function
static inline float fixed_to_roughaddress(int x, float spacing) {
    return (fixed_to_floating(x) * spacing);
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
    unsigned int x_fixed_point_offset = sin_cos_tan_in(x, &quadrant);
    float offset_float = fixed_to_roughaddress(x_fixed_point_offset, sin_cos_tan_spacing);
    int address = (int) offset_float;

    float base = sin_cos_tan_table[address];
    float y = base + (sin_cos_tan_table[address + 1] - base) * (offset_float - (float) address);
    return sin_float_out(y, &quadrant);
}

float cosf(float x) {
    int quadrant;
    unsigned int x_fixed_point_offset = cos_to_sin_in(x, &quadrant);
    float offset_float = fixed_to_roughaddress(x_fixed_point_offset, sin_cos_tan_spacing);
    int address = (int) offset_float;

    float base = sin_cos_tan_table[address];
    float y = base + (sin_cos_tan_table[address + 1] - base) * (offset_float - (float) address);
    return sin_float_out(y, &quadrant);
}

float tanf(float x) {
    int quadrant;

    unsigned int cos_x_fixed_point_offset = cos_to_sin_in(x, &quadrant);
    float cos_offset_float= fixed_to_roughaddress(cos_x_fixed_point_offset, sin_cos_tan_spacing);
    int cos_offset_addr_down = (int) cos_offset_float;

    unsigned int sin_x_fixed_point_offset = sin_cos_tan_in(x, &quadrant);
    float sin_offset_float = fixed_to_roughaddress(sin_x_fixed_point_offset, sin_cos_tan_spacing);
    int sin_offset_addr_down = (int) sin_offset_float;

    float cos_base = sin_cos_tan_table[cos_offset_addr_down];
    float cos_y = cos_base + (sin_cos_tan_table[cos_offset_addr_down + 1] - cos_base) * (cos_offset_float - (float) cos_offset_addr_down);

    float sin_base = sin_cos_tan_table[sin_offset_addr_down];
    float sin_y = sin_base + (sin_cos_tan_table[sin_offset_addr_down + 1] - sin_base) * (sin_offset_float - (float) sin_offset_addr_down);

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
        float offset_float = float_to_roughaddress(x, exp_spacing);
    #else
        int extra_data;
        float offset_float = float_to_roughaddress(exp_range_extension_in(x, &extra_data), exp_spacing);
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

__host float log_spacing;

#if LOG_STORE_IN_WRAM > 0
__host float log_table[1 << LOG_PRECISION];
#else
__mram_noinit float log_table[1 << LOG_PRECISION];
#endif

// Function
float logf (float x) {
    #ifdef NOWRAP
        float offset_float = float_to_roughaddress(x, log_spacing);
    #else
        int extra_data;
        float offset_float = float_to_roughaddress(log_range_extension_in(x, &extra_data), log_spacing);
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

__host float sqrt_spacing;

#if SQRT_STORE_IN_WRAM > 0
__host float sqrt_table[1 << SQRT_PRECISION];
#else
__mram_noinit float sqrt_table[1 << SQRT_PRECISION];
#endif

// Function
float sqrtf (float x) {
    #ifdef NOWRAP
        float offset_float = float_to_roughaddress(x, sqrt_spacing);
    #else
        int extra_data;
        float offset_float = float_to_roughaddress(sqrt_range_extension_in(x, &extra_data), sqrt_spacing);
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

__host float cndf_spacing;

#if CNDF_STORE_IN_WRAM > 0
__host int cndf_table[1 << SQRT_PRECISION];
#else
__mram_noinit int cndf_table[1 << SQRT_PRECISION];
#endif

// Function
float cndf (float x) {
    if (x > 0) {
        float address_with_decimals = float_to_roughaddress(x, cndf_spacing);
        int lower_address = (int) address_with_decimals;
        float base = cndf_table[lower_address];
        return 0.5f + base + (cndf_table[lower_address + 1] - base) * (address_with_decimals - (float) lower_address);
    } else {
        float address_with_decimals = float_to_roughaddress(-x, cndf_spacing);
        int lower_address = (int) address_with_decimals;
        float base = cndf_table[lower_address];
        return 0.5f - base - (cndf_table[lower_address + 1] - base) * (address_with_decimals - (float) lower_address);
    }
}
