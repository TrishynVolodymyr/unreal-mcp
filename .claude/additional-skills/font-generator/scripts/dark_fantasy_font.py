"""
Dark Fantasy Font - Sharp, angular, gothic-inspired glyphs
Letters: A, B, W, X, Z
"""

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
import os


def draw_dark_A(pen, width=700, height=750):
    """
    Dark Fantasy A - Sharp peaked top, angular crossbar, spiked feet
    """
    # Main body - sharp angular A
    pen.moveTo((width // 2, height))           # Peak
    pen.lineTo((width // 2 + 40, height - 80)) # Right of peak
    pen.lineTo((width - 30, 0))                # Bottom right outer
    pen.lineTo((width - 100, 0))               # Bottom right inner start
    pen.lineTo((width - 130, 80))              # Right foot spike up
    pen.lineTo((width - 160, 0))               # Right foot spike down
    pen.lineTo((width // 2 + 60, 280))         # Right side of crossbar gap
    pen.lineTo((width // 2 - 60, 280))         # Left side of crossbar gap
    pen.lineTo((160, 0))                       # Left foot spike down
    pen.lineTo((130, 80))                      # Left foot spike up
    pen.lineTo((100, 0))                       # Left inner
    pen.lineTo((30, 0))                        # Bottom left outer
    pen.lineTo((width // 2 - 40, height - 80)) # Left of peak
    pen.closePath()

    # Angular crossbar hole (counter-clockwise for hole)
    pen.moveTo((width // 2, 500))              # Top point
    pen.lineTo((width // 2 - 80, 340))         # Bottom left
    pen.lineTo((width // 2 + 80, 340))         # Bottom right
    pen.closePath()


def draw_dark_B(pen, width=580, height=750):
    """
    Dark Fantasy B - Strong stem with two angular bowls
    Proper proportions, sharp but controlled
    """
    s = 80  # stem width

    # Outer shape - continuous outline
    pen.moveTo((0, 0))                         # Bottom-left
    pen.lineTo((0, height))                    # Up to top-left
    pen.lineTo((350, height))                  # Top bar right
    pen.lineTo((450, height - 50))             # Top-right angle
    pen.lineTo((480, height - 150))            # Upper bowl right point
    pen.lineTo((450, height//2 + 50))          # Upper bowl bottom-right
    pen.lineTo((350, height//2))               # Middle notch
    pen.lineTo((470, height//2 - 50))          # Lower bowl top-right
    pen.lineTo((520, height//4 + 30))          # Lower bowl right point (wider)
    pen.lineTo((470, 50))                      # Lower bowl bottom-right
    pen.lineTo((350, 0))                       # Bottom bar right
    pen.closePath()

    # Top bowl hole (counter-clockwise)
    pen.moveTo((s, height - s))                # Top-left of hole
    pen.lineTo((320, height - s))              # Top-right of hole
    pen.lineTo((380, height - 100))            # Right corner
    pen.lineTo((400, height - 180))            # Right bulge
    pen.lineTo((380, height//2 + s + 20))      # Bottom-right
    pen.lineTo((s, height//2 + s + 20))        # Bottom-left
    pen.closePath()

    # Bottom bowl hole (counter-clockwise)
    pen.moveTo((s, height//2 - s - 20))        # Top-left of hole
    pen.lineTo((380, height//2 - s - 20))      # Top-right
    pen.lineTo((420, height//4 + 30))          # Right bulge
    pen.lineTo((380, s))                       # Bottom-right
    pen.lineTo((s, s))                         # Bottom-left
    pen.closePath()


def draw_dark_W(pen, width=800, height=750):
    """
    Dark Fantasy W - Connected double-V shape with uniform stroke
    Single continuous outline
    """
    s = 70  # stroke width

    # W as single connected outline (clockwise outer, then back inner)
    pen.moveTo((0, height))                    # Top-left start

    # Outer left edge down
    pen.lineTo((160, 0))                       # Bottom-left point

    # Up to left-center peak
    pen.lineTo((280, 450))                     # Left inner peak

    # Down to center valley
    pen.lineTo((width//2, 80))                 # Center bottom point (sharp V)

    # Up to right-center peak
    pen.lineTo((width - 280, 450))             # Right inner peak

    # Down to bottom-right
    pen.lineTo((width - 160, 0))               # Bottom-right point

    # Outer right edge up
    pen.lineTo((width, height))                # Top-right

    # Inner right edge down
    pen.lineTo((width - s, height))            # Top-right inner
    pen.lineTo((width - 160 - s//2, s + 40))   # Bottom-right inner
    pen.lineTo((width - 280, 450 - s))         # Right peak inner
    pen.lineTo((width//2, 80 + s + 40))        # Center valley inner
    pen.lineTo((280, 450 - s))                 # Left peak inner
    pen.lineTo((160 + s//2, s + 40))           # Bottom-left inner
    pen.lineTo((s, height))                    # Top-left inner

    pen.closePath()


def draw_dark_X(pen, width=620, height=750):
    """
    Dark Fantasy X - Four arms meeting at angular center
    Each arm is a proper trapezoid, meeting at a diamond center
    """
    s = 70   # stroke width at ends
    cs = 50  # center size (half-width of center diamond)
    cx, cy = width // 2, height // 2

    # Draw as single shape - trace around entire X outline
    pen.moveTo((0, height))                    # Top-left outer corner
    pen.lineTo((s + 20, height))               # Top-left inner corner
    pen.lineTo((cx - cs, cy + cs))             # To center (top-left of diamond)
    pen.lineTo((cx, cy + cs + 20))             # Center top point
    pen.lineTo((cx + cs, cy + cs))             # Center (top-right of diamond)
    pen.lineTo((width - s - 20, height))       # Top-right inner corner
    pen.lineTo((width, height))                # Top-right outer corner
    pen.lineTo((width, height - s - 20))       # Top-right down
    pen.lineTo((cx + cs, cy + cs))             # Back to center top-right
    pen.lineTo((cx + cs + 20, cy))             # Center right point
    pen.lineTo((cx + cs, cy - cs))             # Center bottom-right
    pen.lineTo((width, s + 20))                # Bottom-right up
    pen.lineTo((width, 0))                     # Bottom-right outer
    pen.lineTo((width - s - 20, 0))            # Bottom-right inner
    pen.lineTo((cx + cs, cy - cs))             # To center bottom-right
    pen.lineTo((cx, cy - cs - 20))             # Center bottom point
    pen.lineTo((cx - cs, cy - cs))             # Center bottom-left
    pen.lineTo((s + 20, 0))                    # Bottom-left inner
    pen.lineTo((0, 0))                         # Bottom-left outer
    pen.lineTo((0, s + 20))                    # Bottom-left up
    pen.lineTo((cx - cs, cy - cs))             # To center bottom-left
    pen.lineTo((cx - cs - 20, cy))             # Center left point
    pen.lineTo((cx - cs, cy + cs))             # Center top-left
    pen.lineTo((0, height - s - 20))           # Top-left down
    pen.closePath()


def draw_dark_Z(pen, width=620, height=750):
    """
    Dark Fantasy Z - Angular with sharp diagonal, spiked terminals
    """
    stroke = 80
    spike = 50

    # Z shape
    pen.moveTo((30, height))                   # Top-left
    pen.lineTo((width - 50, height))           # Top-right
    pen.lineTo((width - 30, height - spike))   # Top-right spike down
    pen.lineTo((width - 80, height - stroke))  # Top bar bottom-right
    pen.lineTo((150, stroke + 30))             # Diagonal to bottom-left
    pen.lineTo((width - 80, stroke))           # Bottom bar top-right
    pen.lineTo((width - 30, spike))            # Bottom-right spike up
    pen.lineTo((width - 50, 0))                # Bottom-right
    pen.lineTo((30, 0))                        # Bottom-left
    pen.lineTo((50, spike))                    # Bottom-left spike up
    pen.lineTo((100, stroke))                  # Bottom bar top-left
    pen.lineTo((width - 150, height - stroke - 30))  # Diagonal to top
    pen.lineTo((100, height - stroke))         # Top bar bottom-left
    pen.lineTo((50, height - spike))           # Top-left spike down
    pen.closePath()


def draw_notdef(pen, width=500, height=750):
    """Draw .notdef glyph"""
    pen.moveTo((50, 0))
    pen.lineTo((50, height))
    pen.lineTo((width - 50, height))
    pen.lineTo((width - 50, 0))
    pen.closePath()

    pen.moveTo((100, 50))
    pen.lineTo((width - 100, 50))
    pen.lineTo((width - 100, height - 50))
    pen.lineTo((100, height - 50))
    pen.closePath()


def create_dark_fantasy_font(output_path: str, font_name: str = "DarkFantasy"):
    """Create the dark fantasy font"""
    units_per_em = 1000
    ascender = 800
    descender = -200

    glyph_order = [".notdef", "space", "A", "B", "W", "X", "Z"]

    cmap = {
        0x0020: "space",
        0x0041: "A",
        0x0042: "B",
        0x0057: "W",
        0x0058: "X",
        0x005A: "Z",
    }

    advance_widths = {
        ".notdef": 500,
        "space": 300,
        "A": 750,
        "B": 620,
        "W": 850,
        "X": 670,
        "Z": 670,
    }

    fb = FontBuilder(units_per_em, isTTF=True)
    fb.setupGlyphOrder(glyph_order)
    fb.setupCharacterMap(cmap)

    glyphs = {}
    pen_class = TTGlyphPen

    # .notdef
    pen = pen_class(None)
    draw_notdef(pen, width=500, height=750)
    glyphs[".notdef"] = pen.glyph()

    # space
    pen = pen_class(None)
    glyphs["space"] = pen.glyph()

    # A
    pen = pen_class(None)
    draw_dark_A(pen, width=700, height=750)
    glyphs["A"] = pen.glyph()

    # B
    pen = pen_class(None)
    draw_dark_B(pen, width=580, height=750)
    glyphs["B"] = pen.glyph()

    # W
    pen = pen_class(None)
    draw_dark_W(pen, width=800, height=750)
    glyphs["W"] = pen.glyph()

    # X
    pen = pen_class(None)
    draw_dark_X(pen, width=620, height=750)
    glyphs["X"] = pen.glyph()

    # Z
    pen = pen_class(None)
    draw_dark_Z(pen, width=620, height=750)
    glyphs["Z"] = pen.glyph()

    fb.setupGlyf(glyphs)

    metrics = {name: (advance_widths[name], 0) for name in glyph_order}
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=ascender, descent=descender)
    fb.setupHead(unitsPerEm=units_per_em)
    fb.setupOS2(
        sTypoAscender=ascender,
        sTypoDescender=descender,
        sTypoLineGap=200,
        usWinAscent=ascender,
        usWinDescent=abs(descender),
        sxHeight=500,
        sCapHeight=750,
    )
    fb.setupPost()
    fb.setupNameTable({
        "familyName": font_name,
        "styleName": "Regular",
    })

    fb.save(output_path)
    print(f"Dark Fantasy font saved to: {output_path}")
    print(f"Glyphs: {glyph_order}")

    return output_path


def render_preview(font_path: str, output_path: str):
    """Render a preview of the font"""
    from PIL import Image, ImageDraw, ImageFont

    img = Image.new('RGB', (600, 200), (20, 15, 25))  # Dark purple-black bg
    draw = ImageDraw.Draw(img)

    try:
        font_large = ImageFont.truetype(font_path, 80)
        font_small = ImageFont.truetype(font_path, 40)

        # Gold/bronze color for dark fantasy feel
        color = (218, 165, 32)  # Goldenrod

        draw.text((30, 20), "ABWXZ", font=font_large, fill=color)
        draw.text((30, 120), "A B W X Z", font=font_small, fill=(180, 140, 80))

        img.save(output_path)
        print(f"Preview saved to: {output_path}")
        return True
    except Exception as e:
        print(f"Preview failed: {e}")
        return False


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    font_path = os.path.join(script_dir, "DarkFantasy.ttf")
    preview_path = os.path.join(script_dir, "DarkFantasy_preview.png")

    create_dark_fantasy_font(font_path)
    render_preview(font_path, preview_path)
