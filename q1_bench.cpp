// Q1: _Float16 / float / double / long double / _Float128 速度比較
// build: g++-16 -std=c++17 -O2 q1_bench.cpp -o q1_bench -lquadmath
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include <quadmath.h>   // _Float128 的 sqrt / sin：標準庫沒有，用 GCC 附的 libquadmath（連結時加 -lquadmath）

using Clock = std::chrono::steady_clock;

// 數學函式：float/double/long double 用 <cmath>；_Float16 先轉成 float 算；_Float128 用 libquadmath
template <class T> T my_sqrt(T x) { return std::sqrt(x); }
template <class T> T my_sin(T x)  { return std::sin(x); }
_Float16  my_sqrt(_Float16 x)  { return (_Float16)std::sqrt((float)x); }
_Float16  my_sin(_Float16 x)   { return (_Float16)std::sin((float)x); }
_Float128 my_sqrt(_Float128 x) { return sqrtq(x); }
_Float128 my_sin(_Float128 x)  { return sinq(x); }

// 把結果寫進 volatile 變數，避免編譯器把整段運算刪掉
template <class T> void keep(T v) { volatile T sink = v; (void)sink; }

// 回傳每次運算平均花費的奈秒數
template <class F> double ns_per_op(int n, F body) {
    auto t0 = Clock::now();
    body();
    auto t1 = Clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / n;
}

// 實驗一：相依鏈。x 的新值依賴舊值，CPU 無法把多次運算平行化或向量化，量到的是單一指令的延遲
template <class T> double chain_add(int n) { T x = 0, d = T(1e-3);           double t = ns_per_op(n, [&]{ for (int i = 0; i < n; ++i) x = x + d; }); keep(x); return t; }
// 乘數用 1.001 而不是 1.0001：_Float16 只有 11 bits 尾數，1.0001 會被捨入成 1.0，整個迴圈就被編譯器當成沒事做而刪掉
template <class T> double chain_mul(int n) { T x = 1, m = T(1.001);          double t = ns_per_op(n, [&]{ for (int i = 0; i < n; ++i) x = x * m; }); keep(x); return t; }
template <class T> double chain_div(int n) { T x = T(60000), m = T(1.001);   double t = ns_per_op(n, [&]{ for (int i = 0; i < n; ++i) x = x / m; }); keep(x); return t; }
template <class T> double chain_sqrt(int n) { T x = 2;                       double t = ns_per_op(n, [&]{ for (int i = 0; i < n; ++i) x = my_sqrt(x + T(1)); }); keep(x); return t; }

// 實驗二：獨立陣列 a[i] = a[i]*k + b[i]。元素彼此無關，編譯器可以用 SIMD 一次處理多個元素
template <class T> double array_axpy(int n, int reps) {
    std::vector<T> a(n, T(1.5)), b(n, T(2.5));
    T k = T(1.001);   // 同上，1.0001 在 _Float16 會變成 1.0
    return ns_per_op(n * reps, [&]{
        for (int r = 0; r < reps; ++r) {
            for (int i = 0; i < n; ++i) a[i] = a[i] * k + b[i];
            keep(a[r % n]);
        }
    });
}

// 實驗三：呼叫數學函式庫 std::sin
template <class T> double call_sin(int n) {
    T x = T(0.5), acc = 0;
    // 步長 1e-2 而非 1e-4：_Float16 的 0.5 + 1e-4 仍等於 0.5，編譯器會把 x 視為常數並把 sin 整個折疊掉
    double t = ns_per_op(n, [&]{ for (int i = 0; i < n; ++i) { acc += my_sin(x); x += T(1e-2); } });
    keep(acc); return t;
}

// n 依型別快慢調整：_Float128 是純軟體運算，次數減為 1/20，否則跑太久
template <class T> void run(const char* name, int mant_bits, int n_scale = 1) {
    const int N = 200'000'000 / n_scale;
    std::printf("%-12s %3zu-bit (mant %3d) | add %7.3f  mul %7.3f  div %7.3f  sqrt %7.3f | axpy %7.3f | sin %8.3f  (ns/op)\n",
                name, sizeof(T) * 8, mant_bits,
                chain_add<T>(N), chain_mul<T>(N), chain_div<T>(N / 4), chain_sqrt<T>(N / 4),
                array_axpy<T>(4096, 50'000 / n_scale), call_sin<T>(N / 10));
}

int main() {
    run<_Float16>("_Float16", __FLT16_MANT_DIG__);
    run<float>("float", __FLT_MANT_DIG__);
    run<double>("double", __DBL_MANT_DIG__);
    run<long double>("long double", __LDBL_MANT_DIG__);
    run<_Float128>("_Float128", __FLT128_MANT_DIG__, 20);
}
