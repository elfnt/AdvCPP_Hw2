# Q2: measure the accuracy (in ulp) of this machine's sin() against a 50-digit reference
# usage: python3 q2_sin_check.py   (needs: pip install mpmath)
import ctypes, math, random, struct
import mpmath as mp

mp.mp.dps = 50
libm = ctypes.CDLL("libm.dylib")            # macOS: libsystem_m.dylib
libm.sin.restype, libm.sin.argtypes = ctypes.c_double, [ctypes.c_double]

def ulp_error(x):
    got = libm.sin(x)
    exact = mp.sin(mp.mpf(x))
    ulp = math.ulp(float(exact)) if exact != 0 else 5e-324
    return abs(mp.mpf(got) - exact) / ulp

random.seed(1)
ranges = {
    "|x| <= pi/4 (no reduction)": [random.uniform(-0.785, 0.785) for _ in range(20000)],
    "pi/4 < |x| < 5e5 (Cody-Waite)": [random.uniform(-5e5, 5e5) for _ in range(20000)],
    "|x| >= 5e5 (Payne-Hanek)": [random.uniform(-1e300, 1e300) ** 1 for _ in range(20000)],
    "near multiples of pi": [float(k * mp.pi) for k in range(1, 20001)],
}
for name, xs in ranges.items():
    errs = [ulp_error(x) for x in xs]
    worst = max(range(len(xs)), key=lambda i: errs[i])
    not_cr = sum(1 for e in errs if e > 0.5)
    print(f"{name:34s} max err = {float(errs[worst]):.4f} ulp at x={xs[worst]!r}; "
          f"not correctly rounded: {not_cr}/{len(xs)} ({100*not_cr/len(xs):.2f}%)")

# a few famous hard cases
for x in [1e22, float.fromhex("0x1.921fb54442d18p+1"), 5.0e-324, 2.0**60]:
    print(f"sin({x!r}) libm={libm.sin(x)!r}  exact={mp.nstr(mp.sin(mp.mpf(x)), 20)}  err={float(ulp_error(x)):.4f} ulp")
