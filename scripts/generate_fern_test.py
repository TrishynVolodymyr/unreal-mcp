"""Test fern variant - sharper pinnules, less blur, more separation."""
from PIL import Image, ImageFilter
import random
import numpy as np
from scipy.ndimage import gaussian_filter, zoom

WIDTH = 1024
HEIGHT = 1024
ITERATIONS = 5_000_000

random.seed(42)

TRANSFORMS = [
    (0.0, 0.0, 0.0, 0.16, 0.0, 0.0, 0.01),
    (0.85, 0.04, -0.04, 0.85, 0.0, 1.6, 0.85),
    (0.20, -0.26, 0.23, 0.22, 0.0, 1.6, 0.07),
    (-0.15, 0.28, 0.26, 0.24, 0.0, 0.44, 0.07),
]

thresholds = []
cumulative = 0.0
for t in TRANSFORMS:
    cumulative += t[6]
    thresholds.append(cumulative)

points = []
x, y = 0.0, 0.0

for _ in range(ITERATIONS):
    r = random.random()
    for i, threshold in enumerate(thresholds):
        if r <= threshold:
            a, b, c, d, e, f, _ = TRANSFORMS[i]
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

# Render at 2x then downscale for natural AA
SS = 2
density_hi = np.zeros((HEIGHT * SS, WIDTH * SS), dtype=np.float64)

for px, py in points:
    ix = int((px - x_min) * scale * SS + cx * SS)
    iy = (HEIGHT * SS - 1) - int((py - y_min) * scale * SS + cy * SS)
    if 0 <= ix < WIDTH * SS and 0 <= iy < HEIGHT * SS:
        density_hi[iy, ix] += 1.0

# Very light blur at high res — preserves gaps between pinnules
density_hi = gaussian_filter(density_hi, sigma=1.0)

# Downscale 2x with averaging (natural AA)
density = (density_hi[0::2, 0::2] + density_hi[1::2, 0::2] +
           density_hi[0::2, 1::2] + density_hi[1::2, 1::2]) / 4.0

max_density = density.max()
if max_density > 0:
    density /= max_density

img = Image.new('RGBA', (WIDTH, HEIGHT), (0, 0, 0, 0))
pixels = img.load()

for y_px in range(HEIGHT):
    for x_px in range(WIDTH):
        d = density[y_px, x_px]
        if d > 0.006:
            rel_y = 1.0 - (y_px / HEIGHT)
            base_brightness = int(150 + 70 * rel_y)
            brightness = int(base_brightness * min(1.0, 0.7 + d * 0.3))
            brightness = min(255, max(100, brightness))
            alpha = min(255, int(d * 12 * 255))
            pixels[x_px, y_px] = (brightness, brightness, brightness, alpha)

# Higher threshold — keeps gaps visible
for y_px in range(HEIGHT):
    for x_px in range(WIDTH):
        r, g, b, a = pixels[x_px, y_px]
        if a < 35:
            pixels[x_px, y_px] = (0, 0, 0, 0)
        else:
            pixels[x_px, y_px] = (r, g, b, 255)

output_path = "E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover/T_Fern_Atlas_test.png"
img.save(output_path)
print(f"Saved test fern: {output_path} ({WIDTH}x{HEIGHT})")
