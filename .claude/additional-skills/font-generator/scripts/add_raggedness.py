"""
Add raggedness/weathering to an existing font
Takes clean font paths and adds controlled distortion
"""

from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import RecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.pens.t2CharStringPen import T2CharStringPen
import random
import math
import os
import copy


def add_noise_to_point(x, y, amount=8, seed=None):
    """Add random noise to a point"""
    if seed is not None:
        random.seed(seed)

    # Add random offset
    nx = x + random.uniform(-amount, amount)
    ny = y + random.uniform(-amount, amount)
    return (nx, ny)


def interpolate_and_roughen(p1, p2, num_segments=3, roughness=10):
    """
    Break a line into segments and add roughness to intermediate points
    """
    points = []
    for i in range(1, num_segments):
        t = i / num_segments
        x = p1[0] + t * (p2[0] - p1[0])
        y = p1[1] + t * (p2[1] - p1[1])
        # Add perpendicular noise
        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        length = math.sqrt(dx*dx + dy*dy)
        if length > 0:
            # Perpendicular direction
            px, py = -dy/length, dx/length
            noise = random.uniform(-roughness, roughness)
            x += px * noise
            y += py * noise
        points.append((x, y))
    return points


def roughen_glyph(recording, roughness=12, edge_roughness=6, subdivisions=2):
    """
    Take recorded glyph operations and add roughness

    Args:
        recording: List of (operation, args) tuples from RecordingPen
        roughness: Amount of roughness for intermediate points
        edge_roughness: Amount of roughness for original points
        subdivisions: How many times to subdivide lines
    """
    result = []
    last_point = None

    for op, args in recording:
        if op == 'moveTo':
            pt = add_noise_to_point(args[0][0], args[0][1], edge_roughness)
            result.append(('moveTo', (pt,)))
            last_point = pt

        elif op == 'lineTo':
            target = args[0]

            if last_point and subdivisions > 0:
                # Add intermediate rough points
                intermediate = interpolate_and_roughen(
                    last_point, target,
                    num_segments=subdivisions + 1,
                    roughness=roughness
                )
                for pt in intermediate:
                    result.append(('lineTo', (pt,)))

            # Final point with edge roughness
            pt = add_noise_to_point(target[0], target[1], edge_roughness)
            result.append(('lineTo', (pt,)))
            last_point = pt

        elif op == 'qCurveTo':
            # For curves, add noise to control points and end point
            new_args = []
            for point in args:
                pt = add_noise_to_point(point[0], point[1], edge_roughness)
                new_args.append(pt)
            result.append(('qCurveTo', tuple(new_args)))
            last_point = new_args[-1]

        elif op == 'curveTo':
            # Cubic curves
            new_args = []
            for point in args:
                pt = add_noise_to_point(point[0], point[1], edge_roughness)
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


def create_ragged_font(input_path: str, output_path: str,
                       roughness: float = 15,
                       edge_roughness: float = 8,
                       subdivisions: int = 2,
                       seed: int = 42):
    """
    Create a ragged version of a font

    Args:
        input_path: Path to source TTF
        output_path: Path for output TTF
        roughness: Amount of waviness on edges (higher = more ragged)
        edge_roughness: Amount of displacement at corners
        subdivisions: Line subdivisions (more = finer grain roughness)
        seed: Random seed for reproducibility
    """
    random.seed(seed)

    # Load source font
    font = TTFont(input_path)
    glyph_set = font.getGlyphSet()
    glyph_order = font.getGlyphOrder()

    print(f"Processing font: {input_path}")
    print(f"Glyphs: {len(glyph_order)}")
    print(f"Roughness: {roughness}, Edge: {edge_roughness}, Subdivisions: {subdivisions}")

    # Get glyf table for modification
    glyf_table = font['glyf']

    processed = 0
    for glyph_name in glyph_order:
        if glyph_name not in glyph_set:
            continue

        try:
            # Record original glyph
            rec_pen = RecordingPen()
            glyph_set[glyph_name].draw(rec_pen)

            if not rec_pen.value:
                continue

            # Add roughness
            rough_ops = roughen_glyph(
                rec_pen.value,
                roughness=roughness,
                edge_roughness=edge_roughness,
                subdivisions=subdivisions
            )

            # Rebuild glyph with TTGlyphPen
            tt_pen = TTGlyphPen(None)
            replay_to_pen(rough_ops, tt_pen)

            # Replace glyph
            new_glyph = tt_pen.glyph()
            glyf_table[glyph_name] = new_glyph
            processed += 1

        except Exception as e:
            print(f"  Warning: Could not process {glyph_name}: {e}")

    print(f"Processed {processed} glyphs")

    # Update font name
    name_table = font['name']
    for record in name_table.names:
        if record.nameID in (1, 4, 6):  # Family, Full, PostScript names
            try:
                current = record.toUnicode()
                record.string = (current + " Ragged").encode(record.getEncoding())
            except:
                pass

    # Save
    font.save(output_path)
    print(f"Saved to: {output_path}")

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


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    skill_dir = os.path.join(script_dir, "..", "..", ".claude", "skills", "font-generator", "cinzel_font")

    input_font = os.path.join(skill_dir, "Cinzel-Bold.ttf")
    output_font = os.path.join(script_dir, "Cinzel-Ragged.ttf")
    preview_path = os.path.join(script_dir, "Cinzel-Ragged_preview.png")

    # Create ragged version
    create_ragged_font(
        input_font,
        output_font,
        roughness=15,      # Waviness on edges
        edge_roughness=8,  # Corner displacement
        subdivisions=3,    # Line detail
        seed=42
    )

    # Preview
    preview_font(output_font, preview_path)
