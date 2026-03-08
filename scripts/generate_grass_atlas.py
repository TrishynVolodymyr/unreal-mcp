"""Generate dense grass CLUMP textures for cross-plane meshes.
Each texture = a dense cluster of overlapping grass blades filling most of the card.
Stylized/Ghibli approach: wide graphic blades, dense bottom, airy tips at top."""
from PIL import Image, ImageDraw, ImageFilter
import random
import math

W, H = 512, 512

def generate_clump(seed, num_blades=30, style="short"):
    """Generate a single dense grass clump texture."""
    random.seed(seed)
    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Style presets
    if style == "short":
        h_range = (0.45, 0.75)
        base_w_range = (12, 22)
        lean_range = (-25, 25)
        curve_range = (-0.25, 0.25)
    else:  # tall
        h_range = (0.65, 0.95)
        base_w_range = (10, 18)
        lean_range = (-30, 30)
        curve_range = (-0.35, 0.35)

    # Spread blades across the width with some overlap
    spread = W * 0.85  # use 85% of width
    x_start = (W - spread) / 2

    for i in range(num_blades):
        # Random X position across the spread — clustered toward center
        cx = x_start + random.gauss(spread / 2, spread / 4)
        cx = max(10, min(W - 10, cx))

        blade_h = int(H * random.uniform(*h_range))
        base_w = random.randint(*base_w_range)
        tip_w = max(1, random.randint(1, 3))

        lean = random.randint(*lean_range)
        curve = random.uniform(*curve_range)

        # Draw blade as filled polygon with gradient
        steps = 16
        for s in range(steps):
            t_low = s / steps
            t_high = (s + 1) / steps
            y_low = H - int(t_low * blade_h)
            y_high = H - int(t_high * blade_h)

            # Width tapers
            w_low = base_w * (1 - t_low ** 0.7) + tip_w * t_low ** 0.7
            w_high = base_w * (1 - t_high ** 0.7) + tip_w * t_high ** 0.7

            # Lean + curve offset
            off_low = lean * t_low + curve * blade_h * t_low * t_low
            off_high = lean * t_high + curve * blade_h * t_high * t_high

            # Brightness: darker at base (130), lighter at tip (245)
            t_mid = (t_low + t_high) / 2
            bri = int(130 + 115 * t_mid)
            bri = min(250, max(120, bri))

            seg = [
                (cx + off_low - w_low / 2, y_low),
                (cx + off_high - w_high / 2, y_high),
                (cx + off_high + w_high / 2, y_high),
                (cx + off_low + w_low / 2, y_low),
            ]
            draw.polygon(seg, fill=(bri, bri, bri, 255))

    # Soft blur for anti-aliasing
    img = img.filter(ImageFilter.GaussianBlur(radius=0.6))

    # Clean alpha
    px = img.load()
    for y in range(H):
        for x in range(W):
            r, g, b, a = px[x, y]
            if a < 15:
                px[x, y] = (0, 0, 0, 0)
            else:
                px[x, y] = (r, g, b, 255)

    return img


OUT = "E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover"

# Generate 3 short clump variants + 2 tall clump variants
variants = [
    ("T_Grass_Short_01", 100, 28, "short"),
    ("T_Grass_Short_02", 101, 32, "short"),
    ("T_Grass_Short_03", 102, 25, "short"),
    ("T_Grass_Tall_01", 200, 22, "tall"),
    ("T_Grass_Tall_02", 201, 26, "tall"),
]

for name, seed, num_blades, style in variants:
    print(f"Generating {name} (seed={seed}, {num_blades} blades, {style})...")
    img = generate_clump(seed, num_blades, style)

    # Quick fill check
    import numpy as np
    alpha = np.array(img.split()[3])
    fill = np.mean(alpha > 0)
    print(f"  Fill: {fill:.1%}")

    path = f"{OUT}/{name}.png"
    img.save(path)
    print(f"  Saved: {path}")

# Also save a combined atlas for backward compat (just use Short_01)
import shutil
shutil.copy(f"{OUT}/T_Grass_Short_01.png", f"{OUT}/T_Grass_Atlas.png")
print(f"\nT_Grass_Atlas.png updated (copy of Short_01)")
print("Done! 5 dense grass clump textures.")
