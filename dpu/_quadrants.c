#include "_range_extensions.c"
#include "_ldexpf.c"

#ifndef QUADRANTS
#define QUADRANTS

// Internal definitions
// Quadrants for angles in Cordic calculations.
#define QUAD1 1
#define QUAD2 2
#define QUAD3 3
#define QUAD4 4

#define cordic_half_circle (26353590 << EXTRA_PRECISION)
#define cordic_quarter_circle (13176795 << EXTRA_PRECISION)
#define cordic_3q_circle (39530385 << EXTRA_PRECISION)


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

/*
 *  Input Range: 0 to 2.5 * PI
 *  Output Range: 0 to 0.5 * PI
 *
 *  Float to Float version
 */
static inline float sin_cos_quadrant_float(float theta, int *quadrant) {
  // printf("theta: %d\n", theta);

  // Determine quadrant of incoming angle, translate to the range 0 - 13176795
  if (theta < M_PI) {
    if (theta < M_PI * 0.5) {
      *quadrant = QUAD1;
    }
    else {
      *quadrant = QUAD2;
      theta = M_PI - theta ;
    }
  }

  else {
    if (theta < (M_PI * 1.5))
    {
      *quadrant = QUAD3;
      theta -= M_PI;
    }
    else if (theta < M_PI * 2)
    {
      *quadrant = QUAD4;
      theta = (M_PI * 2) - theta ;
    }
    else {
      theta -= (M_PI * 2) ;
      *quadrant = QUAD1;
    }
  }
  return theta;
}

// Does all the conversions needed for sin/cos/tan while keeping it a float
static inline float sin_cos_tan_in_float(float x, int *quadrant){
#ifdef NOWRAP
  return sin_cos_quadrant_float(x, quadrant);
#else
  return sin_cos_quadrant_float(sin_cos_tan_range_extension_in(x), quadrant);
#endif
}

// Does all the conversions needed for sin/cos/tan in one go
static inline int sin_cos_tan_in(float x, int *quadrant){
    #ifdef NOWRAP
        return sin_cos_quadrant(floating_to_fixed(x), quadrant);
    #else
        return sin_cos_quadrant(floating_to_fixed(sin_cos_tan_range_extension_in(x)), quadrant);
    #endif
}

/*
 * Does all the conversions needed for cos in one go
 * but in such a way that from now on you can use a sin lookup table and back-conversion (the quadrant hols the shifting information)
 * This doesn't make a lot of sense for a classical LUT where you can just start from the other end of the address space,
 * but for an ldexpf() based fuzzy LUT it is needed as you can only figure out addresses based of their distance from 0
 */
static inline int cos_to_sin_in(float x, int *quadrant){
    #ifdef NOWRAP
        return sin_cos_quadrant(floating_to_fixed(x) + cordic_quarter_circle, quadrant);
    #else
        return sin_cos_quadrant(floating_to_fixed(sin_cos_tan_range_extension_in(x)) + cordic_quarter_circle, quadrant);
    #endif
}


static inline float sin_out(int y, int *quadrant){
    return fixed_to_floating((*quadrant==QUAD1 || *quadrant==QUAD2) ? y : -y);
}

static inline float cos_out(int x, int *quadrant){
    return fixed_to_floating((*quadrant==QUAD1 || *quadrant==QUAD4) ? x : -x);
}

static inline float tan_out(int x, int y, int *quadrant){
    float ret = (float) y  / (float) x;
    return (*quadrant==QUAD2 || *quadrant==QUAD4) ? - ret: ret;
}

static inline float sin_float_out(float y, int *quadrant){
    return (*quadrant==QUAD1 || *quadrant==QUAD2) ? y : -y;
}

static inline float tan_float_out(float x, float y, int *quadrant){
    float ret = (float) y  / (float) x;
    return (*quadrant==QUAD2 || *quadrant==QUAD4) ? - ret : ret;
}

#endif