#ifndef LDEXPF
#define LDEXPF

#define FLOAT_TOTAL_BITS 32
#define FLOAT_MANTISSA_BITS 23
#define FLOAT_EXPONENT_BITS 8
#define FLOAT_ZERO_EXPONENT 127

#define EXTRA_PRECISION 5
#define FIXED_FRACTION_BITS (FLOAT_MANTISSA_BITS + EXTRA_PRECISION)

#define FLOAT_MAX_EXPONENT ((1 << FLOAT_EXPONENT_BITS) - 1) // 0xFF ( 255) for 32bit IEEE 754 float

#define FLOAT_SIGN_MASK ((unsigned int) 1 << (FLOAT_MANTISSA_BITS + FLOAT_EXPONENT_BITS)) // 0x80000000 for 32bit IEEE 754 float
#define FLOAT_MANTISSA_MASK  ((1 << FLOAT_MANTISSA_BITS) - 1) // 0x007FFFFF for 32bit IEEE 754 float
#define FLOAT_EXPONENT_MASK (FLOAT_SIGN_MASK - 1 - FLOAT_MANTISSA_MASK) // 0x7F80000 for 32bit IEEE 754 float
#define FLOAT_EXPONENT_AND_MANTISSA_MASK (FLOAT_EXPONENT_MASK | FLOAT_MANTISSA_MASK) // 0x7FFFFFF for 32bit IEEE 754 float
#define FLOAT_OMITTED_BIT (1 << FLOAT_MANTISSA_BITS)  // 0x00800000 for 32bit IEEE 754 float

#define MAXFLOAT_MASK_OR (FLOAT_EXPONENT_AND_MANTISSA_MASK - FLOAT_OMITTED_BIT) // 0x7F7FFFFF for 32bit IEEE 754 float
#define MAXFLOAT_MASK_AND (MAXFLOAT_MASK_OR + FLOAT_SIGN_MASK) // 0xFF7FFFFF for 32bit IEEE 754 float


/*
 * Implementation of the LDEXPF library function as defined in
 * https://en.cppreference.com/w/c/numeric/math/ldexp
 *
 * TLDR: returns in * 2 ^ exponent
 */
float ldexpf(float in, int exponent) {
    unsigned int in_binary = * ( unsigned int * ) &in;
    // Any time we use bitshifts in this code, we want the logical bitshift and not the mathematical one,
    // as such numbers should generally be defined as unsigned int

    //printBits(4, &in_binary);

    int current_exponent = (int) (in_binary >> FLOAT_MANTISSA_BITS) & 0xFF;// Get the Exponent for int32

    // For all the inputs which are denormalized, we need to do some extra logic
    if (current_exponent == 0) {
        // First we check that the number isn't 0, otherwise we just return
        if ((in_binary & FLOAT_EXPONENT_AND_MANTISSA_MASK) != 0) {
            // In the case that we want to make our number even smaller the operation is easy,
            // we just keep the sign bit in place and shift the mantissa right
            if (exponent <= 0) {
                in_binary = ((in_binary & FLOAT_EXPONENT_AND_MANTISSA_MASK) >> (-exponent)) + (in_binary & FLOAT_SIGN_MASK);
            }
            // Now this is the hard one, from our denormalized float with left shifting we might reach a denormalized float,
            // normalized float, or even a MAX_FLOAT! We first shift our mantissa until we know where the first 1 is at
            // and keep updating our desired exponent change, and then we can decide what it ends up being with that
            else {
                unsigned int helper = in_binary;
                while((helper & FLOAT_EXPONENT_MASK) == 0) {
                    --exponent;
                    helper <<= 1;
                }
                // In case the exponent is to big we get a maxint again
                if (current_exponent + exponent >= FLOAT_MAX_EXPONENT){
                    in_binary = (in_binary | MAXFLOAT_MASK_OR) & MAXFLOAT_MASK_AND ;
                }
                // If the number is still bigger than 0 we get a normalized float
                else if (exponent > 0) {
                    helper = helper << 1;
                    in_binary = helper + ((exponent - 1) << FLOAT_MANTISSA_BITS) + (in_binary & FLOAT_SIGN_MASK);
                }

                // Finally the case where the output will also be a denormalized float, so we shift back to the right place
                else {
                    for (;exponent < 0; exponent++) {
                        helper >>= 1;
                    }
                    in_binary = helper + (in_binary & FLOAT_SIGN_MASK);
                }
            }
        }
    }
    // If our exponents are to big we have to return a MAX_FLOAT
    else if (current_exponent + exponent >= FLOAT_MAX_EXPONENT){
        // MAX Int
        in_binary = (in_binary | MAXFLOAT_MASK_OR) & MAXFLOAT_MASK_AND ;
    }
    // If our exponents are to small we have to return a denormalized float
    else if (current_exponent + exponent  < 1) {
        // Denormalized
        in_binary = ((((in_binary | FLOAT_OMITTED_BIT) & FLOAT_EXPONENT_AND_MANTISSA_MASK) >> (-exponent)) + (in_binary & FLOAT_SIGN_MASK));
    }
    // If we made it all the way down here there is nothing fancy going on, and we can just add to the exponent
    else {
        in_binary += ((unsigned int) exponent) << FLOAT_MANTISSA_BITS;
    }

    // printBits(4, &in_binary);

    return * ( float * ) &in_binary;
}

/*
 * Custom Conversion from floating point to fixed point notation
 * Does the same as if you would call (int) ldexpf(in, 23)
 *
 * Fixed point notation uses FIXED_FRACTION_BITS bits after the comma in 2s complement
 * E.g. 1 with 23 FIXED_FRACTION_BITS would be 0000 0000 1000 0000 0000 0000 0000 0000
 *                                                        | < --- after comma  --- > |
 *
 * Values representable depends on the FIXED_FRACTION_BITS:
 * 23: +511.99999988079 and -512
 * 29:   +7.999999998   and   -8
 *
 * For Numbers between 1 and -1 there can be significant loss of precision!
 *
 * Behaviour at inputs outside that range is undefined
 */
int floating_to_fixed(float in) {
    unsigned int in_binary = * ( unsigned int * ) &in;

    unsigned int part = (in_binary >> FLOAT_MANTISSA_BITS);
    int current_exponent = (int) part & FLOAT_MAX_EXPONENT;

    // Bit shift mantissa for exponent
    // This if statement is kind of redundant, cant we just shift negative amounts?
    if (FLOAT_ZERO_EXPONENT - EXTRA_PRECISION > current_exponent) {
        in_binary = ((in_binary & FLOAT_MANTISSA_MASK) | FLOAT_OMITTED_BIT) >> (FLOAT_ZERO_EXPONENT - current_exponent - EXTRA_PRECISION);
    } else {
        in_binary = ((in_binary & FLOAT_MANTISSA_MASK) | FLOAT_OMITTED_BIT) << (current_exponent - FLOAT_ZERO_EXPONENT + EXTRA_PRECISION);
    }

    // Make two's complement
    if (part >> FLOAT_EXPONENT_BITS) in_binary = ~in_binary + 0x1;

    return (int) in_binary;
}

/*
 * Custom Conversion from back from fixed point to floating point
 * Does the same as if you would call (float) ldexpf(in, -23)
 *
 * Fixed point notation uses FIXED_FRACTION_BITS bits after the comma in 2s complement
 *
 * always returns back a valid float but there might be sight loss of precision
 */
float fixed_to_floating(int in) {
    unsigned int in_binary = * ( unsigned int * ) &in;

    unsigned int sign = in_binary >> (FLOAT_EXPONENT_BITS + FLOAT_MANTISSA_BITS);

    if (sign) in_binary = ~in_binary + 0x1;

    // return 0 directly as bitshift and wait operations would fail with it
    if (in_binary == 0) {
        return 0.0f;
    }

    // Normalize too big exponents
    int exponent = FLOAT_ZERO_EXPONENT - 1 - EXTRA_PRECISION;
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
 * Another standard math library function:
 * https://en.cppreference.com/w/c/numeric/math/frexp
 *
 * TLDR: Extracts the exponent from a float so that it is in the 0.5 - 1 range.
 */
float frexpf(float in, int*exponent){
    unsigned int in_binary = * ( unsigned int * ) &in;

    unsigned int part = (in_binary >> FLOAT_MANTISSA_BITS);
    int current_exponent = (int) part & FLOAT_MAX_EXPONENT;

    *exponent = (current_exponent - FLOAT_ZERO_EXPONENT);

    return ldexpf(in, -(*exponent));
}
#endif