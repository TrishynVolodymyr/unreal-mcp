"""Generate 4 distinct fern atlas textures using different IFS coefficient sets.
Each produces a genuinely different fern shape."""
from PIL import Image, ImageFilter
import random
import numpy as np
from scipy.ndimage import gaussian_filter

WIDTH = 1024
HEIGHT = 1024
ITERATIONS = 3_000_000

# 4 distinct fern shapes via different IFS coefficients
VARIANTS = [
    {
        "name": "T_Fern_Atlas_01",
        "seed": 123,
        # Tight upright fern (same as old 03 which user liked)
        "transforms": [
            (0.0, 0.0, 0.0, 0.16, 0.0, 0.0, 0.01),
            (0.86, 0.035, -0.035, 0.86, 0.0, 1.6, 0.85),
            (0.18, -0.24, 0.21, 0.20, 0.0, 1.6, 0.07),
            (-0.13, 0.26, 0.24, 0.22, 0.0, 0.44, 0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_02",
        "seed": 55,
        # Cyclosorus — wider, symmetric, shorter pinnae spread more evenly
        "transforms": [
            (0.0, 0.0, 0.0, 0.25, 0.0, -0.14, 0.02),
            (0.95, 0.005, -0.005, 0.93, -0.002, 0.5, 0.84),
            (0.035, -0.20, 0.16, 0.04, -0.09, 0.02, 0.07),
            (-0.04, 0.20, 0.16, 0.04, 0.083, 0.12, 0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_03",
        "seed": 88,
        # Fishbone — long thin pinnae, narrow overall silhouette
        "transforms": [
            (0.0, 0.0, 0.0, 0.16, 0.0, 0.0, 0.01),
            (0.85, 0.02, -0.02, 0.87, 0.0, 1.6, 0.85),
            (0.30, -0.34, 0.30, 0.34, 0.0, 1.0, 0.07),
            (-0.30, 0.34, 0.30, 0.37, 0.0, 0.7, 0.07),
        ],
    },
    {
        "name": "T_Fern_Atlas_04",
        "seed": 200,
        # Thelypteroid — broad triangular, pinnae get much longer at base
        "transforms": [
            (0.0, 0.0, 0.0, 0.16, 0.0, 0.0, 0.01),
            (0.82, 0.05, -0.05, 0.82, 0.0, 1.6, 0.85),
            (0.24, -0.32, 0.28, 0.26, 0.0, 1.6, 0.07),
            (-0.18, 0.34, 0.30, 0.28, 0.0, 0.44, 0.07),
        ],
    },
]


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

    density = gaussian_filter(density, sigma=1.2)

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

    img = img.filter(ImageFilter.GaussianBlur(radius=0.3))
    return img


for v in VARIANTS:
    print(f"Generating {v['name']}...")
    img = generate_fern(v)
    path = f"E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover/{v['name']}.png"
    img.save(path)
    print(f"  Saved: {path}")

print("Done! 4 distinct fern variants generated.")
