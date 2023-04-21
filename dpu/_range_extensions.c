#include "_ldexpf.c"
#include "stdio.h"

#ifndef RANGE_EXTENSIONS
#define RANGE_EXTENSIONS

#define M_PI 3.14159265358979323846
#define M_PI_2 6.2831853071795864769
#define IP_M_2 0.1591549430918953357
#define log2_e 1.44269504089f
#define ln_2 0.69314718056f

/*
 * A general note on the usage of these functions:
 * They generally come in pairs (exception sin/cos/tan) with
 *  - a first function called     ____in
 *  - and the 2nd function called ____out
 *
 * If you now want to use them as a wrapper around your implementation of a transcendental function, you can do something like this:
 *
 * int additional_data;
 * input_with_reduced_range = ____in(input, &additional_data);
 * output_with_reduced_range = your_function(input_with_reduced_range);
 * output = _____out(output_with_reduced_range, &additinal_data);
 *
 */

/*
 *  Input Range: -MAX_FLOAT to +MAX_FLOAT
 *  Output Range: 0 to 2 * PI
 */
static inline float sin_cos_tan_range_extension_in(float x) {
    // Todo: Implement Payne-Hanek Algorithm
    x -= (float) (M_PI_2 * (float) ((int) (x * IP_M_2) - (int) (x < 0)));
    return x;
}


// Helper function needed for exp_range_extension_in()
int floating_to_fixed_exp_helper(float in) {
    unsigned int in_binary = * ( unsigned int * ) &in;

    unsigned int part = (in_binary >> FLOAT_MANTISSA_BITS);
    int current_exponent = (int) part & FLOAT_MAX_EXPONENT;

    // Bit shift mantissa for exponent
    // This if statement is kind of redundant, can't we just shift negative amounts?
    if (FLOAT_ZERO_EXPONENT > current_exponent) {
        in_binary = ((in_binary & FLOAT_MANTISSA_MASK) | FLOAT_OMITTED_BIT) >> (FLOAT_ZERO_EXPONENT - current_exponent);
    } else {
        in_binary = ((in_binary & FLOAT_MANTISSA_MASK) | FLOAT_OMITTED_BIT) << (current_exponent - FLOAT_ZERO_EXPONENT);
    }

    // Make two's complement
    if (part >> FLOAT_EXPONENT_BITS) in_binary = ~in_binary + 0x1;

    return (int) in_binary;
}

// Helper function needed for exp_range_extension_in()
float fixed_to_floating_exp_helper(int in) {
    unsigned int in_binary = * ( unsigned int * ) &in;

    unsigned int sign = in_binary >> (FLOAT_EXPONENT_BITS + FLOAT_MANTISSA_BITS);

    if (sign) in_binary = ~in_binary + 0x1;

    // return 0 directly as bitshift and wait operations would fail with it
    if (in_binary == 0) {
        return 0.0f;
    }

    // Normalize too big exponents
    int exponent = FLOAT_ZERO_EXPONENT - 1;
    while(in_binary > (FLOAT_OMITTED_BIT << 1)) {
        ++exponent;
        in_binary >>= 1;
    }

    // Normalize too small exponents
    while(in_binary < FLOAT_OMITTED_BIT) {
        --exponent;
        in_binary <<= 1;
    }

    // Combine sign, mantissa and exponent
    in_binary += (exponent << FLOAT_MANTISSA_BITS) + (sign << (FLOAT_EXPONENT_BITS + FLOAT_MANTISSA_BITS));

    return  * ( float * ) &in_binary;
}

/*
 *  Input Range: -MAX_FLOAT to +MAX_FLOAT
 *  Output Range: -log(2) to log(2)
 */
static inline float exp_range_extension_in(float x, int *exponent_2pow_integer) {
    int exponent_2pow_int = floating_to_fixed_exp_helper(x * log2_e);
    *exponent_2pow_integer = exponent_2pow_int >> FLOAT_MANTISSA_BITS;
    int exponent_2pow_fractional_part  = exponent_2pow_int - (*exponent_2pow_integer << FLOAT_MANTISSA_BITS);
    return fixed_to_floating_exp_helper(exponent_2pow_fractional_part) * ln_2;
}

// Inverse of exp_range_extension_in()
static inline float exp_range_extension_out(float y, int *exponent_2pow_integer){
    return  ldexpf(y, *exponent_2pow_integer);
}


/*
 *  Input Range: 0 to +MAX_FLOAT
 *  Output Range: 0.5 to 2
 */
static inline float sqrt_range_extension_in(float x, int *exponent_even){
    float ret = frexpf(x, exponent_even);

    // We check for an uneven exponent, and need to change it to even so that halfing the exponent doesn't break later
    if (*exponent_even & 0x1) {
        ret = ldexpf(ret, 1);
        *exponent_even -= 1;
    }

    return ret;
}

// Inverse of sqrt_range_extension_in()
static inline float sqrt_range_extension_out(float y, int *exponent_even){
    return ldexpf(y, *exponent_even * 0.5);
}


/*
 *  Input Range: 0 to +MAX_FLOAT
 *  Output Range: 0.5 to 1
 */
static inline float log_range_extension_in(float x, int *exponent){
    return frexpf(x, exponent);
}

// Inverse of log_range_extension_in()
static inline float log_range_extension_out(float y, int *exponent){
    return y + *exponent * ln_2;
}

#endif