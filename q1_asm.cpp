// Q1: 看各型別的加法各自編譯成什麼指令
// build: g++-16 -std=c++17 -O2 -S q1_asm.cpp -o q1_asm.s
_Float16    add_h(_Float16 a, _Float16 b)       { return a + b; }
float       add_f(float a, float b)             { return a + b; }
double      add_d(double a, double b)           { return a + b; }
long double add_ld(long double a, long double b) { return a + b; }
_Float128   add_q(_Float128 a, _Float128 b)     { return a + b; }

void axpy_f(float* a, const float* b, float k, int n)    { for (int i = 0; i < n; ++i) a[i] = a[i] * k + b[i]; }
void axpy_d(double* a, const double* b, double k, int n) { for (int i = 0; i < n; ++i) a[i] = a[i] * k + b[i]; }
