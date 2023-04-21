#ifndef QUADRANTS
#define QUADRANTS

// Internal definitions
// Quadrants for angles in Cordic calculations.
#define QUAD1 1
#define QUAD2 2
#define QUAD3 3
#define QUAD4 4


#define EXTRA_PRECISION (FIXED_FRACTION_BITS-23)
#define cordic_half_circle (13493038080 >> (32 - FIXED_FRACTION_BITS))
#define cordic_quarter_circle (6746519040 >> (32 - FIXED_FRACTION_BITS))
#define cordic_3q_circle (20239557120 >> (32 - FIXED_FRACTION_BITS))

#ifndef  DIV
#define DIV(x, y) ((int)((((long)x << FIXED_FRACTION_BITS) / y)))
#endif


/*
 *  Input Range: 0 to 2.5 * PI
 *  Output Range: 0 to 0.5 * PI
 *
 *  This can be used the same way as any range extension
 */
static inline int sin_cos_quadrant(int theta, int *quadrant) {
    // printf("theta: %d\n", theta);

    // Determine quadrant of incoming angle, translate to the range 0 - 13176795
    if (theta < cordic_half_circle) {
        if (theta < cordic_quarter_circle) {
            *quadrant = QUAD1;
        }
        else {
            *quadrant = QUAD2;
            theta = cordic_half_circle - theta ;
        }
    }

    else {
        if (theta < cordic_3q_circle)
        {
            *quadrant = QUAD3;
            theta -= cordic_half_circle;
        }
        else if (theta < cordic_half_circle<<1)
        {
            *quadrant = QUAD4;
            theta = (cordic_half_circle<<1) - theta ;
        }
        else {
            theta -= (cordic_half_circle<<1) ;
            *quadrant = QUAD1;
        }
    }
    return theta;
}


// Does all the conversions needed for sin/cos/tan in one go
static inline int sin_cos_tan_in(int x, int *quadrant) {
    return sin_cos_quadrant(x, quadrant);
}

/*
 * Does all the conversions needed for cos in one go
 * but in such a way that from now on you can use a sin lookup table and back-conversion (the quadrant hols the shifting information)
 * This doesn't make a lot of sense for a classical LUT where you can just start from the other end of the address space,
 * but for an ldexpf() based fuzzy LUT it is needed as you can only figure out addresses based of their distance from 0
 */
static inline int cos_to_sin_in(int x, int *quadrant){
    return sin_cos_quadrant(x + cordic_quarter_circle, quadrant);
}


static inline int sin_out(int y, int *quadrant){
    return (*quadrant==QUAD1 || *quadrant==QUAD2) ? y : -y;
}

static inline int cos_out(int x, int *quadrant){
    return (*quadrant==QUAD1 || *quadrant==QUAD4) ? x : -x;
}

static inline int tan_out(int x, int y, int *quadrant){
    int ret = DIV(y, x);
    return (*quadrant==QUAD2 || *quadrant==QUAD4) ? -ret: ret;
}
#endif