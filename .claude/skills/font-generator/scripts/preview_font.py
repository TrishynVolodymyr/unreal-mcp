"""
Preview TTF glyphs by rendering them to an image using PIL
"""

from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import RecordingPen
from PIL import Image, ImageDraw
import os


def render_glyph_to_image(font: TTFont, glyph_name: str, size: int = 200) -> Image.Image:
    """Render a single glyph to a PIL Image"""
    glyph_set = font.getGlyphSet()

    if glyph_name not in glyph_set:
        raise ValueError(f"Glyph '{glyph_name}' not found in font")

    # Record the glyph's drawing commands
    pen = RecordingPen()
    glyph_set[glyph_name].draw(pen)

    # Get glyph bounds
    bounds = None
    for op, args in pen.value:
        if op in ('moveTo', 'lineTo', 'qCurveTo', 'curveTo'):
            for point in args:
                if isinstance(point, tuple):
                    x, y = point
                    if bounds is None:
                        bounds = [x, y, x, y]
                    else:
                        bounds[0] = min(bounds[0], x)
                        bounds[1] = min(bounds[1], y)
                        bounds[2] = max(bounds[2], x)
                        bounds[3] = max(bounds[3], y)

    if bounds is None:
        # Empty glyph (like space)
        img = Image.new('RGB', (size // 2, size), 'white')
        return img

    # Calculate scale and offset
    glyph_width = bounds[2] - bounds[0]
    glyph_height = bounds[3] - bounds[1]

    if glyph_width == 0 or glyph_height == 0:
        img = Image.new('RGB', (size // 2, size), 'white')
        return img

    scale = (size - 20) / max(glyph_width, glyph_height)

    img_width = int(glyph_width * scale) + 20
    img_height = int(glyph_height * scale) + 20

    img = Image.new('RGB', (img_width, img_height), 'white')
    draw = ImageDraw.Draw(img)

    # Transform and draw paths
    def transform_point(x, y):
        # Flip Y axis (font coords are Y-up, PIL is Y-down)
        tx = (x - bounds[0]) * scale + 10
        ty = img_height - ((y - bounds[1]) * scale + 10)
        return (tx, ty)

    # Collect paths for polygon drawing
    current_path = []
    paths = []

    for op, args in pen.value:
        if op == 'moveTo':
            if current_path:
                paths.append(current_path)
            current_path = [transform_point(*args[0])]
        elif op == 'lineTo':
            current_path.append(transform_point(*args[0]))
        elif op == 'qCurveTo':
            # Approximate quadratic Bezier with line segments
            if current_path:
                p0 = current_path[-1]
                for i, point in enumerate(args):
                    p2 = transform_point(*point)
                    if i < len(args) - 1:
                        # Implied on-curve point
                        next_point = transform_point(*args[i + 1])
                        p2 = ((p2[0] + next_point[0]) / 2, (p2[1] + next_point[1]) / 2)
                    # Simple linear approximation for now
                    current_path.append(p2)
        elif op == 'curveTo':
            # Approximate cubic Bezier with line segments
            for point in args:
                current_path.append(transform_point(*point))
        elif op == 'closePath':
            if current_path:
                paths.append(current_path)
                current_path = []
        elif op == 'endPath':
            if current_path:
                paths.append(current_path)
                current_path = []

    if current_path:
        paths.append(current_path)

    # Draw filled polygons
    for path in paths:
        if len(path) >= 3:
            draw.polygon(path, fill='black', outline='black')

    return img


def create_font_preview(font_path: str, output_path: str):
    """Create a preview image showing all glyphs in the font"""
    font = TTFont(font_path)
    glyph_order = font.getGlyphOrder()

    print(f"Font: {font_path}")
    print(f"Glyphs: {glyph_order}")

    # Render each glyph
    glyph_images = []
    for glyph_name in glyph_order:
        try:
            img = render_glyph_to_image(font, glyph_name, size=150)
            glyph_images.append((glyph_name, img))
            print(f"  Rendered: {glyph_name} ({img.size[0]}x{img.size[1]})")
        except Exception as e:
            print(f"  Failed to render {glyph_name}: {e}")

    if not glyph_images:
        print("No glyphs rendered!")
        return

    # Combine into a single preview image
    padding = 10
    label_height = 25

    total_width = sum(img.size[0] for _, img in glyph_images) + padding * (len(glyph_images) + 1)
    max_height = max(img.size[1] for _, img in glyph_images)
    total_height = max_height + label_height + padding * 2

    preview = Image.new('RGB', (total_width, total_height), 'lightgray')
    draw = ImageDraw.Draw(preview)

    x_offset = padding
    for glyph_name, img in glyph_images:
        # Center vertically
        y_offset = padding + (max_height - img.size[1]) // 2
        preview.paste(img, (x_offset, y_offset))

        # Draw label
        label_y = padding + max_height + 5
        draw.text((x_offset + img.size[0] // 2, label_y), glyph_name, fill='black', anchor='mt')

        x_offset += img.size[0] + padding

    preview.save(output_path)
    print(f"\nPreview saved to: {output_path}")


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    font_path = os.path.join(script_dir, "TestFont.ttf")
    output_path = os.path.join(script_dir, "TestFont_preview.png")

    create_font_preview(font_path, output_path)
