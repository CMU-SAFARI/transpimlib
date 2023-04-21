#include <assert.h>
#include <stdio.h>
#include "../../dpu/_ldexpf.c"

int main() {
    // Making sure the first bit works
    assert(ldexpf(1.5f, 1) == 3.0f);
    assert(ldexpf(-1.5f, 1) == -3.0f);
    printf("First Bit Works\n");

    // Making sure the last bit works
    assert(ldexpf(1.00000011920928955078125f, 1) == 2.0000002384185791015625);
    assert(ldexpf(-1.00000011920928955078125f, 1) == -2.0000002384185791015625);
    printf("Last Bit Works\n");


    // Normalized to MAXINT
    assert(ldexpf(1.0f, 127) == 170141183460469231731687303715884105728.0f);
    assert(ldexpf(1.0f, 128) == 340282346638528859811704183484516925440.0f);
    assert(ldexpf(2.0f, 127) == 340282346638528859811704183484516925440.0f);

    assert(ldexpf(-1.0f, 127) == -170141183460469231731687303715884105728.0f);
    assert(ldexpf(-1.0f, 128) == -340282346638528859811704183484516925440.0f);
    assert(ldexpf(-2.0f, 127) == -340282346638528859811704183484516925440.0f);
    printf("MAXINT Works\n");

    // Normalized to Denormalized
    assert(ldexpf(1.76324152623e-38f, -1)/1.76324152623e-38f == 0.5f);
    assert(ldexpf(1.76324152623e-38f, -2)/1.76324152623e-38f == 0.25f);
    assert(ldexpf(1.76324152623e-38f, -3)/1.76324152623e-38f == 0.125f);

    assert(ldexpf(-1.76324152623e-38f, -1)/-1.76324152623e-38f == 0.5f);
    assert(ldexpf(-1.76324152623e-38f, -2)/-1.76324152623e-38f == 0.25f);
    assert(ldexpf(-1.76324152623e-38f, -3)/-1.76324152623e-38f == 0.125f);
    printf("Normalized to Denormalized Works\n");

    // Denormalized to Normalized
    assert(ldexpf(5.87747175411e-39f, 1)/5.87747175411e-39f == 2.0f);
    assert(ldexpf(5.87747175411e-39f, 2)/5.87747175411e-39f == 4.0f);
    assert(ldexpf(5.87747175411e-39f, 3)/5.87747175411e-39f == 8.0f);

    assert(ldexpf(-5.87747175411e-39f, 1)/-5.87747175411e-39f == 2.0f);
    assert(ldexpf(-5.87747175411e-39f, 2)/-5.87747175411e-39f == 4.0f);
    assert(ldexpf(-5.87747175411e-39f, 3)/-5.87747175411e-39f == 8.0f);
    printf("Denormalized to Normalized Works\n");

    // Denormalized to Denormalized
    assert(ldexpf(2.93873587706e-39f, 1) / 2.93873587706e-39f == 2.0f);
    assert(ldexpf(2.93873587706e-39f, -1) / 2.93873587706e-39f == 0.5F);

    assert(ldexpf(-2.93873587706e-39f, 1) / -2.93873587706e-39f == 2.0f);
    assert(ldexpf(-2.93873587706e-39f, -1) / -2.93873587706e-39f == 0.5F);
    printf("Denormalized to Denormalized Works\n");

    // Denormalized to MAXINT
    assert(ldexpf(2.93873587706e-39f, 1000) == 340282346638528859811704183484516925440.0f);
    assert(ldexpf(-2.93873587706e-39f, 1000) == -340282346638528859811704183484516925440.0f);
    printf("Denormalized to MAXINT Works\n");

    // Zero Exponentiation
    assert(ldexpf(0.0f, 127) == 0.0f);
    assert(ldexpf(-0.0f, 127) == -0.0f);
    printf("Zero Exponentiation Works\n");

    for (int test_int = -100; test_int < 100; ++test_int) {
        assert(floating_to_fixed(fixed_to_floating(test_int<<10)) == test_int<<10);
    }

    float test_float;
    for (int test_int = -100; test_int < 100; ++test_int) {
        test_float= (float) test_int;
        assert(fixed_to_floating(floating_to_fixed(test_float)) == test_float);
    }
    printf("Basic bijectiveness Tests Work\n");
}
