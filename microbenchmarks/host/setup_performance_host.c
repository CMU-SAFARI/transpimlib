#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define MEASURE

#if STORE_IN_WRAM > 0
    char storage[]="wram";
#else
    char storage[]="mram";
#endif

#ifdef CORDIC
#include "../../host/cordic_host.c"
    char method[]="cordic";
#elif defined CORDIC_LUT
#include "../../host/cordic_lut_host.c"
    char method[]="cordic-lut";
#elif defined LUT_LDEXPF
#include "../../host/lut_ldexpf_host.c"
    char method[]="lut-ldexpf-nointerpolate";
#elif defined LUT_LDEXPF_INTERPOLATE
#include "../../host/lut_ldexpf_host.c"
    char method[]="lut-ldexpf-interpolate";
#elif defined LUT_MULTI
#include "../../host/lut_multi_host.c"
    char method[]="lut-multi-nointerpolate";
#elif defined LUT_MULTI_INTERPOLATE
    #include "../../host/lut_multi_host.c"
    char method[]="lut-multi-interpolate";
#elif defined LUT_DIRECT
#include "../../host/lut_direct_host.c"
    char method[]="lut-direct-interpolate";
#elif defined LUT_DIRECT_LDEXPF
#include "../../host/lut_direct_ldexpf_host.c"
    char method[]="lut-direct-ldexpf-interpolate";
#endif

#ifndef DPU_BINARY
#define DPU_BINARY "./bin/dpu/setup_performance"
#endif

int main(void) {
    struct dpu_set_t set, dpu;

    DPU_ASSERT(dpu_alloc(1, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    broadcast_tables(set);

    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    DPU_ASSERT(dpu_free(set));

    return 0;
}
