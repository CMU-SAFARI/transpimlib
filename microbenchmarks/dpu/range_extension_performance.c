#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>
#include "../../dpu/_range_extensions.c"

#define CACHE_SIZE 128
#define BUFFER_SIZE (1 << ARRAY_SIZE)

__mram_noinit float buffer[BUFFER_SIZE];
__host uint32_t performance_count;

int main(){

    performance_count = 0;
    __dma_aligned float local_cache[CACHE_SIZE];

    printf("Operations Tested:                        %d\n", BUFFER_SIZE);

    float x, out;
    int extra_data;
    for (unsigned int bytes_read = 0; bytes_read < BUFFER_SIZE;) {

        mram_read(&buffer[bytes_read], local_cache, sizeof(float) * CACHE_SIZE);

        unsigned int write_pos = bytes_read;

        for (unsigned int byte_index = 0; (byte_index < CACHE_SIZE) && (bytes_read < BUFFER_SIZE); byte_index++ , bytes_read++) {

            x = local_cache[byte_index];

            perfcounter_config(COUNT_CYCLES, true);

            #ifdef SIN
                out = sin_cos_tan_range_extension_in(x);
            #elif defined EXP
                out = exp_range_extension_out(exp_range_extension_in(x, &extra_data),  &extra_data);
            #elif defined LOG
                out = log_range_extension_out(log_range_extension_in(x, &extra_data),  &extra_data);
            #elif defined SQRT
                 out = sqrt_range_extension_out(sqrt_range_extension_in(x, &extra_data),  &extra_data);
            #endif

            local_cache[byte_index] = out;

            performance_count += perfcounter_get();

        }

        mram_write(local_cache, &buffer[write_pos], sizeof(float) * CACHE_SIZE);
    }

    return 0;
}