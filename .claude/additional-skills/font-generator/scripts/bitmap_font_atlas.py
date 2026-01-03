#!/usr/bin/env python3
"""
Bitmap Font Atlas Generator for Unreal Engine

Generates a texture atlas and metrics JSON from individual glyph images.
Output is compatible with UE's Slate font system.

Usage:
    python bitmap_font_atlas.py --input ./glyphs --output ./output --name MyFont
"""

import argparse
import json
import math
from pathlib import Path
from PIL import Image


def calculate_atlas_size(glyph_sizes: list[tuple[int, int]], padding: int = 2) -> tuple[int, int]:
    """Calculate optimal atlas size for given glyph dimensions."""
    total_area = sum((w + padding) * (h + padding) for w, h in glyph_sizes)
    
    # Start with power-of-2 size that fits area
    size = 256
    while size * size < total_area * 1.3:  # 30% buffer
        size *= 2
    
    return size, size


def pack_glyphs(glyphs: dict, atlas_size: tuple[int, int], padding: int = 2) -> dict:
    """
    Simple row-based packing algorithm.
    Returns dict mapping glyph names to (x, y, w, h) positions.
    """
    positions = {}
    x, y = padding, padding
    row_height = 0
    atlas_w, atlas_h = atlas_size
    
    # Sort by height descending for better packing
    sorted_glyphs = sorted(glyphs.items(), key=lambda x: x[1].height, reverse=True)
    
    for name, img in sorted_glyphs:
        w, h = img.size
        
        # Check if glyph fits in current row
        if x + w + padding > atlas_w:
            x = padding
            y += row_height + padding
            row_height = 0
        
        # Check if glyph fits in atlas
        if y + h + padding > atlas_h:
            raise ValueError(f"Atlas too small for all glyphs. Current: {atlas_size}")
        
        positions[name] = (x, y, w, h)
        x += w + padding
        row_height = max(row_height, h)
    
    return positions


def create_atlas(glyphs: dict, positions: dict, atlas_size: tuple[int, int]) -> Image.Image:
    """Composite glyphs into atlas image."""
    atlas = Image.new('RGBA', atlas_size, (0, 0, 0, 0))
    
    for name, img in glyphs.items():
        x, y, w, h = positions[name]
        atlas.paste(img, (x, y))
    
    return atlas


def generate_metrics(positions: dict, atlas_size: tuple[int, int], 
                     line_height: int, baseline: int) -> dict:
    """
    Generate UE-compatible font metrics JSON.
    """
    atlas_w, atlas_h = atlas_size
    
    characters = {}
    for name, (x, y, w, h) in positions.items():
        # Extract character code from filename (e.g., "A.png" -> 65, "0x0041.png" -> 65)
        if name.startswith('0x'):
            char_code = int(name, 16)
        elif len(name) == 1:
            char_code = ord(name)
        else:
            # Try to parse as character name
            char_code = ord(name[0]) if name else 0
        
        characters[char_code] = {
            "uvX": x / atlas_w,
            "uvY": y / atlas_h,
            "uvWidth": w / atlas_w,
            "uvHeight": h / atlas_h,
            "width": w,
            "height": h,
            "xOffset": 0,
            "yOffset": baseline - h,
            "xAdvance": w + 1  # Horizontal advance after character
        }
    
    return {
        "name": "GeneratedFont",
        "atlasWidth": atlas_w,
        "atlasHeight": atlas_h,
        "lineHeight": line_height,
        "baseline": baseline,
        "characters": characters
    }


def main():
    parser = argparse.ArgumentParser(description='Generate bitmap font atlas for Unreal Engine')
    parser.add_argument('--input', '-i', required=True, help='Directory containing glyph images')
    parser.add_argument('--output', '-o', required=True, help='Output directory')
    parser.add_argument('--name', '-n', default='BitmapFont', help='Font name')
    parser.add_argument('--padding', '-p', type=int, default=2, help='Padding between glyphs')
    parser.add_argument('--line-height', type=int, default=0, help='Line height (auto if 0)')
    parser.add_argument('--baseline', type=int, default=0, help='Baseline offset (auto if 0)')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load glyph images
    glyphs = {}
    supported_formats = {'.png', '.jpg', '.jpeg', '.bmp', '.tga'}
    
    for img_path in input_dir.iterdir():
        if img_path.suffix.lower() in supported_formats:
            name = img_path.stem
            glyphs[name] = Image.open(img_path).convert('RGBA')
    
    if not glyphs:
        print(f"No glyph images found in {input_dir}")
        return
    
    print(f"Loaded {len(glyphs)} glyphs")
    
    # Calculate sizes
    glyph_sizes = [(img.width, img.height) for img in glyphs.values()]
    atlas_size = calculate_atlas_size(glyph_sizes, args.padding)
    print(f"Atlas size: {atlas_size[0]}x{atlas_size[1]}")
    
    # Calculate line height and baseline if not specified
    max_height = max(img.height for img in glyphs.values())
    line_height = args.line_height if args.line_height > 0 else int(max_height * 1.2)
    baseline = args.baseline if args.baseline > 0 else max_height
    
    # Pack and create atlas
    positions = pack_glyphs(glyphs, atlas_size, args.padding)
    atlas = create_atlas(glyphs, positions, atlas_size)
    
    # Save outputs
    atlas_path = output_dir / f"{args.name}_atlas.png"
    metrics_path = output_dir / f"{args.name}_metrics.json"
    
    atlas.save(atlas_path)
    print(f"Saved atlas: {atlas_path}")
    
    metrics = generate_metrics(positions, atlas_size, line_height, baseline)
    metrics["name"] = args.name
    
    with open(metrics_path, 'w') as f:
        json.dump(metrics, f, indent=2)
    print(f"Saved metrics: {metrics_path}")
    
    print(f"\nTo use in Unreal Engine:")
    print(f"1. Import {atlas_path} as a texture")
    print(f"2. Create a Slate Bitmap Font asset")
    print(f"3. Configure using values from {metrics_path}")


if __name__ == '__main__':
    main()
