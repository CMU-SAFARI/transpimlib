#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>
#include <defs.h>
#include <mutex.h>

#ifdef CORDIC_F2F
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
#elif defined POLYNOMIAL
#include "../polynomial.c"
#endif

#define ROWS_PER_TASKLET 10000
__mram_noinit float data_array[NR_TASKLETS * ROWS_PER_TASKLET];
__host int used_rows = NR_TASKLETS * ROWS_PER_TASKLET;


#define LOCAL_ROWS 4
float local[LOCAL_ROWS * NR_TASKLETS];

int main(){
    unsigned int start_row = me() * used_rows;
    float temp;

    for (unsigned int global_index = 0; global_index < used_rows; global_index += LOCAL_ROWS * NR_TASKLETS) {

        int local_start = me() * LOCAL_ROWS;
        int local_end = local_start + LOCAL_ROWS;

        mram_read(&data_array[global_index + local_start], &local[local_start], sizeof(float) * LOCAL_ROWS);

        for (unsigned int local_index = local_start; (local_index < local_end) && (global_index + local_index < used_rows); local_index++) {
            temp = expf(-local[local_index]);
            local[local_index] = 1/(1+temp);
        }

        mram_write(&local[local_start], &data_array[global_index + local_start], sizeof(float) * LOCAL_ROWS);

    }

    return 0;
}
