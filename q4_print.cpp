// Q4: printf("%.9f") / std::fixed << setprecision(9) 真的是「四捨五入到小數第 9 位」嗎？
// build: g++-16 -std=c++17 -O2 q4_print.cpp -o q4_print
#include <cstdio>
#include <iomanip>
#include <sstream>

// 印出：程式裡寫的字面值、printf 的結果、iostream 的結果、記憶體裡實際存的值（展開到 25 位）
static void show(const char* literal, double x) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(9) << x;
    std::printf("%-14s | printf %.9f | iostream %s | 實際存的值 %.25f\n", literal, x, os.str().c_str(), x);
}
#define S(lit) show(#lit, lit)   // 把字面值本身當字串一起印

int main() {
    S(0.1234567895);
    S(1.0000000005);
    std::printf("%%.0f of 2.5 = %.0f,  %%.0f of 3.5 = %.0f\n", 2.5, 3.5);
}
