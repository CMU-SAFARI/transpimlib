#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>

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
#elif defined CNDF
    #define CNDF_STORE_IN_WRAM STORE_IN_WRAM
#else
    #define SIN_COS_TAN_STORE_IN_WRAM STORE_IN_WRAM
#endif

#define FIXED_FRACTION_BITS 24

// Include the right method

#ifdef LUT_FIXED
    #include "../../dpu/lut_fixed.c"
#elif defined LUT_FIXED_INTERPOLATE
    #include "../../dpu/lut_fixed_interpolate.c"
#endif


#define CACHE_SIZE 128
#define BUFFER_SIZE (1 << ARRAY_SIZE)

__mram_noinit int buffer[BUFFER_SIZE];
__host uint32_t performance_count;

int main(){

    performance_count = 0;
    __dma_aligned int local_cache[CACHE_SIZE];

    printf("Main LUT size / CORDIC Iterations:        %d\n", PRECISION);
    printf("Operations Tested:                        %d\n", BUFFER_SIZE);

    int angle, out;
    for (unsigned int bytes_read = 0; bytes_read < BUFFER_SIZE;) {

        mram_read(&buffer[bytes_read], local_cache, sizeof(int) * CACHE_SIZE);

        volatile unsigned int write_pos = bytes_read;

        for (unsigned int byte_index = 0; (byte_index < CACHE_SIZE) && (bytes_read < BUFFER_SIZE); byte_index++ , bytes_read++) {

            perfcounter_config(COUNT_CYCLES, true);

            angle = local_cache[byte_index];

            #ifdef COS
                out = cosi(angle);
            #elif defined TAN
                out = tani(angle);
            #elif defined EXP
                out = expi(angle);
            #elif defined LOG
                out = logi(angle);
            #elif defined SQRT
                out = sqrti(angle);
            #elif defined TANH
                out = tanhi(angle);
            #elif defined GELU
                out = gelu(angle);
            #elif defined CNDF
                out = cndfi(angle);
            #else
                out = sini(angle);
            #endif

            local_cache[byte_index] = out;

            performance_count += perfcounter_get();

            // printf("func(%d) = %d\n", angle, out);
        }

        mram_write(local_cache, &buffer[write_pos], sizeof(int) * CACHE_SIZE);
    }

    return 0;
}