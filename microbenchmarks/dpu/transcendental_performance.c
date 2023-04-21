#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>
#include <defs.h>

// Only store the required table in WRAM if that is requested
#ifdef COS
    #define SIN_COS_TAN_STORE_IN_WRAM STORE_IN_WRAM
#elif defined TAN
    #define SIN_COS_TAN_STORE_IN_WRAM STORE_IN_WRAM
#elif defined EXP
    #define EXP_STORE_IN_WRAM STORE_IN_WRAM
#elif defined LOG
    #define LOG_STORE_IN_WRAM STORE_IN_WRAM
#elif defined SQRT
    #define SQRT_STORE_IN_WRAM STORE_IN_WRAM
#elif defined TANH
    #define TANH_STORE_IN_WRAM STORE_IN_WRAM
#elif defined GELU
    #define GELU_STORE_IN_WRAM STORE_IN_WRAM
#else
    #define SIN_COS_TAN_STORE_IN_WRAM STORE_IN_WRAM
#endif

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

#define CACHE_SIZE 128
#define BUFFER_SIZE (1 << ARRAY_SIZE)
//#define NR_TASKLETS 16

__mram_noinit float buffer[BUFFER_SIZE];
__host uint32_t performance_count;

int main(){

    performance_count = 0;
    __dma_aligned float local_cache[CACHE_SIZE];

    printf("Main LUT size / CORDIC Iterations:        %d\n", PRECISION);
    printf("Operations Tested:                        %d\n", BUFFER_SIZE);

    float angle, out;
    //for (unsigned int bytes_read = 0; bytes_read < BUFFER_SIZE;) {
    for (unsigned int bytes_read = me() * CACHE_SIZE; bytes_read < BUFFER_SIZE; bytes_read += CACHE_SIZE * NR_TASKLETS){

        mram_read(&buffer[bytes_read], local_cache, sizeof(float) * CACHE_SIZE);

        volatile unsigned int write_pos = bytes_read;

        //for (unsigned int byte_index = 0; (byte_index < CACHE_SIZE) && (bytes_read < BUFFER_SIZE); byte_index++ , bytes_read++) {
        for (unsigned int byte_index = 0; (byte_index < CACHE_SIZE) && (bytes_read + byte_index < BUFFER_SIZE); byte_index++) {

            perfcounter_config(COUNT_CYCLES, true);

            angle = local_cache[byte_index];

            #ifdef COS
                out = cosf(angle);
            #elif defined TAN
                out = tanf(angle);
            #elif defined EXP
                out = expf(angle);
            #elif defined LOG
                out = logf(angle);
            #elif defined SQRT
                out = sqrtf(angle);
            #elif defined TANH
                out = tanhf(angle);
            #elif defined GELU
                out = gelu(angle);
            #else
                out = sinf(angle);
            #endif

            local_cache[byte_index] = out;

            performance_count += perfcounter_get();

            // printf("func(%f) = %f\n", angle, out);
        }

        mram_write(local_cache, &buffer[write_pos], sizeof(float) * CACHE_SIZE);
    }

    return 0;
}
