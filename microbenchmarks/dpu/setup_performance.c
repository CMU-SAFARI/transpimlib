#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>

// Include the right method
#ifdef CORDIC
    #include "../../dpu/cordic.c"
#elif defined CORDIC_LUT
    #include "../../dpu/cordic_lut.c"
#elif defined LUT_LDEXPF
    #include "../../dpu/lut_ldexpf.c"
#elif defined LUT_LDEXPF_INTERPOLATE
    #include "../../dpu/lut_ldexpf_interpolate.c"
#elif defined LUT_MULTI
    #include "../../dpu/lut_multi.c"
#elif defined LUT_MULTI_INTERPOLATE
    #include "../../dpu/lut_multi_interpolate.c"
#elif defined LUT_DIRECT
    #include "../../dpu/lut_direct.c"
#elif defined LUT_DIRECT_LDEXPF
    #include "../../dpu/lut_direct_ldexpf.c"
#endif

int main(){
    // Do something so that imports actually get used
    #ifdef LUT_DIRECT
        float x = tanhf(0.5);
    #elif defined LUT_DIRECT_LDEXPF
        float x = tanhf(0.5);
    #else
        float x = sinf(0.5);
    #endif
    return 0;
}