"""
Add TORN effect to font - dramatic breaks, notches, missing chunks
Unlike ragged (uniform waviness), torn has occasional deep cuts
"""

from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import RecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
import random
import math
import os


def add_tear_to_line(p1, p2, tear_probability=0.3, tear_depth=25, base_roughness=5):
    """
    Add tears/notches to a line segment

    Returns list of points including tears
    """
    points = []
    dx = p2[0] - p1[0]
    dy = p2[1] - p1[1]
    length = math.sqrt(dx*dx + dy*dy)

    if length < 20:  # Too short to tear
        return [p2]

    # Perpendicular direction for cuts
    if length > 0:
        px, py = -dy/length, dx/length
    else:
        px, py = 0, 1

    # Number of potential tear points based on length
    num_segments = max(2, int(length / 40))

    for i in range(1, num_segments):
        t = i / num_segments
        x = p1[0] + t * dx
        y = p1[1] + t * dy

        # Add base roughness
        x += random.uniform(-base_roughness, base_roughness)
        y += random.uniform(-base_roughness, base_roughness)

        # Chance of a tear (deep notch)
        if random.random() < tear_probability:
            # Varied tear sizes: 30% big, 40% medium, 30% small
            size_roll = random.random()
            if size_roll < 0.3:
                # BIG tear - wide and deep
                tear_width = random.uniform(25, 45)
                depth_mult = random.uniform(1.0, 1.5)
            elif size_roll < 0.7:
                # MEDIUM tear
                tear_width = random.uniform(15, 30)
                depth_mult = random.uniform(0.6, 1.0)
            else:
                # SMALL tear - narrow notch
                tear_width = random.uniform(8, 18)
                depth_mult = random.uniform(0.3, 0.6)

            # Point before notch
            bx = x - (dx/length) * tear_width/2
            by = y - (dy/length) * tear_width/2
            points.append((bx + px * base_roughness * random.uniform(-1, 1),
                          by + py * base_roughness * random.uniform(-1, 1)))

            # INWARD CUTS ONLY - no outward spikes
            direction = -1  # cuts into the letter
            tear_size = tear_depth * depth_mult

            nx = x + px * tear_size * direction
            ny = y + py * tear_size * direction
            points.append((nx, ny))

            # Point after notch
            ax = x + (dx/length) * tear_width/2
            ay = y + (dy/length) * tear_width/2
            points.append((ax + px * base_roughness * random.uniform(-1, 1),
                          ay + py * base_roughness * random.uniform(-1, 1)))
        else:
            points.append((x, y))

    # Final point
    fx = p2[0] + random.uniform(-base_roughness, base_roughness)
    fy = p2[1] + random.uniform(-base_roughness, base_roughness)
    points.append((fx, fy))

    return points


def tear_glyph(recording, tear_probability=0.25, tear_depth=30, base_roughness=6):
    """
    Apply torn effect to recorded glyph
    """
    result = []
    last_point = None

    for op, args in recording:
        if op == 'moveTo':
            pt = args[0]
            # Slight roughness on start points
            pt = (pt[0] + random.uniform(-base_roughness, base_roughness),
                  pt[1] + random.uniform(-base_roughness, base_roughness))
            result.append(('moveTo', (pt,)))
            last_point = pt

        elif op == 'lineTo':
            target = args[0]

            if last_point:
                # Add tears along this line
                torn_points = add_tear_to_line(
                    last_point, target,
                    tear_probability=tear_probability,
                    tear_depth=tear_depth,
                    base_roughness=base_roughness
                )
                for pt in torn_points:
                    result.append(('lineTo', (pt,)))
                last_point = torn_points[-1]
            else:
                result.append(('lineTo', (target,)))
                last_point = target

        elif op == 'qCurveTo':
            # Add roughness to curve points, occasional tear at end
            new_args = []
            for i, point in enumerate(args):
                rough = base_roughness if i < len(args) - 1 else base_roughness * 1.5
                pt = (point[0] + random.uniform(-rough, rough),
                      point[1] + random.uniform(-rough, rough))
                new_args.append(pt)
            result.append(('qCurveTo', tuple(new_args)))
            last_point = new_args[-1]

        elif op == 'curveTo':
            new_args = []
            for i, point in enumerate(args):
                rough = base_roughness if i < len(args) - 1 else base_roughness * 1.5
                pt = (point[0] + random.uniform(-rough, rough),
                      point[1] + random.uniform(-rough, rough))
                new_args.append(pt)
            result.append(('curveTo', tuple(new_args)))
            last_point = new_args[-1]

        elif op == 'closePath':
            result.append(('closePath', ()))
            last_point = None

        elif op == 'endPath':
            result.append(('endPath', ()))
            last_point = None

    return result


def replay_to_pen(operations, pen):
    """Replay recorded operations to a pen"""
    for op, args in operations:
        if op == 'moveTo':
            pen.moveTo(*args)
        elif op == 'lineTo':
            pen.lineTo(*args)
        elif op == 'qCurveTo':
            pen.qCurveTo(*args)
        elif op == 'curveTo':
            pen.curveTo(*args)
        elif op == 'closePath':
            pen.closePath()
        elif op == 'endPath':
            pen.endPath()


def create_torn_font(input_path: str, output_path: str,
                     tear_probability: float = 0.25,
                     tear_depth: float = 30,
                     base_roughness: float = 6,
                     seed: int = 42):
    """
    Create a torn/damaged version of a font

    Args:
        input_path: Path to source TTF
        output_path: Path for output TTF
        tear_probability: Chance of tear per segment (0.0-1.0)
        tear_depth: How deep tears cut into letters
        base_roughness: Background roughness level
        seed: Random seed
    """
    random.seed(seed)

    font = TTFont(input_path)
    glyph_set = font.getGlyphSet()
    glyph_order = font.getGlyphOrder()

    print(f"Processing: {input_path}")
    print(f"Tear probability: {tear_probability}, Depth: {tear_depth}, Base roughness: {base_roughness}")

    glyf_table = font['glyf']

    processed = 0
    for glyph_name in glyph_order:
        if glyph_name not in glyph_set:
            continue

        try:
            rec_pen = RecordingPen()
            glyph_set[glyph_name].draw(rec_pen)

            if not rec_pen.value:
                continue

            # Apply torn effect
            torn_ops = tear_glyph(
                rec_pen.value,
                tear_probability=tear_probability,
                tear_depth=tear_depth,
                base_roughness=base_roughness
            )

            # Rebuild glyph
            tt_pen = TTGlyphPen(None)
            replay_to_pen(torn_ops, tt_pen)

            glyf_table[glyph_name] = tt_pen.glyph()
            processed += 1

        except Exception as e:
            print(f"  Warning: {glyph_name}: {e}")

    print(f"Processed {processed} glyphs")

    # Update font name
    name_table = font['name']
    for record in name_table.names:
        if record.nameID in (1, 4, 6):
            try:
                current = record.toUnicode()
                record.string = (current + " Torn").encode(record.getEncoding())
            except:
                pass

    font.save(output_path)
    print(f"Saved: {output_path}")
    return output_path


def preview_font(font_path: str, output_path: str, text: str = "ABWXZ"):
    """Create preview image"""
    from PIL import Image, ImageDraw, ImageFont

    img = Image.new('RGB', (700, 200), (20, 15, 25))
    draw = ImageDraw.Draw(img)

    try:
        font_large = ImageFont.truetype(font_path, 80)
        font_small = ImageFont.truetype(font_path, 40)

        color = (218, 165, 32)
        draw.text((30, 20), text, font=font_large, fill=color)
        draw.text((30, 120), text, font=font_small, fill=(180, 140, 80))

        img.save(output_path)
        print(f"Preview: {output_path}")
    except Exception as e:
        print(f"Preview failed: {e}")


def preview_full_charset(font_path: str, output_path: str):
    """Create comprehensive preview with full character set"""
    from PIL import Image, ImageDraw, ImageFont

    img = Image.new('RGB', (1200, 600), (20, 15, 25))
    draw = ImageDraw.Draw(img)

    try:
        font_title = ImageFont.truetype(font_path, 60)
        font_main = ImageFont.truetype(font_path, 44)
        font_small = ImageFont.truetype(font_path, 32)

        color_gold = (218, 165, 32)
        color_bronze = (180, 140, 80)
        color_dim = (140, 110, 60)

        y = 20
        # Title
        draw.text((30, y), "CINZEL TORN", font=font_title, fill=color_gold)
        y += 80

        # Uppercase
        draw.text((30, y), "ABCDEFGHIJKLM", font=font_main, fill=color_gold)
        y += 55
        draw.text((30, y), "NOPQRSTUVWXYZ", font=font_main, fill=color_gold)
        y += 55

        # Lowercase
        draw.text((30, y), "abcdefghijklm", font=font_main, fill=color_bronze)
        y += 55
        draw.text((30, y), "nopqrstuvwxyz", font=font_main, fill=color_bronze)
        y += 55

        # Numbers
        draw.text((30, y), "0123456789", font=font_main, fill=color_bronze)
        y += 55

        # Symbols
        draw.text((30, y), "!@#$%&*()-+=[]{}:;\"'<>,.?/", font=font_small, fill=color_dim)
        y += 45

        # Sample text
        draw.text((30, y), "The Dark Lord Awaits", font=font_main, fill=color_gold)

        img.save(output_path)
        print(f"Full preview: {output_path}")
    except Exception as e:
        print(f"Full preview failed: {e}")


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    skill_dir = os.path.join(script_dir, "..", "..", ".claude", "skills", "font-generator", "cinzel_font")

    input_font = os.path.join(skill_dir, "Cinzel-Bold.ttf")
    output_font = os.path.join(script_dir, "Cinzel-Torn.ttf")
    preview_path = os.path.join(script_dir, "Cinzel-Torn_preview.png")

    # Create torn version - varied sizes (big, medium, small)
    create_torn_font(
        input_font,
        output_font,
        tear_probability=0.18,  # 18% - slightly more to show variety
        tear_depth=120,         # Base depth (multiplied by size category)
        base_roughness=4,       # Subtle background
        seed=42
    )

    preview_font(output_font, preview_path)

    # Full character set preview
    full_preview_path = os.path.join(script_dir, "Cinzel-Torn_full_preview.png")
    preview_full_charset(output_font, full_preview_path)
