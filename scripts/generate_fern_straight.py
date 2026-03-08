"""Generate 3 STRAIGHT fern variants based on CONFIRMED coefficients.
Straightened (T2 b=0, c=0) + moderate upward pinnae curl.
Curl via: slightly higher c in T3/T4 (pinnae angle) and slightly higher a/d (compound detail)."""
import random
import numpy as np
from scipy.ndimage import gaussian_filter, label
from PIL import Image, ImageFilter

W, H = 1024, 1024
N_POINTS = 4_000_000
SIGMA = 0.7
ALPHA_THRESHOLD = 25
MIN_CLUSTER_PX = 50

VARIANTS = [
    {
        "name": "T_Fern_Straight_01",
        "desc": "Confirmed_01 base — diamond, slight upward curl",
        "seed": 601,
        # Base Confirmed_01 straightened. Original a/d=0.05, c=0.18.
        # Mild bump: c 0.18->0.21, a/d 0.05->0.06. Keep thin.
        "transforms": [
            (0.0,   0.0,   0.0,   0.20,  0.0,  -0.12, 0.01),
            (0.93,  0.0,   0.0,   0.90,  0.0,   0.55, 0.84),
            (0.06, -0.22,  0.21,  0.06, -0.08,  0.02, 0.09),
            (-0.06, 0.22,  0.21,  0.06,  0.08,  0.12, 0.06),
        ],
    },
    {
        "name": "T_Fern_Straight_02",
        "desc": "Confirmed_02 base — cyclosorus, slight curl added",
        "seed": 602,
        # Base Confirmed_02. Original a=0.04/d=0.045, c=0.16.
        # Mild bump: c 0.16->0.19, a/d 0.04->0.05. Keep thin.
        "transforms": [
            (0.0,   0.0,   0.0,   0.25,  0.0,  -0.14, 0.02),
            (0.95,  0.0,   0.0,   0.93,  0.0,   0.5,  0.84),
            (0.05, -0.18,  0.19,  0.05, -0.08,  0.02, 0.07),
            (-0.05, 0.18,  0.19,  0.05,  0.08,  0.12, 0.07),
        ],
    },
    {
        "name": "T_Fern_Straight_03",
        "desc": "Confirmed_03 base — bushy oval, longer pinnae + curl",
        "seed": 603,
        # Base Confirmed_03 straightened. Longer pinnae: b 0.16->0.20.
        # Slightly lower c 0.26->0.23 so they don't all go straight up.
        # a/d 0.05->0.06 for mild compound detail.
        "transforms": [
            (0.0,   0.0,   0.0,   0.20,  0.0,  -0.12, 0.01),
            (0.90,  0.0,   0.0,   0.88,  0.0,   0.7,  0.80),
            (0.06, -0.20,  0.23,  0.06, -0.07,  0.02, 0.095),
            (-0.06, 0.20,  0.23,  0.06,  0.07,  0.12, 0.095),
        ],
    },
]


def ifs_generate(transforms, seed):
    random.seed(seed)
    cum, total = [], 0.0
    for t in transforms:
        total += t[6]
        cum.append(total)
    x, y = 0.0, 0.0
    pts = []
    for _ in range(N_POINTS):
        r = random.random()
        for i, th in enumerate(cum):
            if r <= th:
                a, b, c, d, e, f, _ = transforms[i]
                xn = a * x + b * y + e
                yn = c * x + d * y + f
                x, y = xn, yn
                break
        pts.append((x, y))
    return pts


def rasterize(pts):
    xs, ys = zip(*pts)
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    pad = 30
    iw, ih = W - 2 * pad, H - 2 * pad
    sx = iw / (x1 - x0) if x1 != x0 else 1
    sy = ih / (y1 - y0) if y1 != y0 else 1
    s = min(sx, sy)
    ox = pad + (iw - (x1 - x0) * s) / 2
    oy = pad + (ih - (y1 - y0) * s) / 2
    density = np.zeros((H, W), dtype=np.float64)
    for px, py in pts:
        ix = int((px - x0) * s + ox)
        iy = H - 1 - int((py - y0) * s + oy)
        if 0 <= ix < W and 0 <= iy < H:
            density[iy, ix] += 1.0
    density = gaussian_filter(density, sigma=SIGMA)
    mx = density.max()
    if mx > 0:
        density /= mx
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    px = img.load()
    for row in range(H):
        for col in range(W):
            d = density[row, col]
            if d > 0.005:
                ry = 1.0 - row / H
                bri = int((150 + 70 * ry) * min(1.0, 0.7 + d * 0.3))
                bri = min(255, max(100, bri))
                a = min(255, int(d * 8 * 255))
                px[col, row] = (bri, bri, bri, a)
    for row in range(H):
        for col in range(W):
            r, g, b, a = px[col, row]
            px[col, row] = (r, g, b, 255) if a >= ALPHA_THRESHOLD else (0, 0, 0, 0)
    alpha = np.array(img.split()[3])
    labeled, nf = label((alpha > 0).astype(np.int32))
    if nf > 1:
        sizes = np.bincount(labeled.ravel())
        sizes[0] = 0
        px = img.load()
        for cid in range(1, nf + 1):
            if sizes[cid] < MIN_CLUSTER_PX:
                for row, col in zip(*np.where(labeled == cid)):
                    px[col, row] = (0, 0, 0, 0)
    img = img.filter(ImageFilter.GaussianBlur(radius=0.3))
    return img


def validate(img, name):
    alpha = np.array(img.split()[3])
    problems = []
    top = alpha[:int(H * 0.12), :]
    if top.size:
        mf = np.max(np.mean(top > 0, axis=1))
        if mf > 0.25:
            problems.append(f"top merge {mf:.0%}")
    fill = np.mean(alpha > 0)
    if fill > 0.30:
        problems.append(f"dense {fill:.0%}")
    if fill < 0.03:
        problems.append(f"sparse {fill:.0%}")
    ok = len(problems) == 0
    tag = "OK" if ok else "WARN"
    print(f"  [{tag}] {name}: fill={fill:.1%}" + (f" ({'; '.join(problems)})" if problems else ""))
    return ok


OUT = "E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover"
for v in VARIANTS:
    name = v["name"]
    print(f"Generating {name} -- {v['desc']}...")
    img = rasterize(ifs_generate(v["transforms"], v["seed"]))
    validate(img, name)
    out = f"{OUT}/{name}.png"
    img.save(out)
    print(f"  Saved: {out}")

print("\nDone! 3 straight ferns with moderate pinnae curl.")
