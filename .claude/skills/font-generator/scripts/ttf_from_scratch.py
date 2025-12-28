"""
TTF Font Generation from Scratch using fonttools
No SVG intermediate - draw glyphs directly with pen commands
"""

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.t2CharStringPen import T2CharStringPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
import os


def draw_letter_A(pen, width=600, height=700):
    """Draw a simple capital A"""
    # Outer shape (clockwise for solid)
    pen.moveTo((width // 2, height))  # Top point
    pen.lineTo((width - 50, 0))        # Bottom right
    pen.lineTo((width - 150, 0))       # Inner bottom right
    pen.lineTo((width // 2 + 30, 250)) # Right side of crossbar gap
    pen.lineTo((width // 2 - 30, 250)) # Left side of crossbar gap
    pen.lineTo((150, 0))               # Inner bottom left
    pen.lineTo((50, 0))                # Bottom left
    pen.closePath()

    # Crossbar hole (counter-clockwise for hole)
    pen.moveTo((width // 2, 450))       # Top of triangle hole
    pen.lineTo((width // 2 + 60, 300))  # Bottom right
    pen.lineTo((width // 2 - 60, 300))  # Bottom left
    pen.closePath()


def draw_letter_B(pen, width=550, height=700):
    """Draw a simple capital B"""
    stem_width = 100

    # Outer shape (clockwise)
    pen.moveTo((50, 0))
    pen.lineTo((50, height))
    pen.lineTo((350, height))
    pen.qCurveTo((500, height), (500, height - 100))
    pen.qCurveTo((500, height // 2 + 50), (350, height // 2))
    pen.lineTo((380, height // 2))
    pen.qCurveTo((530, height // 2), (530, height // 4))
    pen.qCurveTo((530, 0), (350, 0))
    pen.closePath()

    # Top hole (counter-clockwise - reversed from outer)
    pen.moveTo((50 + stem_width, height // 2 + 80))
    pen.lineTo((320, height // 2 + 80))
    pen.qCurveTo((400, height // 2 + 80), (400, height - 150))
    pen.qCurveTo((400, height - 80), (320, height - 80))
    pen.lineTo((50 + stem_width, height - 80))
    pen.closePath()

    # Bottom hole (counter-clockwise - reversed from outer)
    pen.moveTo((50 + stem_width, 80))
    pen.lineTo((340, 80))
    pen.qCurveTo((430, 80), (430, height // 4))
    pen.qCurveTo((430, height // 2 - 80), (340, height // 2 - 80))
    pen.lineTo((50 + stem_width, height // 2 - 80))
    pen.closePath()


def draw_letter_C(pen, width=550, height=700):
    """Draw a simple capital C"""
    # Outer arc
    pen.moveTo((width - 50, height - 150))
    pen.qCurveTo((width - 50, height), (width // 2, height))
    pen.qCurveTo((50, height), (50, height // 2))
    pen.qCurveTo((50, 0), (width // 2, 0))
    pen.qCurveTo((width - 50, 0), (width - 50, 150))
    pen.lineTo((width - 150, 150))
    pen.qCurveTo((width - 150, 100), (width // 2, 100))
    pen.qCurveTo((150, 100), (150, height // 2))
    pen.qCurveTo((150, height - 100), (width // 2, height - 100))
    pen.qCurveTo((width - 150, height - 100), (width - 150, height - 150))
    pen.closePath()


def draw_letter_O(pen, width=600, height=700):
    """Draw a simple capital O with proper winding for hole"""
    cx, cy = width // 2, height // 2
    outer_rx, outer_ry = 250, 350
    inner_rx, inner_ry = 150, 250

    # Outer ellipse (clockwise in font coords = counter-clockwise visually)
    # We'll approximate with 4 quadratic curves
    pen.moveTo((cx, cy + outer_ry))  # Top
    pen.qCurveTo((cx + outer_rx, cy + outer_ry), (cx + outer_rx, cy))  # Top-right
    pen.qCurveTo((cx + outer_rx, cy - outer_ry), (cx, cy - outer_ry))  # Bottom-right
    pen.qCurveTo((cx - outer_rx, cy - outer_ry), (cx - outer_rx, cy))  # Bottom-left
    pen.qCurveTo((cx - outer_rx, cy + outer_ry), (cx, cy + outer_ry))  # Top-left
    pen.closePath()

    # Inner ellipse (opposite winding = clockwise visually for hole)
    pen.moveTo((cx, cy + inner_ry))  # Top
    pen.qCurveTo((cx - inner_rx, cy + inner_ry), (cx - inner_rx, cy))  # Top-left (reversed!)
    pen.qCurveTo((cx - inner_rx, cy - inner_ry), (cx, cy - inner_ry))  # Bottom-left
    pen.qCurveTo((cx + inner_rx, cy - inner_ry), (cx + inner_rx, cy))  # Bottom-right
    pen.qCurveTo((cx + inner_rx, cy + inner_ry), (cx, cy + inner_ry))  # Top-right
    pen.closePath()


def draw_letter_D(pen, width=550, height=700):
    """Draw a simple capital D with hole"""
    # Outer shape (stem + curved right side)
    pen.moveTo((50, 0))
    pen.lineTo((50, height))
    pen.lineTo((250, height))
    pen.qCurveTo((500, height), (500, height // 2))
    pen.qCurveTo((500, 0), (250, 0))
    pen.closePath()

    # Inner hole (opposite winding)
    pen.moveTo((150, 100))
    pen.lineTo((230, 100))
    pen.qCurveTo((400, 100), (400, height // 2))
    pen.qCurveTo((400, height - 100), (230, height - 100))
    pen.lineTo((150, height - 100))
    pen.closePath()


def draw_square(pen, width=500, height=500):
    """Draw a simple square - for testing"""
    pen.moveTo((50, 50))
    pen.lineTo((50, height - 50))
    pen.lineTo((width - 50, height - 50))
    pen.lineTo((width - 50, 50))
    pen.closePath()


def draw_notdef(pen, width=500, height=700):
    """Draw .notdef glyph (rectangle with X)"""
    # Outer rectangle (clockwise)
    pen.moveTo((50, 0))
    pen.lineTo((50, height))
    pen.lineTo((width - 50, height))
    pen.lineTo((width - 50, 0))
    pen.closePath()

    # Inner rectangle hole (counter-clockwise)
    pen.moveTo((100, 50))
    pen.lineTo((width - 100, 50))
    pen.lineTo((width - 100, height - 50))
    pen.lineTo((100, height - 50))
    pen.closePath()


def create_ttf_font(output_path: str, font_name: str = "TestFont"):
    """
    Create a TTF font from scratch with a few glyphs.
    """
    # Font metrics
    units_per_em = 1000
    ascender = 800
    descender = -200

    # Define glyphs and their Unicode mappings
    glyph_order = [".notdef", "space", "A", "B", "C", "D", "O"]

    # Character map: Unicode codepoint -> glyph name
    cmap = {
        0x0020: "space",  # Space
        0x0041: "A",      # A
        0x0042: "B",      # B
        0x0043: "C",      # C
        0x0044: "D",      # D
        0x004F: "O",      # O
    }

    # Glyph widths (advance width)
    advance_widths = {
        ".notdef": 500,
        "space": 250,
        "A": 650,
        "B": 600,
        "C": 600,
        "D": 600,
        "O": 650,
    }

    # Create FontBuilder
    fb = FontBuilder(units_per_em, isTTF=True)
    fb.setupGlyphOrder(glyph_order)
    fb.setupCharacterMap(cmap)

    # Draw glyphs
    pen_class = TTGlyphPen
    glyphs = {}

    # .notdef
    pen = pen_class(None)
    draw_notdef(pen, width=500, height=700)
    glyphs[".notdef"] = pen.glyph()

    # space (empty glyph)
    pen = pen_class(None)
    glyphs["space"] = pen.glyph()

    # A
    pen = pen_class(None)
    draw_letter_A(pen, width=600, height=700)
    glyphs["A"] = pen.glyph()

    # B
    pen = pen_class(None)
    draw_letter_B(pen, width=550, height=700)
    glyphs["B"] = pen.glyph()

    # C
    pen = pen_class(None)
    draw_letter_C(pen, width=550, height=700)
    glyphs["C"] = pen.glyph()

    # D
    pen = pen_class(None)
    draw_letter_D(pen, width=550, height=700)
    glyphs["D"] = pen.glyph()

    # O
    pen = pen_class(None)
    draw_letter_O(pen, width=600, height=700)
    glyphs["O"] = pen.glyph()

    # Setup glyph table
    fb.setupGlyf(glyphs)

    # Setup metrics
    metrics = {name: (advance_widths[name], 0) for name in glyph_order}
    fb.setupHorizontalMetrics(metrics)

    # Setup horizontal header
    fb.setupHorizontalHeader(ascent=ascender, descent=descender)

    # Setup head table
    fb.setupHead(unitsPerEm=units_per_em)

    # Setup OS/2 table
    fb.setupOS2(
        sTypoAscender=ascender,
        sTypoDescender=descender,
        sTypoLineGap=200,
        usWinAscent=ascender,
        usWinDescent=abs(descender),
        sxHeight=500,
        sCapHeight=700,
    )

    # Setup post table
    fb.setupPost()

    # Setup name table
    fb.setupNameTable({
        "familyName": font_name,
        "styleName": "Regular",
    })

    # Save the font
    fb.save(output_path)
    print(f"Font saved to: {output_path}")

    # Verify by reopening
    font = TTFont(output_path)
    print(f"Glyphs in font: {font.getGlyphOrder()}")
    print(f"Character map entries: {len(font.getBestCmap())}")

    return output_path


if __name__ == "__main__":
    output_dir = os.path.dirname(os.path.abspath(__file__))
    output_file = os.path.join(output_dir, "TestFont.ttf")
    create_ttf_font(output_file, "TestFont")
