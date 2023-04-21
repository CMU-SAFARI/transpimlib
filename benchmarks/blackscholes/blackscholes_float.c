#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>
#include <defs.h>

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

#define fptype float

typedef struct OptionData_ {
    fptype s;          // spot price
    fptype strike;     // strike price
    fptype r;          // risk-free interest rate
    fptype divq;       // dividend rate
    fptype v;          // volatility
    fptype t;          // time to maturity or option expiration in years
    //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)
    char OptionType;   // Option type.  "P"=PUT, "C"=CALL
    fptype divs;       // dividend vals (not used in this test)
    fptype DGrefval;   // DerivaGem Reference Value
} OptionData;

#define ROWS_PER_TASKLET 10000
__host int used_rows = ROWS_PER_TASKLET * NR_TASKLETS;
__mram_noinit float price[NR_TASKLETS * ROWS_PER_TASKLET];
__mram_noinit OptionData option[NR_TASKLETS * ROWS_PER_TASKLET];

#define LOCAL_ROWS 2
OptionData local[LOCAL_ROWS * NR_TASKLETS];
float local_price[LOCAL_ROWS * NR_TASKLETS];

// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
#define inv_sqrt_2xPI 0.39894228040143270286

fptype CNDF ( fptype InputX )
{
    int sign;

    fptype OutputX;
    fptype xInput;
    fptype xNPrimeofX;
    fptype expValues;
    fptype xK2;
    fptype xK2_2, xK2_3;
    fptype xK2_4, xK2_5;
    fptype xLocal, xLocal_1;
    fptype xLocal_2, xLocal_3;

    // Check for negative value of InputX
    if (InputX < 0.0) {
        InputX = -InputX;
        sign = 1;
    } else
        sign = 0;

    xInput = InputX;

    // Compute NPrimeX term common to both four & six decimal accuracy calcs
    expValues = expf(-0.5f * InputX * InputX);
    xNPrimeofX = expValues;
    xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

    xK2 = 0.2316419 * xInput;
    xK2 = 1.0 + xK2;
    xK2 = 1.0 / xK2;
    xK2_2 = xK2 * xK2;
    xK2_3 = xK2_2 * xK2;
    xK2_4 = xK2_3 * xK2;
    xK2_5 = xK2_4 * xK2;

    xLocal_1 = xK2 * 0.319381530;
    xLocal_2 = xK2_2 * (-0.356563782);
    xLocal_3 = xK2_3 * 1.781477937;
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_4 * (-1.821255978);
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_5 * 1.330274429;
    xLocal_2 = xLocal_2 + xLocal_3;

    xLocal_1 = xLocal_2 + xLocal_1;
    xLocal   = xLocal_1 * xNPrimeofX;
    xLocal   = 1.0 - xLocal;

    OutputX  = xLocal;

    if (sign) {
        OutputX = 1.0 - OutputX;
    }

    return OutputX;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
fptype BlkSchlsEqEuroNoDiv( fptype sptprice,
                            fptype strike, fptype rate, fptype volatility,
                            fptype time, int otype)
{
    fptype OptionPrice;

    // local private working variables for the calculation
    fptype xStockPrice;
    fptype xStrikePrice;
    fptype xRiskFreeRate;
    fptype xVolatility;
    fptype xTime;
    fptype xSqrtTime;

    fptype logValues;
    fptype xLogTerm;
    fptype xD1;
    fptype xD2;
    fptype xPowerTerm;
    fptype xDen;
    fptype d1;
    fptype d2;
    fptype FutureValueX;
    fptype NofXd1;
    fptype NofXd2;
    fptype NegNofXd1;
    fptype NegNofXd2;

    xStockPrice = sptprice;
    xStrikePrice = strike;
    xRiskFreeRate = rate;
    xVolatility = volatility;

    xTime = time;
    xSqrtTime = sqrtf(xTime);

    logValues = logf( sptprice / strike );

    xLogTerm = logValues;


    xPowerTerm = xVolatility * xVolatility;
    xPowerTerm = xPowerTerm * 0.5;

    xD1 = xRiskFreeRate + xPowerTerm;
    xD1 = xD1 * xTime;
    xD1 = xD1 + xLogTerm;

    xDen = xVolatility * xSqrtTime;
    xD1 = xD1 / xDen;
    xD2 = xD1 -  xDen;

    d1 = xD1;
    d2 = xD2;

#ifdef LUT_LDEXPF_INTERPOLATE
    NofXd1 = cndf( d1 );
    NofXd2 = cndf( d2 );
#elif defined LUT_MULTI_INTERPOLATE
    NofXd1 = cndf( d1 );
    NofXd2 = cndf( d2 );
#else
    NofXd1 = CNDF( d1 );
    NofXd2 = CNDF( d2 );
#endif

    FutureValueX = strike * ( expf( -(rate)*(time) ) );
    if (otype == 0) {
        OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
    } else {
        NegNofXd1 = (1.0 - NofXd1);
        NegNofXd2 = (1.0 - NofXd2);
        OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
    }

    return OptionPrice;
}


int main(){
    for (unsigned int global_index = 0; global_index < used_rows; global_index += LOCAL_ROWS * NR_TASKLETS) {

        int local_start = me() * LOCAL_ROWS;
        int local_end = local_start + LOCAL_ROWS;

        mram_read(&option[global_index + local_start], &local[local_start], sizeof(OptionData) * LOCAL_ROWS);

        for (unsigned int local_index = local_start; (local_index < local_end) && (global_index + local_index < used_rows); local_index++) {

            if (!(local[local_index].strike == 0.0 || local[local_index].v == 0.0 || local[local_index].t == 0.0)) {
                local_price[local_index] = BlkSchlsEqEuroNoDiv(local[local_index].s, local[local_index].strike,
                                                               local[local_index].r, local[local_index].v,
                                                               local[local_index].t,
                                                               local[local_index].OptionType == 80 ? 1 : 0);
            }
        }

        mram_write(&local_price[local_start], &price[global_index + local_start], sizeof(int) * LOCAL_ROWS);

    }

    return 0;
}
