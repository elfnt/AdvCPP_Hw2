// Q2: probe how this machine's libm sin() behaves
// build: clang++ -std=c++17 -O2 q2_sin_test.cpp -o q2_sin_test
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cfenv>

static uint64_t bits(double d) { uint64_t u; std::memcpy(&u, &d, 8); return u; }

int main() {
    // (a) special values required by the C standard / IEEE 754
    double specials[] = { 0.0, -0.0, INFINITY, -INFINITY, NAN, 1e-310 /*subnormal*/, 0x1p-30 };
    for (double x : specials) {
        std::feclearexcept(FE_ALL_EXCEPT);
        double y = std::sin(x);
        int ex = std::fetestexcept(FE_ALL_EXCEPT);
        std::printf("sin(%-10g) = %-24.17g bits=%016llx  exc:%s%s%s\n", x, y, (unsigned long long)bits(y),
                    (ex & FE_INVALID) ? " INVALID" : "", (ex & FE_INEXACT) ? " INEXACT" : "",
                    (ex & FE_UNDERFLOW) ? " UNDERFLOW" : "");
    }
    // (b) values across the three range-reduction regimes found in the disassembly
    //     |x| <= pi/4   : no reduction, straight polynomial
    //     |x| <  524288.6: Cody-Waite  (x*2/pi -> frintn, 3-part pi/2)
    //     |x| >= 524288.6: Payne-Hanek (128-bit integer multiply by 2/pi, umulh)
    double xs[] = { 0.5, 0.785398163397448, 1.0, 3.0, 100.0, 1e5, 524288.0, 524289.0,
                    1e6, 1e10, 1e22, 1e300, 0x1.921fb54442d18p+1 /* nearest double to pi */ };
    std::printf("\n%-24s %-26s %-18s\n", "x", "sin(x)", "bits");
    for (double x : xs)
        std::printf("%-24.17g %-26.17g %016llx\n", x, std::sin(x), (unsigned long long)bits(std::sin(x)));
    return 0;
}
