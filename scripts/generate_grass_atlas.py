"""Generate grass atlas texture - grayscale + alpha PNG for stylized grass."""
from PIL import Image, ImageDraw, ImageFilter
import random
import math

WIDTH = 512
HEIGHT = 512
NUM_BLADES = 12  # enough blades to cover any UV subdivision (4-6 per mesh)

random.seed(42)

img = Image.new('RGBA', (WIDTH, HEIGHT), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

blade_width = WIDTH // NUM_BLADES

for i in range(NUM_BLADES):
    cx = int((i + 0.5) * blade_width)

    # Randomize blade properties
    blade_h = random.randint(int(HEIGHT * 0.6), int(HEIGHT * 0.95))
    base_w = random.randint(blade_width // 3, blade_width // 2)
    tip_w = max(1, base_w // 6)

    # Slight lean
    lean = random.randint(-blade_width // 6, blade_width // 6)

    # Slight curve (quadratic)
    curve = random.uniform(-0.3, 0.3)

    # Draw blade as filled polygon - bottom to top
    points = []
    steps = 20
    for s in range(steps + 1):
        t = s / steps  # 0 = bottom, 1 = top
        y = HEIGHT - int(t * blade_h)

        # Width tapers from base to tip
        w = base_w * (1 - t) + tip_w * t

        # Apply lean and curve
        offset = lean * t + curve * blade_h * t * t

        x_left = cx + offset - w / 2
        x_right = cx + offset + w / 2

        points.append((x_left, y))

    # Go back down the right side
    for s in range(steps, -1, -1):
        t = s / steps
        y = HEIGHT - int(t * blade_h)
        w = base_w * (1 - t) + tip_w * t
        offset = lean * t + curve * blade_h * t * t
        x_right = cx + offset + w / 2
        points.append((x_right, y))

    # Vertical gradient: darker at bottom (100), lighter at top (240)
    # Draw blade in segments for gradient effect
    for s in range(steps):
        t_low = s / steps
        t_high = (s + 1) / steps
        y_low = HEIGHT - int(t_low * blade_h)
        y_high = HEIGHT - int(t_high * blade_h)

        # Brightness gradient
        brightness = int(140 + 100 * ((t_low + t_high) / 2))
        brightness = min(255, brightness)

        # Width at this segment
        w_low = base_w * (1 - t_low) + tip_w * t_low
        w_high = base_w * (1 - t_high) + tip_w * t_high

        offset_low = lean * t_low + curve * blade_h * t_low * t_low
        offset_high = lean * t_high + curve * blade_h * t_high * t_high

        seg_points = [
            (cx + offset_low - w_low / 2, y_low),
            (cx + offset_high - w_high / 2, y_high),
            (cx + offset_high + w_high / 2, y_high),
            (cx + offset_low + w_low / 2, y_low),
        ]

        draw.polygon(seg_points, fill=(brightness, brightness, brightness, 255))

# Slight blur for smoother alpha edges
img = img.filter(ImageFilter.GaussianBlur(radius=0.5))

# Ensure clean alpha - threshold
pixels = img.load()
for y in range(HEIGHT):
    for x in range(WIDTH):
        r, g, b, a = pixels[x, y]
        if a < 20:
            pixels[x, y] = (0, 0, 0, 0)
        else:
            pixels[x, y] = (r, g, b, 255)

output_path = "E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover/T_Grass_Atlas.png"
img.save(output_path)
print(f"Saved grass atlas: {output_path} ({WIDTH}x{HEIGHT})")
