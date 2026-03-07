"""Generate fern atlas texture using Barnsley Fern IFS fractal.
Produces a realistic filled fern frond as grayscale + alpha PNG."""
from PIL import Image, ImageFilter
import random
import numpy as np
from scipy.ndimage import gaussian_filter

WIDTH = 1024
HEIGHT = 1024
ITERATIONS = 3_000_000

random.seed(42)

# Barnsley Fern IFS coefficients
TRANSFORMS = [
    (0.0, 0.0, 0.0, 0.16, 0.0, 0.0, 0.01),       # Stem
    (0.85, 0.04, -0.04, 0.85, 0.0, 1.6, 0.85),     # Main body
    (0.20, -0.26, 0.23, 0.22, 0.0, 1.6, 0.07),     # Left pinnae
    (-0.15, 0.28, 0.26, 0.24, 0.0, 0.44, 0.07),    # Right pinnae
]

thresholds = []
cumulative = 0.0
for t in TRANSFORMS:
    cumulative += t[6]
    thresholds.append(cumulative)

# Generate points
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

# Find bounds
xs = [p[0] for p in points]
ys = [p[1] for p in points]
x_min, x_max = min(xs), max(xs)
y_min, y_max = min(ys), max(ys)

# Map to image with padding
pad = 30
img_w = WIDTH - 2 * pad
img_h = HEIGHT - 2 * pad

scale_x = img_w / (x_max - x_min) if x_max != x_min else 1
scale_y = img_h / (y_max - y_min) if y_max != y_min else 1
scale = min(scale_x, scale_y)

cx = pad + (img_w - (x_max - x_min) * scale) / 2
cy = pad + (img_h - (y_max - y_min) * scale) / 2

# Accumulate density - single pixel per point (more points = filled naturally)
density = np.zeros((HEIGHT, WIDTH), dtype=np.float64)

for px, py in points:
    ix = int((px - x_min) * scale + cx)
    iy = HEIGHT - 1 - int((py - y_min) * scale + cy)

    if 0 <= ix < WIDTH and 0 <= iy < HEIGHT:
        density[iy, ix] += 1.0

# Light blur - just enough to fill micro-gaps, not enough to merge pinnae
density = gaussian_filter(density, sigma=1.2)

# Normalize
max_density = density.max()
if max_density > 0:
    density /= max_density

# Create RGBA image
img = Image.new('RGBA', (WIDTH, HEIGHT), (0, 0, 0, 0))
pixels = img.load()

for y_px in range(HEIGHT):
    for x_px in range(WIDTH):
        d = density[y_px, x_px]
        if d > 0.005:
            # Vertical gradient: darker at base, lighter at tips
            rel_y = 1.0 - (y_px / HEIGHT)
            base_brightness = int(150 + 70 * rel_y)

            brightness = int(base_brightness * min(1.0, 0.7 + d * 0.3))
            brightness = min(255, max(100, brightness))

            # Alpha: sharp fill - either opaque or not
            alpha = min(255, int(d * 8 * 255))
            pixels[x_px, y_px] = (brightness, brightness, brightness, alpha)

# Hard alpha threshold
for y_px in range(HEIGHT):
    for x_px in range(WIDTH):
        r, g, b, a = pixels[x_px, y_px]
        if a < 25:
            pixels[x_px, y_px] = (0, 0, 0, 0)
        else:
            pixels[x_px, y_px] = (r, g, b, 255)

# Very light final blur for edge smoothing only
img = img.filter(ImageFilter.GaussianBlur(radius=0.3))

output_path = "E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover/T_Fern_Atlas.png"
img.save(output_path)
print(f"Saved fern atlas: {output_path} ({WIDTH}x{HEIGHT})")
