#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#ifdef NOWRAP
    char extension[]="nowrap";
#else
    char extension[]="wrap";
#endif

#define FIXED_FRACTION_BITS 24

#ifdef LUT_FIXED
#include "../../host/lut_fixed_host.c"
    char method[]="lut-fixed-nointerpolate";
#elif defined LUT_FIXED_INTERPOLATE
#include "../../host/lut_fixed_host.c"
    char method[]="lut-fixed-interpolate";
#endif

#ifdef COS
char operation[]= "cos";
    double (*original)() = cos;
#elif defined TAN
char operation[]= "tan";
    double (*original)() = tan;
#elif defined EXP
char operation[]= "exp";
    double (*original)() = exp;
#elif defined LOG
char operation[]= "log";
    double (*original)() = log;
#elif defined SQRT
char operation[]= "sqrt";
    double (*original)() = sqrt;
#elif defined TANH
char operation[]= "tanh";
    double (*original)() = tanh;
#elif defined GELU
char operation[]= "gelu";
    double (*original)() = gelu;
#elif defined CNDF
    char operation[]= "cndf";
   double (*original)() = cndf;
#else
char operation[]= "sin";
double (*original)() = sin;
#endif

#if STORE_IN_WRAM > 0
    char storage[]="wram";
#else
    char storage[]="mram";
#endif

#if COUNT_INSTR > 0
    char perf[]="instructions";
#else
    char perf[]="cycles";
#endif


#ifndef DPU_BINARY
#define DPU_BINARY "bin/dpu/transcendental_performance_fixed"
#endif

#define M_PI 3.14159265358979323846

#define BUFFER_SIZE (1 << ARRAY_SIZE)

float distribute(float min, float max, int index, int total) {
    return (float) index / (float) BUFFER_SIZE * (max - min) + min;
}


void populate_mram(struct dpu_set_t set, int input_buffer[]) {
    float x;
    float offset = 0.000615643;
    float factor = 0.993453984;
    for (int byte_index = 0; byte_index < BUFFER_SIZE; byte_index++) {
        #ifdef NOWRAP
            #ifdef EXP
                x = distribute(offset, log(2) * 0.95 * factor, byte_index, BUFFER_SIZE);
            #elif defined LOG
                x = distribute(0.5 + offset, 1.1, byte_index, BUFFER_SIZE);
            #elif defined TAN
                x = distribute(- M_PI * 0.4, M_PI * 0.4, byte_index, BUFFER_SIZE);
                x = x < 0 ? x + 2 * M_PI : x;
            #elif defined SQRT
                x = distribute(0.5 + offset, 2 * factor, byte_index, BUFFER_SIZE);
            #else
                x = distribute(offset, 2 * M_PI * factor, byte_index, BUFFER_SIZE);
            #endif
        #else
            #ifdef EXP
                    x = distribute(-5, 5, byte_index, BUFFER_SIZE);
            #elif defined LOG
                    x = distribute(0.1, 100000, byte_index, BUFFER_SIZE);
            #elif defined TAN
                    x = distribute(- M_PI * 0.4, M_PI * 0.4, byte_index, BUFFER_SIZE);
                    x = x < 0 ? x + 2 * M_PI : x;
            #elif defined SQRT
                    x = distribute(offset, 10000, byte_index, BUFFER_SIZE);
            #else
                    x = distribute(offset, 4 * M_PI * factor, byte_index, BUFFER_SIZE);
            #endif
        #endif
        #if defined TANH
         x = distribute(-20, 20, byte_index, BUFFER_SIZE);
        #elif defined GELU
         x = distribute(-20, 20, byte_index, BUFFER_SIZE);
        #endif

        input_buffer[byte_index] = (int) (ldexpf(x, FIXED_FRACTION_BITS) + 0.5f);
        // printf("%f\n", x);
    }
    DPU_ASSERT(dpu_broadcast_to(set, "buffer", 0, input_buffer, sizeof(int) * BUFFER_SIZE, DPU_XFER_DEFAULT));
}

float ulp(float x)
{
    if (x > 0) {
        return nextafterf(x, INFINITY) - x;
    }  else {
        return nextafterf(x, -INFINITY) - x;
    }
}


int main(void) {
    int input_buffer[BUFFER_SIZE];
    struct dpu_set_t set, dpu;

    DPU_ASSERT(dpu_alloc(1, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    broadcast_tables(set);
    populate_mram(set, input_buffer);

    double start = clock();
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    double end = clock();

    printf("Performed Operation:                      %s_%s_%s_%s\n", method, extension, storage, operation);

    // retrieve number of instructions on DPU
    // retrieve DPU frequency
    uint32_t performance_count;
    uint32_t clocks_per_sec;
    DPU_FOREACH(set, dpu) {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
        DPU_ASSERT(dpu_copy_from(dpu, "performance_count", 0, &performance_count, sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "CLOCKS_PER_SEC", 0, &clocks_per_sec, sizeof(uint32_t)));
    }

    int output_buffer[BUFFER_SIZE];

    DPU_FOREACH(set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, "buffer", 0, &output_buffer, sizeof(float) * BUFFER_SIZE));
    }

    DPU_ASSERT(dpu_free(set));

    double error, ulp_error;
    double max_error_position, max_ulp_error_position;
    double max_error = 0;
    double cumulative_error = 0;
    double max_ulp_error = 0;
    double cumulative_ulp_error = 0;
    float input_float, output_float;

    for (int byte_index = 0; byte_index < BUFFER_SIZE; byte_index++) {

        input_float = ldexpf(input_buffer[byte_index], -FIXED_FRACTION_BITS);
        output_float = ldexpf(output_buffer[byte_index], -FIXED_FRACTION_BITS);
        error = fabs((original(input_float)) - output_float);
        ulp_error = error / ulp((float) (original(input_float)));

        // printf("Input: %f, Golden Output: %.24f, Actual Output: %.24f, Error: %.24f, Error in ULPs: %f\n", input_float, original(input_float), output_float, error, ulp_error);

        cumulative_error += error * error;
        cumulative_ulp_error += ulp_error * ulp_error;

        max_error_position =  error > max_error ? input_buffer[byte_index] : max_error_position;
        max_error =  error > max_error ? error : max_error;

        max_ulp_error_position =  ulp_error > max_ulp_error ? input_buffer[byte_index] : max_ulp_error_position;
        max_ulp_error =  ulp_error > max_ulp_error ? ulp_error : max_ulp_error;

    }

    double average_squared_error = sqrt(cumulative_error / performance_count);
    double average_squared_ulp_error = sqrt(cumulative_ulp_error / performance_count);

    // Printouts
    printf("Average Squared Error:                    %.4e\n", average_squared_error);
    printf("Average Squared ULP Error:                %f \n", average_squared_ulp_error);

    printf("Max Error:                                %.4e at %f \n", max_error, ldexp(max_error_position, -FIXED_FRACTION_BITS));
    printf("Max ULP Error:                            %f   at %f \n", max_ulp_error, ldexp(max_ulp_error_position, -FIXED_FRACTION_BITS));

    FILE *out_file = fopen("output/function.csv", "a"); // write only
    fprintf(out_file, "%s_%s_%s_%s_%s, %d, %.2f, %e, %e, %e, %e\n", method, extension, storage, operation, perf, PRECISION, (float) performance_count / (float) BUFFER_SIZE, average_squared_error, max_error, average_squared_ulp_error, max_ulp_error); // write to file

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