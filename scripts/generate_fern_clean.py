"""Generate genuinely different fern shapes.
Confirmed ferns NOT touched. Bold coefficient changes for real shape variety."""
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
        "name": "T_Fern_Cyclosorus_01",
        "seed": 42,
        # SHORT & WIDE — very few rows, long pinnae, squat oval shape
        # body d=0.82 = heavy compression = fewer rows
        # pinnae b=-0.35 = very long pinnae
        "transforms": [
            {"a": 0.0,  "b": 0.0,  "c": 0.0,  "d": 0.20, "e": 0.0,  "f":-0.12, "p": 0.01},
            {"a": 0.90, "b": 0.0,  "c": 0.0,  "d": 0.82, "e": 0.0,  "f": 0.8,  "p": 0.84},
            {"a": 0.07, "b":-0.35, "c": 0.18, "d": 0.07, "e":-0.06, "f": 0.02, "p": 0.075},
            {"a":-0.07, "b": 0.35, "c": 0.18, "d": 0.07, "e": 0.06, "f": 0.12, "p": 0.075},
        ],
    },
    {
        "name": "T_Fern_Cyclosorus_02",
        "seed": 58,
        # CURVED ARCH — stem bends right, pinnae sweep left
        # body b=0.03, c=-0.03 = curve
        # asymmetric pinnae probabilities = one side denser
        "transforms": [
            {"a": 0.0,  "b": 0.0,  "c": 0.0,  "d": 0.20, "e": 0.0,  "f":-0.12, "p": 0.01},
            {"a": 0.93, "b": 0.03, "c":-0.03, "d": 0.90, "e": 0.0,  "f": 0.55, "p": 0.84},
            {"a": 0.05, "b":-0.22, "c": 0.18, "d": 0.05, "e":-0.08, "f": 0.02, "p": 0.09},
            {"a":-0.05, "b": 0.22, "c": 0.18, "d": 0.05, "e": 0.08, "f": 0.12, "p": 0.06},
        ],
    },
]


def ifs_generate(transforms, seed):
    random.seed(seed)
    cum, total = [], 0.0
    for t in transforms:
        total += t["p"]
        cum.append(total)
    x, y = 0.0, 0.0
    pts = []
    for _ in range(N_POINTS):
        r = random.random()
        for i, th in enumerate(cum):
            if r <= th:
                t = transforms[i]
                xn = t["a"]*x + t["b"]*y + t["e"]
                yn = t["c"]*x + t["d"]*y + t["f"]
                x, y = xn, yn
                break
        pts.append((x, y))
    return pts


def rasterize(pts):
    xs, ys = zip(*pts)
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    pad = 30
    iw, ih = W-2*pad, H-2*pad
    s = min(iw/(x1-x0) if x1!=x0 else 1, ih/(y1-y0) if y1!=y0 else 1)
    ox = pad + (iw-(x1-x0)*s)/2
    oy = pad + (ih-(y1-y0)*s)/2
    density = np.zeros((H, W), dtype=np.float64)
    for px, py in pts:
        ix = int((px-x0)*s+ox)
        iy = H-1-int((py-y0)*s+oy)
        if 0<=ix<W and 0<=iy<H:
            density[iy, ix] += 1.0
    density = gaussian_filter(density, sigma=SIGMA)
    mx = density.max()
    if mx > 0:
        density /= mx
    img = Image.new("RGBA", (W, H), (0,0,0,0))
    px = img.load()
    for row in range(H):
        for col in range(W):
            d = density[row, col]
            if d > 0.005:
                ry = 1.0 - row/H
                bri = int((150+70*ry)*min(1.0, 0.7+d*0.3))
                bri = min(255, max(100, bri))
                a = min(255, int(d*8*255))
                px[col, row] = (bri, bri, bri, a)
    for row in range(H):
        for col in range(W):
            r, g, b, a = px[col, row]
            px[col, row] = (r,g,b,255) if a>=ALPHA_THRESHOLD else (0,0,0,0)
    alpha = np.array(img.split()[3])
    labeled, nf = label((alpha>0).astype(np.int32))
    if nf > 1:
        sizes = np.bincount(labeled.ravel())
        sizes[0] = 0
        px = img.load()
        for cid in range(1, nf+1):
            if sizes[cid] < MIN_CLUSTER_PX:
                for row, col in zip(*np.where(labeled==cid)):
                    px[col, row] = (0,0,0,0)
    img = img.filter(ImageFilter.GaussianBlur(radius=0.3))
    return img


def validate(img, name):
    alpha = np.array(img.split()[3])
    problems = []
    top = alpha[:int(H*0.12), :]
    if top.size:
        mf = np.max(np.mean(top>0, axis=1))
        if mf > 0.25:
            problems.append(f"top merge {mf:.0%}")
    fill = np.mean(alpha>0)
    if fill > 0.30:
        problems.append(f"dense {fill:.0%}")
    if fill < 0.03:
        problems.append(f"sparse {fill:.0%}")
    ok = len(problems)==0
    print(f"  [{'OK' if ok else 'WARN'}] {name}: fill={fill:.1%}" + (f" ({'; '.join(problems)})" if problems else ""))
    return ok


for v in VARIANTS:
    name = v["name"]
    print(f"Generating {name} ...")
    img = rasterize(ifs_generate(v["transforms"], v["seed"]))
    validate(img, name)
    out = f"E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover/{name}.png"
    img.save(out)
    print(f"  Saved: {out}")
print("\nConfirmed ferns NOT touched.")
