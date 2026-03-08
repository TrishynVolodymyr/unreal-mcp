"""Generate diverse fern/leaf textures from different IFS coefficient families.
Uses genuinely different IFS formulas — not just Cyclosorus tweaks.
Confirmed_01 and Confirmed_02 are NOT touched."""
import random
import numpy as np
from scipy.ndimage import gaussian_filter, label
from PIL import Image, ImageFilter

W, H = 1024, 1024
N_POINTS = 4_000_000
SIGMA = 0.7
ALPHA_THRESHOLD = 25
MIN_CLUSTER_PX = 50

# Each variant uses a fundamentally different IFS coefficient family.
# Format: (a, b, c, d, e, f, probability)
VARIANTS = [
    {
        "name": "T_Fern_Atlas_01",
        "desc": "Fishbone — skeletal, tiny pinnae, thin spine",
        "seed": 101,
        "transforms": [
            (0.0,   0.0,    0.0,   0.25,  0.0,    -0.4,   0.02),
            (0.95,  0.002, -0.002, 0.93, -0.002,   0.5,   0.84),
            (0.035,-0.11,   0.27,  0.01, -0.05,    0.005,  0.07),
            (-0.04, 0.11,   0.27,  0.01,  0.047,   0.06,   0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_02",
        "desc": "Culcita — broad round tree-fern, wide pinnae angles",
        "seed": 102,
        "transforms": [
            (0.0,   0.0,   0.0,   0.25,  0.0,  -0.14, 0.02),
            (0.85,  0.02, -0.02,  0.83,  0.0,   1.0,  0.84),
            (0.09, -0.28,  0.3,   0.11,  0.0,   0.6,  0.07),
            (-0.09, 0.28,  0.3,   0.09,  0.0,   0.7,  0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_03",
        "desc": "Paul Bourke Leaf — multi-lobed, not a fern at all",
        "seed": 103,
        "transforms": [
            (0.0,    0.2439,  0.0,    0.3053, 0.0,    0.0,    0.10),
            (0.7248, 0.0337, -0.0253, 0.7426, 0.206,  0.2538, 0.45),
            (0.1583,-0.1297,  0.355,  0.3676, 0.1383, 0.175,  0.22),
            (0.3386, 0.3694,  0.2227,-0.0756, 0.0679, 0.0826, 0.23),
        ],
    },
    {
        "name": "T_Fern_Atlas_04",
        "desc": "1999 variant — 3-transform asymmetric spiral fern",
        "seed": 104,
        "transforms": [
            (0.01, -0.41,  0.39,  0.0, -0.28, -0.185, 0.10),
            (0.7,   0.33, -0.35,  0.7,  0.185, 0.015, 0.70),
            (0.0,   0.175, 0.013, 0.46,-0.095,-0.285, 0.20),
        ],
    },
    {
        "name": "T_Fern_Atlas_05",
        "desc": "Modified Barnsley — short wide pinnae, squat shape",
        "seed": 105,
        "transforms": [
            (0.0,   0.0,   0.0,   0.2,   0.0,  -0.12,  0.01),
            (0.845, 0.035,-0.035, 0.82,  0.0,   1.6,   0.85),
            (0.2,  -0.31,  0.255, 0.245, 0.0,   0.29,  0.07),
            (-0.15, 0.24,  0.25,  0.20,  0.0,   0.68,  0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_06",
        "desc": "Classic Barnsley — the original fractal fern",
        "seed": 106,
        "transforms": [
            (0.0,   0.0,   0.0,   0.16,  0.0,   0.0,   0.01),
            (0.85,  0.04, -0.04,  0.85,  0.0,   1.6,   0.85),
            (0.2,  -0.26,  0.23,  0.22,  0.0,   1.6,   0.07),
            (-0.15, 0.28,  0.26,  0.24,  0.0,   0.44,  0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_07",
        "desc": "Twig — asymmetric branching, 3 transforms",
        "seed": 107,
        "transforms": [
            (0.387, 0.430,  0.430, -0.387, 0.2560, 0.5220, 0.333),
            (0.441,-0.091, -0.009, -0.322, 0.4219, 0.5059, 0.333),
            (-0.468,0.020, -0.113,  0.015, 0.4,    0.4,    0.334),
        ],
    },
    {
        "name": "T_Fern_Atlas_08",
        "desc": "Windswept tree — 5-transform asymmetric canopy",
        "seed": 108,
        "transforms": [
            (0.195, -0.488, 0.344,  0.443, 0.4431, 0.2452, 0.2),
            (0.462,  0.414,-0.252,  0.361, 0.2511, 0.5692, 0.2),
            (-0.637, 0.000, 0.000,  0.501, 0.8562, 0.2512, 0.2),
            (-0.035, 0.070,-0.469,  0.022, 0.4884, 0.5069, 0.2),
            (-0.058,-0.070, 0.453, -0.111, 0.5976, 0.0969, 0.2),
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
    # Threshold alpha
    for row in range(H):
        for col in range(W):
            r, g, b, a = px[col, row]
            px[col, row] = (r, g, b, 255) if a >= ALPHA_THRESHOLD else (0, 0, 0, 0)
    # Remove tiny isolated clusters
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
    print(f"Generating {name} — {v['desc']}...")
    img = rasterize(ifs_generate(v["transforms"], v["seed"]))
    validate(img, name)
    out = f"{OUT}/{name}.png"
    img.save(out)
    print(f"  Saved: {out}")

print("\nDone! Confirmed_01 and Confirmed_02 were NOT touched.")
