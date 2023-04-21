#include <stdio.h>
#include "_ldexpf.c"
#include "_quadrants.c"

#ifndef PRECISION
#define PRECISION 10 // This needs to match on CPU and DPU side!
#endif

// Values needed to support different float sizes
#define FLOAT_MANTISSA_BITS 23
#define FLOAT_EXPONENT_MAKS 0xFF

// Helper Function
int float_to_address(float in, int precision, int mantissa_size, int min_exponent){
    unsigned int in_binary;
    in_binary = * (  unsigned int  * ) &in;

    int current_exponent = (in_binary >> FLOAT_MANTISSA_BITS) & FLOAT_EXPONENT_MAKS; // Get the Exponent for int32

    if (current_exponent < (127 + min_exponent)) {
        return (int) ldexpf(in, mantissa_size - min_exponent);
    } else if (current_exponent > (125 + min_exponent + (1 << (precision - mantissa_size)))) {
        return ((1 << precision) - 1);
    }

    return ((current_exponent - (126 + min_exponent)) << mantissa_size) + (in_binary >> (FLOAT_MANTISSA_BITS - mantissa_size) & ((1 << mantissa_size) - 1));
}


// Helper Function
float address_to_float(unsigned int in, int precision, int mantissa_size, int min_exponent){
    int exponent = (int) (in >> mantissa_size) & FLOAT_EXPONENT_MAKS;

    if (in < (1 << mantissa_size)) {
        return ldexpf(in, min_exponent - mantissa_size);
    }

    unsigned int ret = (((126 + min_exponent) + exponent) << FLOAT_MANTISSA_BITS) + ((in & ((1 << mantissa_size) - 1)) << (FLOAT_MANTISSA_BITS - mantissa_size));
    return * (  float  * ) &ret;
}

// Helper Function
float float_to_diff(float x, unsigned int in, int precision, int mantissa_size, int min_exponent){
    int exponent = (int) (in >> mantissa_size) & FLOAT_EXPONENT_MAKS - 1;

    if (in < (1 << mantissa_size)) {
        return ldexpf(x, mantissa_size - min_exponent) - (float) in;
    }

    if (exponent >= ((1 << (precision - mantissa_size)) - 2)) {
        return 0.0f;
    }

    return ldexpf(x - address_to_float(in, precision, mantissa_size, min_exponent), -exponent - min_exponent + mantissa_size);
}


/***********************************************************
*   TANH
*/

// Address generation function parameters
#define TANH_PRECISION PRECISION // This needs to match on CPU and DPU side!
#define TANH_MANTISSA_SIZE (PRECISION - 3) // This needs to match on CPU and DPU side!
#define TANH_MIN_EXPONENT -4 // This needs to match on CPU and DPU side!

// Storage
#ifndef TANH_STORE_IN_WRAM
#define TANH_STORE_IN_WRAM 0
#endif

#if TANH_STORE_IN_WRAM > 0
__host float tanh_table[1 << TANH_PRECISION];
#else
__mram_noinit float tanh_table[1 << TANH_PRECISION];
#endif

// Function
float tanhf(float x) {
    if (x >= 0) {
        int address = float_to_address(x, TANH_PRECISION, TANH_MANTISSA_SIZE, TANH_MIN_EXPONENT);
        float base = (address >= 0) ? tanh_table[address]: 0.0f;
        return base + (tanh_table[address + 1] - base) * float_to_diff(x, address, TANH_PRECISION, TANH_MANTISSA_SIZE, TANH_MIN_EXPONENT);
    } else {
        int address = float_to_address(-x, TANH_PRECISION, TANH_MANTISSA_SIZE, TANH_MIN_EXPONENT);
        float base = (address >= 0) ? tanh_table[address]: 0.0f;
        return - base - (tanh_table[address + 1] - base) * float_to_diff(-x, address, TANH_PRECISION, TANH_MANTISSA_SIZE, TANH_MIN_EXPONENT);
    }
}

/***********************************************************
*   GELU
*/

// Address generation function parameters
#define GELU_PRECISION (PRECISION - 1) // This needs to match on CPU and DPU side!
#define GELU_MANTISSA_SIZE (PRECISION - 4) // This needs to match on CPU and DPU side!
#define GELU_MIN_EXPONENT -4 // This needs to match on CPU and DPU side!

// Storage
#ifndef GELU_STORE_IN_WRAM
#define GELU_STORE_IN_WRAM 0
#endif

#if GELU_STORE_IN_WRAM > 0
__host float gelu_table_p[1 << GELU_PRECISION];
__host float gelu_table_n[1 << GELU_PRECISION];
#else
__mram_noinit float gelu_table_p[1 << GELU_PRECISION];
__mram_noinit float gelu_table_n[1 << GELU_PRECISION];
#endif

// Function
float gelu(float x) {
    if (x > 0) {
        int address = float_to_address(x, GELU_PRECISION, GELU_MANTISSA_SIZE,GELU_MIN_EXPONENT);
        float base = (address >= 0) ? gelu_table_p[address]: 0.0f;
        if (address == (1 << GELU_PRECISION) - 1) {
            return x;
        }
        return base + (gelu_table_p[address + 1] - base) * float_to_diff(x, address,  GELU_PRECISION, GELU_MANTISSA_SIZE, GELU_MIN_EXPONENT);
    } else {
        int address = float_to_address(-x, GELU_PRECISION, GELU_MANTISSA_SIZE,GELU_MIN_EXPONENT);
        float base = (address >= 0) ? gelu_table_n[address]: 0.0f;
        return base + (gelu_table_n[address + 1] - base) * float_to_diff(-x, address,  GELU_PRECISION, GELU_MANTISSA_SIZE, GELU_MIN_EXPONENT);
    }
}

/***********************************************************
*   SIN
*/

// Address generation function parameters
#define SIN_COS_TAN_PRECISION PRECISION // This needs to match on CPU and DPU side!
#define SIN_COS_TAN_MANTISSA_SIZE (PRECISION - 3) // This needs to match on CPU and DPU side!
#define SIN_COS_TAN_MIN_EXPONENT -6 // This needs to match on CPU and DPU side!

// Storage
#ifndef SIN_COS_TAN_STORE_IN_WRAM
#define SIN_COS_TAN_STORE_IN_WRAM 0
#endif

#if SIN_COS_TAN_STORE_IN_WRAM > 0
__host float sin_table[1 << SIN_COS_TAN_PRECISION];
#else
__mram_noinit float sin_table[1 << SIN_COS_TAN_PRECISION];
#endif

// Function
float sinf(float x) {
  int quadrant;
  float x_quadrant = sin_cos_tan_in_float(x, &quadrant);
  int address = float_to_address(x_quadrant, SIN_COS_TAN_PRECISION, SIN_COS_TAN_MANTISSA_SIZE, SIN_COS_TAN_MIN_EXPONENT);
  float base = (address >= 0) ? sin_table[address]: 0.0f;
  return sin_float_out(base + (sin_table[address + 1] - base) * float_to_diff(x_quadrant, address, SIN_COS_TAN_PRECISION, SIN_COS_TAN_MANTISSA_SIZE, SIN_COS_TAN_MIN_EXPONENT), &quadrant);
}