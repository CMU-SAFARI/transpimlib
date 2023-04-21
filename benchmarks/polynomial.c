#import "../dpu/_ldexpf.c"

// Import from stackoverflow:
// https://stackoverflow.com/a/10552567
// Adriano Repetti
// previously
// scoofy / fuzzpilz
static float expf(float x) {
    return (362880+x*(362880+x*(181440+x*(60480+x*(15120+x*(3024+x*(504+x*(72+x*(9+x)))))))))*2.75573192e-6;
}


// Another Import from stackoverflow: https://stackoverflow.com/a/39822314
float logf (float a) {
    float m, r, s, t, i, f;
    int32_t e, q;
    e = (* (int * ) &a - 0x3f2aaaab) & 0xff800000;
    q = (* (int * ) &a - e);
    m = * (float * ) &q;
    i = (float)e * 1.19209290e-7f; // 0x1.0p-23
    f = m - 1.0f;
    s = f * f;
    r = 0.230836749f * f -0.279208571f; // 0x1.d8c0f0p-3, -0x1.1de8dap-2
    t = 0.331826031 * f -0.498910338f; // 0x1.53ca34p-2, -0x1.fee25ap-2
    r = r * s + t;
    r = r * s + f;
    r = i * 0.693147182f + r; // 0x1.62e430p-1 // log(2)
    return r;
}

float sqrtf(float x)
{
    if (x < 1) return 1.0 / sqrtf(1.0 / x);

    float xhi = x;
    float xlo = 0;
    float guess = ldexpf(x, -1);

    while (guess * guess != x)
    {
        if (guess * guess > x)
            xhi = guess;
        else
            xlo = guess;

        float new_guess = ldexpf(xhi + xlo, -1);
        if (new_guess == guess)
            break; // not getting closer
        guess = new_guess;
    }
    return guess;
}