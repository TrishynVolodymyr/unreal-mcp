"""Generate only Cyclosorus 01 and 02 — different shapes from 03.
03 is NOT touched. Must be genuinely different silhouettes."""
from PIL import Image, ImageFilter
import random
import numpy as np
from scipy.ndimage import gaussian_filter, label

WIDTH = 1024
HEIGHT = 1024
ITERATIONS = 3_000_000

VARIANTS = [
    {
        "name": "T_Fern_Cyclosorus_01",
        "seed": 50,
        # Wide diamond — pinnae slope steeper downward, wider spread
        # Lower d in t2 = fewer rows, bigger pinnae in t3/t4
        "transforms": [
            (0.0, 0.0, 0.0, 0.25, 0.0, -0.14, 0.02),
            (0.92, 0.0, 0.0, 0.87, 0.0, 0.65, 0.84),
            (0.07, -0.28, 0.24, 0.07, -0.07, 0.02, 0.07),
            (-0.07, 0.28, 0.24, 0.07, 0.07, 0.12, 0.07),
        ],
    },
    {
        "name": "T_Fern_Cyclosorus_02",
        "seed": 82,
        # Asymmetric — left pinnae longer than right, slight lean
        "transforms": [
            (0.0, 0.0, 0.0, 0.25, 0.0, -0.14, 0.02),
            (0.94, 0.0, 0.0, 0.91, 0.0, 0.55, 0.84),
            (0.06, -0.25, 0.22, 0.06, -0.08, 0.02, 0.07),   # left: longer
            (-0.04, 0.16, 0.15, 0.04, 0.08, 0.12, 0.07),     # right: shorter
        ],
    },
]


def validate_fern(img, name):
    alpha_arr = np.array(img.split()[3])
    issues = []

    top_region = alpha_arr[:int(HEIGHT * 0.15), :]
    top_rows_fill = np.mean(top_region > 0, axis=1)
    max_fill = np.max(top_rows_fill) if len(top_rows_fill) > 0 else 0
    if max_fill > 0.25:
        issues.append(f"Top merging: row fill {max_fill:.1%} (max 25%)")

    binary = (alpha_arr > 0).astype(np.int32)
    labeled, num_features = label(binary)
    if num_features > 1:
        component_sizes = np.bincount(labeled.ravel())
        component_sizes[0] = 0
        small_fragments = sum(1 for s in component_sizes[1:] if 0 < s < 50)
        if small_fragments > 0:
            issues.append(f"{small_fragments} isolated fragments (<50px)")

    fill_ratio = np.mean(alpha_arr > 0)
    if fill_ratio > 0.35:
        issues.append(f"Too dense: {fill_ratio:.1%} fill")
    elif fill_ratio < 0.05:
        issues.append(f"Too sparse: {fill_ratio:.1%} fill")

    if issues:
        print(f"  WARN [{name}]: {'; '.join(issues)}")
    else:
        print(f"  OK [{name}]: clean (fill {fill_ratio:.1%})")
    return len(issues) == 0


def generate_fern(variant):
    random.seed(variant["seed"])
    transforms = variant["transforms"]

    thresholds = []
    cumulative = 0.0
    for t in transforms:
        cumulative += t[6]
        thresholds.append(cumulative)

    points = []
    x, y = 0.0, 0.0

    for _ in range(ITERATIONS):
        r = random.random()
        for i, threshold in enumerate(thresholds):
            if r <= threshold:
                a, b, c, d, e, f, _ = transforms[i]
                x_new = a * x + b * y + e
                y_new = c * x + d * y + f
                x, y = x_new, y_new
                break
        points.append((x, y))

    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    x_min, x_max = min(xs), max(xs)
    y_min, y_max = min(ys), max(ys)

    pad = 30
    img_w = WIDTH - 2 * pad
    img_h = HEIGHT - 2 * pad

    scale_x = img_w / (x_max - x_min) if x_max != x_min else 1
    scale_y = img_h / (y_max - y_min) if y_max != y_min else 1
    scale = min(scale_x, scale_y)

    cx = pad + (img_w - (x_max - x_min) * scale) / 2
    cy = pad + (img_h - (y_max - y_min) * scale) / 2

    density = np.zeros((HEIGHT, WIDTH), dtype=np.float64)

    for px, py in points:
        ix = int((px - x_min) * scale + cx)
        iy = HEIGHT - 1 - int((py - y_min) * scale + cy)
        if 0 <= ix < WIDTH and 0 <= iy < HEIGHT:
            density[iy, ix] += 1.0

    density = gaussian_filter(density, sigma=0.7)

    max_density = density.max()
    if max_density > 0:
        density /= max_density

    img = Image.new('RGBA', (WIDTH, HEIGHT), (0, 0, 0, 0))
    pixels = img.load()

    for y_px in range(HEIGHT):
        for x_px in range(WIDTH):
            d = density[y_px, x_px]
            if d > 0.005:
                rel_y = 1.0 - (y_px / HEIGHT)
                base_brightness = int(150 + 70 * rel_y)
                brightness = int(base_brightness * min(1.0, 0.7 + d * 0.3))
                brightness = min(255, max(100, brightness))
                alpha = min(255, int(d * 8 * 255))
                pixels[x_px, y_px] = (brightness, brightness, brightness, alpha)

    for y_px in range(HEIGHT):
        for x_px in range(WIDTH):
            r, g, b, a = pixels[x_px, y_px]
            if a < 25:
                pixels[x_px, y_px] = (0, 0, 0, 0)
            else:
                pixels[x_px, y_px] = (r, g, b, 255)

    # Remove tiny isolated pixel clusters (< 50px)
    alpha_arr = np.array(img.split()[3])
    binary = (alpha_arr > 0).astype(np.int32)
    labeled, num_features = label(binary)
    if num_features > 1:
        component_sizes = np.bincount(labeled.ravel())
        component_sizes[0] = 0
        pixels = img.load()
        for comp_id in range(1, num_features + 1):
            if component_sizes[comp_id] < 50:
                mask = labeled == comp_id
                ys_arr, xs_arr = np.where(mask)
                for y_px, x_px in zip(ys_arr, xs_arr):
                    pixels[x_px, y_px] = (0, 0, 0, 0)

    img = img.filter(ImageFilter.GaussianBlur(radius=0.3))
    return img


for v in VARIANTS:
    print(f"Generating {v['name']}...")
    img = generate_fern(v)
    valid = validate_fern(img, v['name'])
    path = f"E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover/{v['name']}.png"
    img.save(path)
    status = "PASS" if valid else "WARN"
    print(f"  [{status}] Saved: {path}")

print("Done! (T_Fern_Cyclosorus_03 was NOT modified)")
