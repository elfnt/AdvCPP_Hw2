# C++ Hw1

實驗環境：

| 項目 | 內容 |
|:--|:--|
| 機器 | MacBook Air，Apple M3（arm64，4 P-core + 4 E-core），24 GB RAM |
| 作業系統 | macOS 15.6.1（24G90） |
| 編譯器 | Homebrew GCC 16.2.0（`g++-16`） |
| 編譯旗標 | 預設 `-std=c++17 -O2`；個別實驗另加 `-O3`、`-ffast-math`、`-march=armv8.2-a+fp16`，會在該題註明 |
| 函式庫 | 數學函式：macOS 內建 Apple libm（`libsystem_m.dylib`）；`_Float128` 用 GCC 附的 libquadmath |
| 分析工具 | `lldb`（反組譯 libm）、Python 3.13 + mpmath 1.4.1（Q2 的高精度參考值） |
| 浮點相關巨集 | `__LDBL_MANT_DIG__ = 53`、`__FP_FAST_FMA = 1`、`__FLT_EVAL_METHOD__ = 0` |
| Q5 對照機 | Windows 10 Pro（19045），Intel i5-12400F，winlibs MinGW-w64 GCC 16.2.0（i686，32 位元） |



## Q1



### 實驗介紹
主要測試 5 種浮點數型別：
| 型別 | 位元 | 尾數位元 | 在 Apple M3 + GCC 16 上的實作 |
|---|---|---|---|
| `_Float16` | 16 | 11 | 硬體支援，但 GCC 預設 target（armv8-a）沒開 FP16 算術，會先轉成 float 算再轉回，因此另外測了加上 `-march=armv8.2-a+fp16` 的版本用硬體計算 FP16 |
| `float` | 32 | 24 | 硬體 |
| `double` | 64 | 53 | 硬體 |
| `long double` | 64 | 53 | macOS arm64 ABI 定義為與 `double` 相同 |
| `_Float128` | 128 | 113 | 純軟體（libgcc 的 `__addtf3` 等函式；`sqrt`/`sin` 用 libquadmath） |

- **操作變因**：資料型別。程式用 template 寫，五種型別跑的是同一份程式碼。實驗二另外多比較最佳化等級 `-O2` 與 `-O3`（差別在 GCC 是否向量化）。
- **控制變因**：同一台機器（Apple M3，macOS 15.6.1）、同一個編譯器（Homebrew GCC 16.2.0，`g++-16 -std=c++17`）、相同的計時方式（`std::chrono::steady_clock`，報告每次運算的平均 ns）。運算結果寫進 `volatile` 變數，避免編譯器把整段運算刪掉。
- **無法完全控制的變因**：
  - M3 有 P-core E-core，沒辦法指定程式跑在哪顆核心，由 macOS 排程決定。
  - CPU 時脈會動態調整，所以只報告 ns，不換算成 cycle。
  - 應對方式：每個數字是數千萬～2 億次運算的平均（`_Float128` 太慢，次數減為 1/20），並重跑兩次確認差異在 2% 以內。


### 實驗一
- **實驗設計**：每一步需依賴上一步結果，從而使 CPU 不會對運算進行平行化，觀察單一浮點數指令的延遲
- **實驗目的**：硬體 FPU 處理不同寬度的數字，速度有差嗎？沒有硬體支援的型別慢多少？

```cpp
for (int i = 0; i < n; ++i) x = x + d;
```

### 實驗二

- **實驗設計**：在「元素彼此獨立」的陣列運算裡，每個元素的平均處理時間，並比較編譯器有沒有做 SIMD 和向量化優化。
- **實驗目的**：常聽說「float 比 double 快一倍」，想試著探討看看原因。

```cpp
for (int i = 0; i < n; ++i) a[i] = a[i] * k + b[i];
```

### 實驗三
- **實驗設計**：呼叫數學函式 `sin` 的每次呼叫時間。
- **實驗目的**：函式庫對不同型別的實作有差嗎？

```cpp
for (int i = 0; i < n; ++i) { acc += sin(x); x += T(1e-2); }
```

### 實驗結果
`g++-16 -std=c++17 -O2 q1_bench.cpp -o q1_bench -lquadmath && ./q1_bench`


因為 `float128` 太慢，所以下圖 y 軸是用對數尺度。
![image](https://hackmd.io/_uploads/HySq1JEsGl.png)

#### 實驗一 (a)
**結論**：有硬體支援的三種型別——加法、乘法一樣快，除法、開根號 float 快約 20%。
**分析**：加法與乘法是硬體一次算完的電路，32 或 64 bits 都是相同 cycle 數。除法與開根號是「每個 cycle 算出幾位」的迭代電路，位數越多步驟越多，所以 float（24 bits）比 double（53 bits）快一些。

#### 實驗一 (b)
**結論**：`_Float16` 預設比 float 慢 3.4 倍，開啟硬體 FP16 後就和 float 一樣快。
**分析**：GCC 預設的 target 不包含 FP16 算術，每次加法要做「轉 float, add, 轉回 half」三條相依指令，所以 2.25 ns ≈ 3 × 0.67 ns。
加上 `-march=armv8.2-a+fp16` 後組語變成一條 `fadd h0, h0, h1`，加法時間 0.673 ns 與 float 相同，除法與開根號則比 float 再快一些（位數更少）。

#### 實驗一 \(c)
**結論**：`_Float128` 慢 10～25 倍，`sin` 慢 190 倍。
**分析**：沒有硬體支援，每個運算都是函式呼叫、用整數指令模擬 113 bits 的尾數。開根號與 `sin` 要跑多次迭代，差距更大。

#### 實驗二
**結論**：沒向量化時 float / double / long double 一樣快；向量化後 float 剛好快一倍。

**分析**：`-O2`：GCC 沒有把 float / double 的迴圈向量化（組語只有純量 `fmadd`），三者都是 0.25 ns——與實驗一的結論一致：**單一運算的速度與寬度無關**。
`-O3`：GCC 使用 NEON SIMD 指令 `fmla`，float 版一條指令算 4 個元素、double 版算 2 個，所以每個元素的時間差一倍（0.065 vs 0.131）。


#### 實驗三
**結論**：`sinf` 比 `sin` 快 1.4 倍；`_Float16` 沒有自己的 `sin`。**

**分析**：float 與 double 都呼叫 macOS 的 libm，float 版只需約 24 bits 準確度，用較低階的多項式即可。`_Float16` 沒有專屬的函式庫實作，實際上是轉成 float 呼叫 `sinf` 再轉回，所以比 `sinf` 還慢（2.52 vs 1.82 ns）。`_Float128` 的 `sinq` 是 libquadmath 的純軟體實作，478 ns。

## Q2

分析的是我電腦上 `std::sin` 實際執行的程式碼。GCC 在 macOS 沒有自己的數學函式庫，`nm -u` 顯示程式呼叫的 `_sin` 來自 `/usr/lib/libSystem.B.dylib`，也就是 Apple libm（閉源）。因此用 `lldb` 反組譯來分析，並用 50 位精度的 `mpmath` 驗證誤差。

### 規格

C/C++ 標準要求：`sin(±0) = ±0`、`sin(±∞) = NaN` 且觸發 `FE_INVALID`、`sin(NaN) = NaN`。**標準不規定精度**，IEEE 754 只「建議」正確捨入（結果是最接近真值的 double）。實測（`q2_sin_test.cpp`，`g++-16 -std=c++17 -O2`）：

```
sin(0         ) = 0                        exc:
sin(-0        ) = -0                       exc:
sin(inf       ) = nan                      exc: INVALID
sin(nan       ) = nan                      exc:
sin(1e-310    ) = 9.9999999999999694e-311  exc: INEXACT UNDERFLOW
sin(3.1415926535897931) = 1.2246467991473532e-16    ← 「最接近 π 的 double」不是 π，函式庫正確算出差值
sin(1e+22)              = -0.85220084976718879      ← 經典大引數測試，早期函式庫常算錯
```

全部符合標準。

### 演算法與 range reduction（反組譯結果）

```
$ lldb -b -o "disassemble -n sin" -o "memory read --format f --size 8 <位址>" ./q2_sin_test
```

函式開頭的常數透露了結構：`π/4`、`524288.6（≈2¹⁹）` 兩個門檻，`2/π`，以及拆成三段的 `π/2`（`1.5707963267923333` + `2.56e-12` + `1.06e-23`），還有一個 64-bit 整數 `0xc90fdaa22168c235 = π/2 × 2⁶³`。依 |x| 大小分三條路：

```
|x| ≤ π/4          → 不歸約，直接算多項式

|x| < 2¹⁹          → Cody–Waite：k = round(x·2/π)；r = ((x − k·C1) − k·C2) − k·C3
                     π/2 拆三段逐一扣除，避免 x − k·π/2 的大數相減吃掉精度
                     k 的最低兩個 bit 決定象限（選 sin/cos 核心、是否變號）

|x| ≥ 2¹⁹          → Payne–Hanek：依 x 的指數從「2/π 的位元表」取出 192 bits，
                     用 64×64→128 位元整數乘法算出 x·2/π 的小數部分（完全沒有浮點誤差），
                     再乘回 π/2 轉成 double
```

需要第三種是因為`1e22` 的 ulp 約 $2×10^6$，用 double 算 `x − k·π/2` 餘數會被整個吃掉；三段式 π/2 只能撐到 $k ≈ 2^{19}$，再大就必須用整數算術。

### 數值近似

歸約後的 $r ∈ [−π/4, π/4]$，用多項式逼近。從記憶體讀出的係數：

```
sin 核心：-0.1666666666666663  0.0083333333333221182  -1.9841269829589539e-4  2.7557e-6  -2.5051e-8  1.5896e-10
cos 核心：-0.5  0.041666666666666595  -0.0013888888888873056  2.4802e-5  -2.7557e-7  2.0876e-9  -1.1359e-11
```

係數接近但不等於 $1/n!$（如 $1/5! = 0.008333333333333333$，這裡是 …333**221**），代表是 **minimax（Remez）多項式**而非泰勒展開，係數經過調整，讓區間內最大誤差最小。sin 用到 $x^{13}$、cos 用到 $x^{14}$，以 Horner 法展開：$x + x^3·(c_1 + x^2·(c₂ + …))$。

### 特殊硬體指令

| 指令 | 用途 |
|---|---|
| `fmadd` / `fnmsub` | **FMA**，乘加只捨入一次；Horner 每一步只損失半個 ulp，是精度的基礎 |
| `frintn` / `fcvtns` | 硬體 round-to-nearest，一條指令完成 `k = round(x·2/π)` |
| `umulh` | 64×64→128 位元乘法取高位，Payne–Hanek 的整數歸約 |
| `ucvtf d, x, #imm` | 定點整數直接轉 double（含縮放） |
| `eor`（NEON） | 位元運算套符號，避免分支 |


### 精度實測

因為要比 double 更精準才有意義，所以用 `mpmath`（50 位）當真值，三條路徑各隨機取 20,000 點，量 ulp 誤差（`q2_sin_check.py`）：

```
|x| <= pi/4 (no reduction)         max err = 0.6575 ulp; not correctly rounded: 514/20000 (2.57%)
pi/4 < |x| < 5e5 (Cody-Waite)      max err = 0.8348 ulp; not correctly rounded: 818/20000 (4.09%)
|x| >= 5e5 (Payne-Hanek)           max err = 0.8407 ulp; not correctly rounded: 849/20000 (4.25%)
sin(1e+22)  err = 0.0611 ulp
```


最大誤差 0.84 ulp，但約 3～4% 的結果不是正確捨入（> 0.5 ulp）。

### 結論
1. 我的機器上 `std::sin` 是 Apple libm 的實作，特殊值與例外旗標完全符合 C/C++ 標準。
2. 演算法是「三段式 range reduction（直接算 / Cody–Waite / Payne–Hanek）+ minimax 多項式」，全程用 FMA、硬體捨入與 128 位元整數乘法等指令，沒有用任何硬體 sin 指令。
3. 精度：抽樣 6 萬點最大 0.84 ulp，但約有 4% 不是正確捨入，這個標準沒要求且 Apple 也沒保證，所以不同平台的 libm 最後一位可能會不同。

## Q3

### `-ffast-math` 

`-ffast-math` 是一組「允許編譯器不遵守 IEEE 754 / C++ 標準浮點規則」的選項。
```bash
$ diff <(g++-16 -Q --help=optimizers -O2) <(g++-16 -Q --help=optimizers -O2 -ffast-math)
```

| 子選項 | 狀態 | 意思 |
|:--|:-:|:--|
| `-fassociative-math` | enabled | 允許重新結合：`(a+b)+c → a+(b+c)` |
| `-freciprocal-math` | enabled | 允許 `a/b → a*(1/b)` |
| `-ffinite-math-only` | enabled | 假設沒有 NaN、沒有 Inf |
| `-fsigned-zeros` | disabled | 不區分 `+0` 與 `-0` |
| `-fmath-errno` | disabled | 數學函式不設 `errno`（`sqrt` 可以直接變成一條指令） |
| `-ftrapping-math` | disabled | 假設浮點運算不會觸發例外 |
| `-funsafe-math-optimizations` | enabled | 上面幾項的總開關 |

並定義巨集 `__FAST_MATH__` 和 `__FINITE_MATH_ONLY__`。

而他改變的會是語意，讓很多 IEEE 保證成立的規則從此不成立。


### 實驗

同一份 code，參數使用`g++-16 -std=c++17 -O2`，只差在 `-ffast-math`。

主要挑了三個函式來測試，其中加上 `noinline` 是為了避免常數被折疊。

```cpp
__attribute__((noinline)) double assoc(double a, double b, double c) { return (a + b) + c; }
__attribute__((noinline)) bool   is_nan(double x)  { return std::isnan(x); }
__attribute__((noinline)) double div_x_x(double x) { return x / x; }

int main() {
    SHOW(assoc(1e16, -1e16, 1.0));   // (1e16 + -1e16) + 1 = 1；若改成 1e16 + (-1e16 + 1) 就是 0
    SHOW(is_nan(NAN));
    SHOW(div_x_x(0.0));              // 0/0 應該是 NaN
}
```

### 結果

```bash
$ g++-16 -std=c++17 -O2             q3_fastmath.cpp -o q3_strict
$ g++-16 -std=c++17 -O2 -ffast-math q3_fastmath.cpp -o q3_fast
```

| 測試 | 沒開 `-ffast-math` | 開 `-ffast-math` | 對應的子選項 |
|:--|:-:|:-:|:--|
| `assoc(1e16, -1e16, 1.0)` | **1** | **0** | `-fassociative-math` |
| `is_nan(NAN)` | **1** | **0** | `-ffinite-math-only` |
| `div_x_x(0.0)` | **nan** | **1** | `-ffinite-math-only` |

組語（`g++-16 -O2 -S`）：

| 函式 | 沒開 | 開 `-ffast-math` |
|:--|:--|:--|
| `assoc` | `fadd d0,d0,d1` → `fadd d0,d0,d2`（先算 a+b） | `fadd d1,d1,d2` → `fadd d0,d1,d0`（先算 b+c） |
| `is_nan` | `fcmp d0, d0` → `cset w0, vs` | `mov w0, 0`（直接回傳 false） |
| `div_x_x` | `fdiv d0, d0, d0` | `fmov d0, 1.0`（直接回傳 1） |


### 解讀

- `assoc`：結果從 1 變 0
`(1e16 + (-1e16)) + 1.0` 正確答案是 1。fast-math 允許重新結合，編譯器改成先算 `(-1e16) + 1.0`——但 1e16 的 ulp 是 2，加 1 進不去，結果還是 `-1e16`，最後 `1e16 + (-1e16) = 0`。數學上結合律成立，浮點數不成立。

- `is_nan`：永遠回傳 false
`-ffinite-math-only` 讓編譯器假設 NaN 不存在，整個函式被編成 `mov w0, 0`。任何靠 `isnan` 檢查輸入的程式，在 fast-math 下形同沒檢查。

- `div_x_x`：0/0 變成 1
同樣的假設下，編譯器認為 `x/x` 永遠等於 1，連除法都不做。



### 結論


1. `-ffast-math` 是「一組」選項，核心是允許重新結合、假設沒有 NaN/Inf、不區分正負零、不設 errno。用 `g++ -Q --help=optimizers` 可以讓 GCC 自己列出來。
2. 會改變程式語意：實測 `(a+b)+c` 從 1 變 0、`isnan(NaN)` 回傳 false、`0/0` 回傳 1，組語顯示編譯器直接把這些函式折疊成常數。
3. 只適合輸入保證有限、不依賴 NaN/Inf、結果可以容忍最後幾位不同的程式。若只是想讓 `sqrt` inline 或讓迴圈向量化，應個別開 `-fno-math-errno` 或 `-fassociative-math`，而不是整包 `-ffast-math`。

## Q4


### 實驗

同一個 double 分別用 `printf("%.9f")` 和 `setprecision(9)` 印出，並用 `%.25f` 印出它實際存的值來對照。`g++-16 -std=c++17 -O2`。

```cpp
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
```

### 結果

| 程式裡寫的 | `printf %.9f` | `setprecision(9)` | 實際存的值 |
|:--|:-:|:-:|:--|
| `0.1234567895` | 0.12345678**9** | 0.12345678**9** | 0.12345678**94999**99970728766 |
| `1.0000000005` | 1.00000000**1** | 1.00000000**1** | 1.00000000**05000**00413701855 |
| `2.5`（`%.0f`） | **2** | | 2.5（精確） |
| `3.5`（`%.0f`） | **4** | | 3.5（精確） |



### 分析解釋

- 反例 1：`0.1234567895` 沒有進位。
看第 10 位是 5，直覺上要四捨五入。但實際存的值是 0.1234567894999…，第 10 位是 4，所以 `printf` 印 …789 是對的。
- 反例 2：`1.0000000005` 有進位。
1.0000000005000000**4**…，比一半多一點點，所以進位成 …001。

- 額外測試：`2.5` 印成 `2`，`3.5` 印成 `4`。
是因為 2.5 和 3.5 在二進位是精確的，剛好在中間，這時 `printf` 不是「五入」，而是最近的取偶數那邊（2.5 → 2、3.5 → 4）。因為這是 IEEE 754 的預設捨入規則，目的是讓大量數字捨入後的誤差不會都偏向同一邊。

- `setprecision(9)` 和 `printf` 結果完全一樣，因為 libstdc++ 底層就是呼叫 `snprintf`。

### 結論


1. `%.9f` 和 `setprecision(9)` 做的是「把實際存的值捨入到 9 位，剛好一半時取偶數」，兩者行為相同，而且對實際存的值來說是正確的。
2. 但它不是對我們表面看到的十進位數做四捨五入，且面對尾數為 5 時會有進位或捨去至最近偶數的處理。
https://en.wikipedia.org/wiki/Rounding#Rounding_half_to_even


## Q5

同一份 `q5_portable.cpp`，編譯參數`g++ -std=c++17 -O2`，分別在兩台機器編譯執行。

| | macOS | Windows |
|:--|:--|:--|
| CPU | Apple M3（arm64，64 位元程式） | Intel i5-12400F（x86，32 位元） |
| OS | macOS 15.6.1 | Windows 10 Pro 19045 |
| 編譯器 | GCC 16.2.0（Homebrew） | GCC 16.2.0（winlibs MinGW-w64，i686） |

### 實驗設計

```cpp
// q5_portable.cpp（節錄）
__attribute__((noinline)) double mul_sub(double a, double b, double c) { return a * b - c; }   // a*b 會不會先被捨入？
__attribute__((noinline)) double mul_only(double a, double b)          { return a * b; }       // 先存起來：一定先捨入
__attribute__((noinline)) double add3(double a, double b, double c)    { return (a + b) + c; } // 中間值用什麼精度算？

double a = 1.0 + 0x1p-27, b = 1.0 - 0x1p-27;   // a*b = 1 - 2^-54，double 存不下，捨入後剛好是 1
double p = mul_only(a, b);
SHOW(mul_sub(a, b, p));                          // a*b 不先捨入 → -2^-54；先捨入 → 1 - 1 = 0
SHOW(add3(1e16, 1.0, -1e16));                    // double 算：1e16+1 存不下 → 0；80-bit 中間值：存得下 → 1
std::printf("%g\n", (double)(((1.0L + 1e-18L) - 1.0L) * 1e18L));   // long double 精度
SHOW(std::exp(0.1)); SHOW(std::pow(2.1, 3.3)); SHOW(std::sin(1e22)); SHOW(std::tgamma(5.5));
std::printf("%.0f %.0f %.0f %.0f\n", 0.5, 1.5, 2.5, 3.5);
```

### 結果

**平台設定**（程式用 `<cfloat>` 的巨集印出來的，是後面結果不同的原因）：

| 巨集 | 意思 | macOS M3 | Windows 32-bit |
|:--|:--|:--|:--|
| `FLT_EVAL_METHOD` | `double` 運算的中間值用什麼精度算：0 = 就用 double；2 = 用 `long double`（x87 的 80 位元） | 0 | **2** |
| `LDBL_MANT_DIG` | `long double` 的尾數有幾個 bit（53 = 和 double 一樣；64 = 80 位元格式） | 53 | **64** |
| `FP_FAST_FMA` | 有定義代表 CPU 有 FMA 指令（乘加一次完成、中間不捨入） | defined | not defined |

**計算結果**：

| 項目 | macOS M3 | Windows 32-bit | 相同？ |
|:--|:--|:--|:-:|
| `mul_sub(a, b, p)` = a·b − c | −5.55e-17 | −5.55e-17 | Yes（但原因不同，見下） |
| `add3(1e16, 1, −1e16)` | **0** | **1** | No |
| `((1 + 1e-18) − 1) × 1e18`（long double） | **0** | **0.975782** | No |
| `exp`、`pow`、`sin(1e22)`、`tgamma` 的 bits | 相同 | 相同 | Yes |
| `%.0f` of 0.5 1.5 2.5 3.5 | 0 2 2 4 | 0 2 2 4 | Yes |

**反向驗證**：Windows 上加 `-mfpmath=sse -msse2`（改用 SSE 單元算 double，不走 x87）後：`FLT_EVAL_METHOD` 變 0、`add3` 變 **0**、`mul_sub` 變 **0**；`long double` 仍是 0.975782。

### 解釋

**`add3`：Mac 算出 0，Windows 算出 1（硬體差異）**
32 位元 x86 的 GCC 預設用 **x87 浮點單元**，它的暫存器是 80 位元，`FLT_EVAL_METHOD = 2` 的意思就是「所有 double 運算的中間值都用 80 位元算」。`1e16 + 1` 在 double 存不下（1e16 的 ulp 是 2），但在 80 位元存得下，所以減回 1e16 後剩 1。Mac 用 64 位元算，1e16 + 1 當場被捨入成 1e16，剩 0。**同一行 `double` 程式碼，中間值的精度不一樣。** 加 `-mfpmath=sse` 強迫 Windows 用 SSE 單元後結果變 0，證實差異來自浮點單元。

**`mul_sub`：兩邊都是 −5.55e-17，但原因不同**
Mac 有 FMA 指令，`a*b − c` 被收縮成一條 `fnmsub`，a·b 不經過捨入。Windows 沒有 FMA，但 x87 用 80 位元算 a·b，1 − 2⁻⁵⁴ 剛好存得下，同樣沒被捨入。兩種不同的硬體機制碰巧給出相同答案。反向驗證證實這點：Windows 改用 SSE（既無 80 位元、也無 FMA）後，a·b 先被捨入成 1，結果變成 **0**。

**`long double`：0 vs 0.975782（ABI 差異）**
macOS arm64 把 `long double` 定義成 64 位元 double（53 bits 尾數），1 + 1e-18 直接等於 1。x86 的 GCC 把它定義成 80 位元 x87 格式（64 bits 尾數、`sizeof = 12`），1e-18 能留下來（不是剛好 1，因為 1e-18 本身也被捨入成 2⁻⁶³ 的整數倍）。這是 ABI 的規定，和用哪個浮點單元無關，所以加 `-mfpmath=sse` 後也不會變。

**數學函式庫與 printf：相同**
這四個輸入在 Apple libm 和 MinGW 的 libm 碰巧都給出相同的 bits，`%.0f` 的 tie 也都取偶數。Q2 已說明 libm 不保證正確捨入，換其他輸入仍可能不同，但在這個實驗裡沒有觀察到。

### 結論

1. 同一份程式、同一個 GCC 16.2.0、同一組旗標，macOS M3 與 Windows 32 位元 x86 上有 **2 個數值結果不同**：`(1e16+1)−1e16` 得 0 vs 1；`long double` 得 0 vs 0.976。三個平台巨集（`FLT_EVAL_METHOD`、`LDBL_MANT_DIG`、`FP_FAST_FMA`）事先就預告了這些差異。
2. 差異來源有兩層：**硬體**（x87 的 80 位元中間值 vs 64 位元 SIMD 單元；有無 FMA）、ABI（`long double` 是 64 還是 80 bits）。`mul_sub` 則示範了兩種不同機制可能碰巧給出相同結果，單看一個測試會誤判；在 Windows 上改用 SSE 後三種情況（FMA、x87、純 SSE）才完全分開。
3. 要讓浮點結果跨平台一致，光鎖住編譯器版本和旗標不夠：x86 要加 `-mfpmath=sse`（避開 x87）、加 `-ffp-contract=off`（關掉 FMA 收縮）、避免用 `long double`。



## Q6
從一開始剛接觸程式時，我被灌輸的印象只有「浮點數有誤差」、「浮點數運算很慢」、「沒事不要用浮點數」，在以往寫題目時確實也都不會遇到相關題目。再到大二時學到計算機結構，了解浮點數在電腦中的表示法，才了解誤差與延遲是如何而來。

經過這次作業分析後，我才了解到實作上還有更多細節需要注意，比如在 Mac 和 Windows 上`long double` 會有區別，若是用此型態撰寫的軟體放到不同裝置上，導致印出的數據有差可能就慘了。

還有大家可能只聽過 IEEE754，卻不知道 IEEE 有很多其他相關規範，這些也都很值得反思。

並且這些浮點數知識在未來 HPC、AI 的計算上會更加重要，畢竟每次計算的誤差都會因為下次迭代而放大。

### 額外例子
我們學過在浮點數中有 `NaN` 的情況，但一但它放入 `std::sort` 就會導致未定義行為。
`NaN` 和任何數比較都是 false，包括和自己：`NaN < x`、`x < NaN`、`NaN == NaN` 全部是 false。這會破壞 C++ 標準容器與演算法依賴的「嚴格弱序」前提：

| 情境 | 後果 |
|:--|:--|
| `std::sort(v.begin(), v.end())`，`v` 裡有 NaN | **未定義行為**。libstdc++ 的 introsort 曾因此寫到陣列外而 crash，不只是「排得不對」 |
| `std::set<double>` / `std::map<double, T>` 放入 NaN | 等價關係崩壞，可能找不到自己剛插入的鍵 |
| `std::min(a, NaN)` vs `std::min(NaN, a)` | 一個回傳 `a`、一個回傳 NaN，結果取決於參數順序；`std::fmin` 才會一致地忽略 NaN |

由於 `NaN` 可能會在使用 sort 或存入容器時才發現，可以讓資料在進入容器前用 `std::isnan` 過濾，然而 Q3 已經證明，一旦開了 `-ffast-math`，連這道檢查都會被編譯器拿掉，讓程式壞掉。

## Q7
- 我在實驗設計上搭配我自己設計想法的 prompt 給 AI，請他建立對應的實驗 code。
- 我請 AI 幫我實驗產生的數據做成比較好看一點的表格。
- 在實驗設計時，我請 AI 幫我想一些我自己想不到的實驗測試內容。
- 我請 AI 幫我分析總結部分實驗結果，還有給出我不知道原因的解釋。
- Q5 因為我不太熟，所以這部分絕大部分是 AI 生成，但我有自己讀過確認內容。