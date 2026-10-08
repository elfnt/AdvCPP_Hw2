# Q1: 把 q1_bench 的結果畫成圖   usage: python3 q1_plot.py  -> q1_chart.png
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager
import numpy as np

# ---- 數據（g++-16 -std=c++17，Apple M3，ns/op）----
types = ["_Float16", "float", "double", "long double", "_Float128"]
ops   = ["add", "mul", "div", "sqrt"]
latency = {                      # 實驗一，-O2
    "_Float16":    [2.252, 2.490, 3.470, 6.194],
    "float":       [0.668, 0.994, 1.986, 3.230],
    "double":      [0.671, 0.995, 2.477, 3.969],
    "long double": [0.673, 0.993, 2.481, 3.964],
    "_Float128":   [7.612, 13.683, 29.957, 91.724],
}
axpy_O2 = [0.064, 0.250, 0.251, 0.251, 11.446]   # 實驗二
axpy_O3 = [0.064, 0.065, 0.131, 0.131, 11.400]
sin_ns  = [2.517, 1.817, 2.557, 2.514, 478.295]  # 實驗三
half_fp16 = {"add": 0.673, "div": 1.741, "sqrt": 2.729, "axpy": 0.033}  # -march=armv8.2-a+fp16

# ---- 樣式 ----
for f in ["PingFang TC", "Heiti TC", "Noto Sans CJK TC"]:
    if any(f in x.name for x in font_manager.fontManager.ttflist):
        plt.rcParams["font.family"] = f; break
plt.rcParams.update({"axes.spines.top": False, "axes.spines.right": False,
                     "axes.edgecolor": "#c3c2b7", "axes.labelcolor": "#52514e",
                     "xtick.color": "#52514e", "ytick.color": "#898781",
                     "grid.color": "#e1e0d9", "font.size": 10})
colors = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4"]   # 固定順序對應五種型別
ink = "#0b0b0b"

def label(ax, bars, fmt="{:.2f}"):
    for b in bars:
        h = b.get_height()
        ax.annotate(("{:.1f}" if h >= 10 else fmt).format(h), (b.get_x() + b.get_width() / 2, b.get_height()),
                    xytext=(0, 2), textcoords="offset points", ha="center", va="bottom", fontsize=7.5, color=ink)

fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(15, 4.6), facecolor="#fcfcfb",
                                    gridspec_kw={"width_ratios": [4, 2.6, 1.6]})
for ax in (ax1, ax2, ax3):
    ax.set_facecolor("#fcfcfb"); ax.set_yscale("log"); ax.grid(axis="y", lw=0.6); ax.set_axisbelow(True)

# 實驗一
x = np.arange(len(ops)); w = 0.16
for i, t in enumerate(types):
    bars = ax1.bar(x + (i - 2) * w, latency[t], w * 0.9, color=colors[i])
    label(ax1, bars)
ax1.set_xticks(x, ops); ax1.set_ylabel("每次運算時間 (ns，對數刻度)")
ax1.set_title("實驗一：單一運算的延遲（相依鏈，-O2）", loc="left", color=ink)
ax1.set_ylim(0.3, 300)
from matplotlib.patches import Patch   # label 以 "_" 開頭會被 matplotlib 的 legend 忽略，所以手動建
ax1.legend([Patch(color=c) for c in colors], types, frameon=False, ncol=5, fontsize=8, loc="upper left")

# 實驗二
x = np.arange(len(types)); w = 0.38
b1 = ax2.bar(x - w / 2, axpy_O2, w * 0.92, color="#9ec5f4", label="-O2（未向量化）")
b2 = ax2.bar(x + w / 2, axpy_O3, w * 0.92, color="#2a78d6", label="-O3（向量化）")
label(ax2, b1, "{:.3f}"); label(ax2, b2, "{:.3f}")
ax2.set_xticks(x, types, fontsize=8); ax2.set_ylim(0.02, 40)
ax2.set_title("實驗二：陣列 a[i]=a[i]*k+b[i] 每元素時間", loc="left", color=ink)
ax2.legend(frameon=False, fontsize=8, loc="upper left")

# 實驗三
bars = ax3.bar(types, sin_ns, 0.6, color=colors)
label(ax3, bars, "{:.1f}"); ax3.set_ylim(0.8, 2000)
ax3.set_xticks(range(len(types)), types, fontsize=8, rotation=20, ha="right")
ax3.set_title("實驗三：sin(x) 每次呼叫", loc="left", color=ink)

fig.text(0.01, 0.01, "Apple M3 · macOS 15.6.1 · Homebrew GCC 16.2.0 (g++-16 -std=c++17) · "
         f"_Float16 加上 -march=armv8.2-a+fp16 後：add {half_fp16['add']}、div {half_fp16['div']}、sqrt {half_fp16['sqrt']}、axpy {half_fp16['axpy']} ns",
         fontsize=8, color="#52514e")
fig.tight_layout(rect=(0, 0.04, 1, 1))
fig.savefig("q1_chart.png", dpi=200)
print("saved q1_chart.png")
