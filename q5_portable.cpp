// Q5: 同一份程式、同一組旗標，在不同機器上跑，結果是否相同？
// build（每台機器都用相同旗標）:  g++ -std=c++17 -O2 q5_portable.cpp -o q5
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

// 印出值和它的 64 個 bit（十六進位），只要 bits 不同就是結果不同
static unsigned long long bits(double d) { unsigned long long u; std::memcpy(&u, &d, 8); return u; }
#define SHOW(expr) std::printf("  %-30s = %-24.17g  bits=%016llx\n", #expr, (double)(expr), bits((double)(expr)))

__attribute__((noinline)) double mul_sub(double a, double b, double c) { return a * b - c; }   // 一條算式：可能被收縮成 FMA
__attribute__((noinline)) double mul_only(double a, double b)          { return a * b; }       // 先存起來：一定先捨入
__attribute__((noinline)) double add3(double a, double b, double c)    { return (a + b) + c; } // x87 會用 80-bit 算中間值

int main() {
    std::puts("=== 1. 平台 ===");
#if defined(__aarch64__)
    std::puts("  arch = arm64");
#elif defined(__x86_64__)
    std::puts("  arch = x86_64");
#elif defined(__i386__)
    std::puts("  arch = x86 (32-bit)");
#else
    std::puts("  arch = other");
#endif
    std::printf("  gcc %d.%d.%d, sizeof(void*) = %zu\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, sizeof(void*));
    std::printf("  sizeof(long double) = %zu, LDBL_MANT_DIG = %d, FLT_EVAL_METHOD = %d\n",
                sizeof(long double), LDBL_MANT_DIG, (int)FLT_EVAL_METHOD);
#ifdef FP_FAST_FMA
    std::puts("  FP_FAST_FMA: defined (有硬體 FMA)");
#else
    std::puts("  FP_FAST_FMA: not defined (沒有硬體 FMA)");
#endif

    std::puts("\n=== 2. FMA 收縮：a*b - c ===");
    double a = 1.0 + 0x1p-27, b = 1.0 - 0x1p-27;   // a*b = 1 - 2^-54，double 存不下，捨入後剛好是 1
    double p = mul_only(a, b);
    SHOW(p);                                         // 一定是 1
    SHOW(mul_sub(a, b, p));                          // 有 FMA：a*b 不先捨入 → -2^-54；沒有 FMA：1 - 1 = 0

    std::puts("\n=== 3. 中間值精度：(1e16 + 1) - 1e16 ===");
    SHOW(add3(1e16, 1.0, -1e16));                    // double 算：1e16+1 存不下 → 0；x87 80-bit 中間值：存得下 → 1

    std::puts("\n=== 4. long double ===");
    long double one = 1.0L, tiny = 1e-18L;
    std::printf("  ((1 + 1e-18) - 1) * 1e18 = %g   (64-bit 尾數約 1；53-bit 是 0)\n", (double)(((one + tiny) - one) * 1e18L));

    std::puts("\n=== 5. 數學函式庫 ===");
    SHOW(std::exp(0.1));
    SHOW(std::pow(2.1, 3.3));
    SHOW(std::sin(1e22));
    SHOW(std::tgamma(5.5));

    std::puts("\n=== 6. printf 的捨入 ===");
    std::printf("  %%.0f of 0.5 1.5 2.5 3.5 = %.0f %.0f %.0f %.0f\n", 0.5, 1.5, 2.5, 3.5);
}
