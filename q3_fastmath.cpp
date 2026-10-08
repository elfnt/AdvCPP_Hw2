// Q3: 同一份程式，有無 -ffast-math 的結果比較
// build: g++-16 -std=c++17 -O2             q3_fastmath.cpp -o q3_strict
//        g++-16 -std=c++17 -O2 -ffast-math q3_fastmath.cpp -o q3_fast
#include <cmath>
#include <cstdio>

#define SHOW(expr) std::printf("%-24s = %g\n", #expr, (double)(expr))

// noinline：讓編譯器把它當獨立函式處理，避免整個被常數折疊，也方便看組語
__attribute__((noinline)) double assoc(double a, double b, double c) { return (a + b) + c; }
__attribute__((noinline)) bool   is_nan(double x)  { return std::isnan(x); }
__attribute__((noinline)) double div_x_x(double x) { return x / x; }

int main() {
#ifdef __FAST_MATH__
    std::puts("== WITH -ffast-math ==");
#else
    std::puts("== WITHOUT -ffast-math ==");
#endif
    SHOW(assoc(1e16, -1e16, 1.0));   // (1e16 + -1e16) + 1 = 1；若改成 1e16 + (-1e16 + 1) 就是 0
    SHOW(is_nan(NAN));
    SHOW(div_x_x(0.0));              // 0/0 應該是 NaN
}
