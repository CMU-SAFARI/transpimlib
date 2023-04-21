#include "_ldexpf.c"
#include "stdio.h"

#ifndef RANGE_EXTENSIONS
#define RANGE_EXTENSIONS

// We save these values as 2^32 * x (the most precise notation), so we can bithift them right for our notation
#define M_PI (13493037705 >> (32 - FIXED_FRACTION_BITS))
#define M_PI_2 (26986075409 >> (32 - FIXED_FRACTION_BITS))
#define IP_M_2 (683565276 >> (32 - FIXED_FRACTION_BITS))
#define log2_e (6196328019 >> (32 - FIXED_FRACTION_BITS))
#define ln_2 (2977044471 >> (32 - FIXED_FRACTION_BITS))

#ifndef  MULT
#define MULT(x, y) ((int)(((long)x * y) >> FIXED_FRACTION_BITS))
#endif

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
    return x % M_PI_2;
}

/*
 *  Input Range: -MAX_FLOAT to +MAX_FLOAT
 *  Output Range: -log(2) to log(2)
 */
static inline float exp_range_extension_in(float x, int *exponent_2pow_integer) {
    int exponent_2pow_int = MULT(x, log2_e);
    *exponent_2pow_integer = exponent_2pow_int >> FIXED_FRACTION_BITS;
    int exponent_2pow_fractional_part  = exponent_2pow_int & ((1 << FIXED_FRACTION_BITS) - 1);
    return MULT(exponent_2pow_fractional_part, ln_2);
}

// Inverse of exp_range_extension_in()
static inline float exp_range_extension_out(float y, int *exponent_2pow_integer){
    return  y << *exponent_2pow_integer;
}


/*
 *  Input Range: 0 to +MAX_FLOAT
 *  Output Range: 0.5 to 2
 */
static inline float sqrt_range_extension_in(float x, int *exponent_even){

    int integer_part = x >> FIXED_FRACTION_BITS

    while(integer_part > 0) {
        ++*exponent_even;
        integer_part >>= 1;
        x >>= 1;
    }

    if (*exponent_even % 2 == 1){
        --*exponent_even;
        x <<= 1;
    }

    return x;
}

// Inverse of sqrt_range_extension_in()
static inline float sqrt_range_extension_out(float y, int *exponent_even){
    return y << (*exponent_even >> 1);
}


/*
 *  Input Range: 0 to +MAX_FLOAT
 *  Output Range: 0.5 to 1
 */
static inline float log_range_extension_in(float x, int *exponent){
    int integer_part = x >> FIXED_FRACTION_BITS

    while(integer_part > 0) {
        ++*exponent;
        integer_part >>= 1;
        x >>= 1;
    }
}

// Inverse of log_range_extension_in()
static inline float log_range_extension_out(float y, int *exponent){
    return y + MULT(*exponent, ln_2);
}

#endif