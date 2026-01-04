#!/usr/bin/env python3
"""
Render high-resolution glyph images from a TTF font for SDF generation.

Usage:
    python render_glyphs.py --font Cinzel-Regular.ttf --output ./glyphs_regular --height 256
    python render_glyphs.py --font Cinzel-Bold.ttf --output ./glyphs_bold --height 256
"""

import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


# Standard character set
CHARSET_STANDARD = (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    " .,!?:;'\"-_"
)

# Character name mapping for filenames (avoiding filesystem issues)
CHAR_NAMES = {
    ' ': 'space',
    '.': 'period',
    ',': 'comma',
    '!': 'exclam',
    '?': 'question',
    ':': 'colon',
    ';': 'semicolon',
    "'": 'quotesingle',
    '"': 'quotedbl',
    '-': 'hyphen',
    '_': 'underscore',
}


def get_filename(char: str) -> str:
    """Get safe filename for a character."""
    if char in CHAR_NAMES:
        return CHAR_NAMES[char]
    elif char.isupper():
        return f"upper_{char}"
    elif char.islower():
        return f"lower_{char}"
    else:
        return char


def render_glyph(char: str, font: ImageFont.FreeTypeFont, height: int,
                 padding: int = 16) -> Image.Image:
    """
    Render a single glyph as white on transparent background.

    Args:
        char: Character to render
        font: PIL ImageFont
        height: Target height for the glyph
        padding: Padding around the glyph

    Returns:
        RGBA image with white glyph on transparent background
    """
    # Get glyph bounding box
    bbox = font.getbbox(char)
    if bbox is None:
        # Fallback for space or missing glyphs
        return Image.new('RGBA', (height // 2, height), (0, 0, 0, 0))

    left, top, right, bottom = bbox
    glyph_width = right - left
    glyph_height = bottom - top

    # Create image with padding
    img_width = glyph_width + padding * 2
    img_height = height + padding * 2

    img = Image.new('RGBA', (img_width, img_height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Calculate position to center the glyph
    x = padding - left
    y = padding - top + (height - glyph_height) // 2

    # Draw glyph in white
    draw.text((x, y), char, font=font, fill=(255, 255, 255, 255))

    return img


def main():
    parser = argparse.ArgumentParser(description='Render TTF glyphs as images')
    parser.add_argument('--font', '-f', required=True, help='Path to TTF/OTF font file')
    parser.add_argument('--output', '-o', required=True, help='Output directory')
    parser.add_argument('--height', '-H', type=int, default=256,
                        help='Glyph height in pixels (default: 256)')
    parser.add_argument('--charset', '-c', default='standard',
                        choices=['standard', 'uppercase', 'custom'],
                        help='Character set to render')
    parser.add_argument('--custom-chars', default='',
                        help='Custom characters to render (with --charset custom)')

    args = parser.parse_args()

    font_path = Path(args.font)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Determine character set
    if args.charset == 'uppercase':
        chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?:;'\"-_"
    elif args.charset == 'custom':
        chars = args.custom_chars
    else:
        chars = CHARSET_STANDARD

    # Load font at appropriate size
    # We want the resulting glyph to be approximately `height` pixels tall
    font_size = int(args.height * 0.85)  # Adjust for typical cap height ratio

    try:
        font = ImageFont.truetype(str(font_path), font_size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return 1

    print(f"Font: {font_path.name}")
    print(f"Target height: {args.height}px")
    print(f"Characters: {len(chars)}")
    print(f"Output: {output_dir}")
    print()

    # Render each character
    for char in chars:
        filename = get_filename(char)
        output_path = output_dir / f"{filename}.png"

        img = render_glyph(char, font, args.height)
        img.save(output_path)

        display_char = char if char != ' ' else '(space)'
        print(f"  {display_char} -> {filename}.png ({img.width}x{img.height})")

    print(f"\nRendered {len(chars)} glyphs to {output_dir}")
    print(f"\nNext: python generate_sdf_atlas.py -i {output_dir} -o ./output -n DarkFantasy")

    return 0


if __name__ == '__main__':
    exit(main())
