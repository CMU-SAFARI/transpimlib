#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#ifdef SIN
    char operation[]= "sin";
#elif defined EXP
    char operation[]= "exp";
#elif defined LOG
    char operation[]= "log";
#elif defined SQRT
    char operation[]= "sqrt";
#endif

#if COUNT_INSTR > 0
    char perf[]="instructions";
#else
    char perf[]="cycles";
#endif

#ifndef DPU_BINARY
#define DPU_BINARY "./bin/dpu/range_extension_performance"
#endif

#define M_PI 3.14159265358979323846

#define BUFFER_SIZE (1 << ARRAY_SIZE)

float distribute(float min, float max, int index, int total) {
    return (float) index / (float) BUFFER_SIZE * (max - min) + min;
}

void populate_mram(struct dpu_set_t set, float input_buffer[]) {
    float theta_rad;
    float offset = 0.000615643;
    float factor = 0.993453984;
    for (int byte_index = 0; byte_index < BUFFER_SIZE; byte_index++) {
        #ifdef EXP
                theta_rad = distribute(-5, 5, byte_index, BUFFER_SIZE);
        #elif defined LOG
                theta_rad = distribute(0.1, 100000, byte_index, BUFFER_SIZE);
        #elif defined TAN
                theta_rad = distribute(- M_PI * 0.4, M_PI * 0.4, byte_index, BUFFER_SIZE);
                theta_rad = theta_rad < 0 ? theta_rad + 2 * M_PI : theta_rad;
        #elif defined TANH
                theta_rad = distribute(-20, 20, byte_index, BUFFER_SIZE);
        #elif defined SQRT
                theta_rad = distribute(offset, 10000, byte_index, BUFFER_SIZE);
        #else
                theta_rad = distribute(offset, 4 * M_PI * factor, byte_index, BUFFER_SIZE);
        #endif
        input_buffer[byte_index] = theta_rad;
        // printf("%f\n", theta_rad);
    }
    DPU_ASSERT(dpu_broadcast_to(set, "buffer", 0, input_buffer, sizeof(float) * BUFFER_SIZE, DPU_XFER_DEFAULT));
}


int main(void) {
    float input_buffer[BUFFER_SIZE];
    struct dpu_set_t set, dpu;

    DPU_ASSERT(dpu_alloc(1, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    populate_mram(set, input_buffer);

    double start = clock();
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    double end = clock();

    printf("Performed Operation:                      %s\n", operation);

    // retrieve number of instructions on DPU
    // retrieve DPU frequency
    uint32_t performance_count;
    uint32_t clocks_per_sec;
    DPU_FOREACH(set, dpu) {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
        DPU_ASSERT(dpu_copy_from(dpu, "performance_count", 0, &performance_count, sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "CLOCKS_PER_SEC", 0, &clocks_per_sec, sizeof(uint32_t)));
    }

    DPU_ASSERT(dpu_free(set));

    FILE *out_file = fopen("output/extension.csv", "a");
    fprintf(out_file, "%s_%s, %.2f\n", operation, perf, (float) performance_count / (float) BUFFER_SIZE);

    // Printouts
    #if COUNT_INSTR > 0
        printf("DPU instructions:                         %u\n", performance_count);
        printf("Instructions per Iteration:                %.2f\n\n", (float) performance_count / (float) BUFFER_SIZE);
        printf("DPU time:                                 %.2e secs.\n", (double) performance_count / clocks_per_sec);
    #else
        printf("DPU cycles:                               %u\n", performance_count);
        printf("Cycles per Iteration:                     %.2f\n\n", (float) performance_count / (float) BUFFER_SIZE);
    #endif

    printf("Host elapsed time:                        %.2e secs.\n\n", (end - start) / CLOCKS_PER_SEC );

    return 0;
}
