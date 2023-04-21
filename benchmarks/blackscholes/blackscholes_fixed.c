#include <mram.h>
#include <perfcounter.h>
#include <stdio.h>
#include <defs.h>


#ifdef FIXED
#include "../../dpu/lut_fixed.c"
#else
#include "../../dpu/lut_fixed_interpolate.c"
#endif


#ifndef  MULT
#define MULT(x, y) ((int)(((long)x * y) >> FIXED_FRACTION_BITS))
#endif

#ifndef  DIV
#define DIV(x, y) ((int)((((long)x << FIXED_FRACTION_BITS) / y))
#endif

typedef struct OptionData_ {
    int s;          // spot price
    int strike;     // strike price
    int r;          // risk-free interest rate
    int divq;       // dividend rate
    int v;          // volatility
    int t;          // time to maturity or option expiration in years
    //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)
    char OptionType;   // Option type.  "P"=PUT, "C"=CALL
    int divs;       // dividend vals (not used in this test)
    int DGrefval;   // DerivaGem Reference Value
} OptionData;

#define ROWS_PER_TASKLET 10000
__host int used_rows = ROWS_PER_TASKLET * NR_TASKLETS;
__mram_noinit int price[NR_TASKLETS * ROWS_PER_TASKLET];
__mram_noinit OptionData option[NR_TASKLETS * ROWS_PER_TASKLET];

#define LOCAL_ROWS 1
OptionData local[LOCAL_ROWS * NR_TASKLETS];
int local_price[LOCAL_ROWS * NR_TASKLETS];

// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
#define inv_sqrt_2xPI (1713444047 >> (32 - FIXED_FRACTION_BITS))


int BlkSchlsEqEuroNoDiv( int sptprice,
                            int strike, int rate, int volatility,
                            int time, int otype)
{
    int OptionPrice;

    // local private working variables for the calculation
    int xStockPrice;
    int xStrikePrice;
    int xRiskFreeRate;
    int xVolatility;
    int xTime;
    int xSqrtTime;

    int logValues;
    int xLogTerm;
    int xD1;
    int xD2;
    int xPowerTerm;
    int xDen;
    int d1;
    int d2;
    int FutureValueX;
    int NofXd1;
    int NofXd2;
    int NegNofXd1;
    int NegNofXd2;

    xStockPrice = sptprice;
    xStrikePrice = strike;
    xRiskFreeRate = rate;
    xVolatility = volatility;

    xTime = time;
    xSqrtTime = sqrti(xTime);

    logValues = logi(DIV(sptprice, strike));

    xLogTerm = logValues;


    xPowerTerm = MULT(xVolatility, xVolatility);
    xPowerTerm = xPowerTerm >> 1;

    xD1 = xRiskFreeRate + xPowerTerm;
    xD1 = MULT(xD1, xTime);
    xD1 = xD1 + xLogTerm;

    xDen = MULT(xVolatility, xSqrtTime);
    xD1 = DIV(xD1, xDen);
    xD2 = xD1 -  xDen;

    d1 = xD1;
    d2 = xD2;

    NofXd1 = cndfi( d1 );
    NofXd2 = cndfi( d2 );

    FutureValueX = MULT(strike, (expi( -MULT(rate,time))));
    if (otype == 0) {
        OptionPrice = MULT(sptprice, NofXd1) - MULT(FutureValueX, NofXd2);
    } else {
        NegNofXd1 = ((1 << FIXED_FRACTION_BITS)- NofXd1);
        NegNofXd2 = ((1 << FIXED_FRACTION_BITS) - NofXd2);
        OptionPrice = MULT(FutureValueX,  NegNofXd2) - MULT(sptprice, NegNofXd1);
    }

    return OptionPrice;
}


int main(){
    for (unsigned int global_index = 0; global_index < used_rows; global_index += LOCAL_ROWS * NR_TASKLETS) {


        int local_start = me() * LOCAL_ROWS;
        int local_end = local_start + LOCAL_ROWS;

        mram_read(&option[global_index + local_start], &local[local_start], sizeof(OptionData) * LOCAL_ROWS);

        for (unsigned int local_index = local_start; (local_index < local_end) && (global_index + local_index < used_rows); local_index++) {

            if (!(local[local_index].strike == 0 || local[local_index].v == 0 || local[local_index].t == 0)) {
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