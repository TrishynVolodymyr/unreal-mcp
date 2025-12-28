#!/usr/bin/env python3
"""
SDF Font Atlas Generator for Unreal Engine

Generates a Signed Distance Field atlas from high-res glyph images.
SDF fonts scale perfectly at any size - the industry standard for game fonts.

Usage:
    python generate_sdf_atlas.py --input ./glyphs --output ./output --name MyFont

Requirements:
    pip install pillow numpy scipy --break-system-packages

For best results, provide glyph images at 256px+ height.
"""

import argparse
import json
import math
from pathlib import Path
from typing import Optional

import numpy as np
from PIL import Image
from scipy import ndimage


def generate_sdf(image: Image.Image, spread: int = 32) -> Image.Image:
    """
    Generate Signed Distance Field from a binary/grayscale glyph image.
    
    Args:
        image: Input glyph image (white glyph on transparent/black background)
        spread: Distance field spread in pixels (affects edge softness range)
    
    Returns:
        Grayscale SDF image where:
        - 0.5 (128) = edge
        - >0.5 = inside glyph
        - <0.5 = outside glyph
    """
    # Convert to grayscale numpy array
    if image.mode == 'RGBA':
        # Use alpha channel as the shape
        arr = np.array(image)[:, :, 3].astype(np.float32) / 255.0
    elif image.mode == 'LA':
        arr = np.array(image)[:, :, 1].astype(np.float32) / 255.0
    else:
        arr = np.array(image.convert('L')).astype(np.float32) / 255.0
    
    # Threshold to binary
    binary = (arr > 0.5).astype(np.float32)
    
    # Calculate distance transform for inside and outside
    # Distance to nearest 0 (outside the glyph)
    dist_outside = ndimage.distance_transform_edt(binary)
    # Distance to nearest 1 (inside the glyph) 
    dist_inside = ndimage.distance_transform_edt(1 - binary)
    
    # Signed distance: positive inside, negative outside
    signed_dist = dist_outside - dist_inside
    
    # Normalize to 0-1 range with spread
    # 0.5 is the edge, values spread out from there
    normalized = (signed_dist / spread) * 0.5 + 0.5
    normalized = np.clip(normalized, 0, 1)
    
    # Convert to 8-bit image
    sdf_array = (normalized * 255).astype(np.uint8)
    return Image.fromarray(sdf_array, mode='L')


def generate_msdf(image: Image.Image, spread: int = 32) -> Image.Image:
    """
    Generate Multi-channel Signed Distance Field.
    MSDF uses RGB channels to preserve sharp corners better than single-channel SDF.
    
    This is a simplified MSDF - for production, consider using msdfgen tool.
    """
    # For now, generate single-channel SDF and replicate to RGB
    # True MSDF requires edge-aware channel separation
    sdf = generate_sdf(image, spread)
    
    # Convert to RGB (simplified - real MSDF has different values per channel)
    sdf_array = np.array(sdf)
    msdf_array = np.stack([sdf_array, sdf_array, sdf_array], axis=-1)
    
    return Image.fromarray(msdf_array, mode='RGB')


def calculate_atlas_size(glyph_sizes: list, padding: int = 4) -> tuple:
    """Calculate power-of-2 atlas size that fits all glyphs."""
    total_area = sum((w + padding) * (h + padding) for w, h in glyph_sizes)
    
    size = 256
    while size * size < total_area * 1.4:  # 40% buffer for packing inefficiency
        size *= 2
        if size > 4096:
            raise ValueError("Glyphs too large for reasonable atlas size")
    
    return size, size


def pack_glyphs_shelf(glyphs: dict, atlas_size: tuple, padding: int = 4) -> dict:
    """
    Shelf packing algorithm - simple and effective.
    Returns dict mapping glyph names to (x, y, w, h) positions.
    """
    positions = {}
    atlas_w, atlas_h = atlas_size
    
    # Sort by height descending
    sorted_items = sorted(glyphs.items(), key=lambda x: x[1].height, reverse=True)
    
    shelf_y = padding
    shelf_height = 0
    x = padding
    
    for name, img in sorted_items:
        w, h = img.size
        
        # Start new shelf if needed
        if x + w + padding > atlas_w:
            shelf_y += shelf_height + padding
            shelf_height = 0
            x = padding
        
        # Check vertical overflow
        if shelf_y + h + padding > atlas_h:
            raise ValueError(f"Atlas {atlas_size} too small. Try larger size or fewer glyphs.")
        
        positions[name] = (x, shelf_y, w, h)
        x += w + padding
        shelf_height = max(shelf_height, h)
    
    return positions


def create_sdf_atlas(glyphs: dict, positions: dict, atlas_size: tuple, 
                     spread: int = 32, use_msdf: bool = False) -> Image.Image:
    """Create SDF atlas from glyph images."""
    
    if use_msdf:
        atlas = Image.new('RGB', atlas_size, (0, 0, 0))
    else:
        atlas = Image.new('L', atlas_size, 0)
    
    for name, original_img in glyphs.items():
        x, y, w, h = positions[name]
        
        # Generate SDF for this glyph
        if use_msdf:
            sdf_glyph = generate_msdf(original_img, spread)
        else:
            sdf_glyph = generate_sdf(original_img, spread)
        
        # Resize if needed (SDF should match original dimensions)
        if sdf_glyph.size != (w, h):
            sdf_glyph = sdf_glyph.resize((w, h), Image.Resampling.LANCZOS)
        
        atlas.paste(sdf_glyph, (x, y))
    
    return atlas


def char_code_from_name(name: str) -> Optional[int]:
    """Parse character code from filename."""
    # Handle hex format: 0x0041 or U+0041
    if name.lower().startswith('0x'):
        return int(name, 16)
    if name.lower().startswith('u+'):
        return int(name[2:], 16)

    # Handle upper_A / lower_a format (Windows case-insensitive workaround)
    if name.startswith('upper_') and len(name) == 7:
        return ord(name[6].upper())
    if name.startswith('lower_') and len(name) == 7:
        return ord(name[6].lower())

    # Single character
    if len(name) == 1:
        return ord(name)

    # Common glyph names
    name_map = {
        'space': 32, 'exclam': 33, 'quotedbl': 34, 'numbersign': 35,
        'dollar': 36, 'percent': 37, 'ampersand': 38, 'quotesingle': 39,
        'parenleft': 40, 'parenright': 41, 'asterisk': 42, 'plus': 43,
        'comma': 44, 'hyphen': 45, 'minus': 45, 'period': 46, 'slash': 47,
        'zero': 48, 'one': 49, 'two': 50, 'three': 51, 'four': 52,
        'five': 53, 'six': 54, 'seven': 55, 'eight': 56, 'nine': 57,
        'colon': 58, 'semicolon': 59, 'less': 60, 'equal': 61,
        'greater': 62, 'question': 63, 'at': 64,
        'bracketleft': 91, 'backslash': 92, 'bracketright': 93,
        'asciicircum': 94, 'underscore': 95, 'grave': 96,
        'braceleft': 123, 'bar': 124, 'braceright': 125, 'asciitilde': 126,
    }
    
    return name_map.get(name.lower())


def generate_metrics(positions: dict, atlas_size: tuple, 
                     line_height: int, baseline: int, spread: int) -> dict:
    """Generate UE-compatible font metrics."""
    atlas_w, atlas_h = atlas_size
    
    characters = {}
    for name, (x, y, w, h) in positions.items():
        char_code = char_code_from_name(name)
        if char_code is None:
            print(f"Warning: Could not determine char code for '{name}', skipping in metrics")
            continue
        
        characters[str(char_code)] = {
            # UV coordinates (normalized 0-1)
            "u": x / atlas_w,
            "v": y / atlas_h,
            "u2": (x + w) / atlas_w,
            "v2": (y + h) / atlas_h,
            # Pixel dimensions
            "width": w,
            "height": h,
            # Positioning
            "xOffset": 0,
            "yOffset": 0,
            "xAdvance": w + 2,  # Horizontal advance
            # For text rendering
            "char": chr(char_code) if 32 <= char_code < 127 else f"U+{char_code:04X}"
        }
    
    return {
        "name": "SDFFont",
        "type": "sdf",
        "spread": spread,
        "atlasWidth": atlas_w,
        "atlasHeight": atlas_h,
        "lineHeight": line_height,
        "baseline": baseline,
        "characters": characters
    }


def main():
    parser = argparse.ArgumentParser(
        description='Generate SDF font atlas for Unreal Engine',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python generate_sdf_atlas.py -i ./glyphs -o ./output -n DemonicFont
    python generate_sdf_atlas.py -i ./glyphs -o ./output --msdf --spread 48

Glyph naming:
    A.png, B.png, ...           - Single characters
    0x0041.png, U+0041.png      - Unicode hex
    space.png, comma.png        - Standard names
        """
    )
    
    parser.add_argument('--input', '-i', required=True, 
                        help='Directory containing high-res glyph images')
    parser.add_argument('--output', '-o', required=True, 
                        help='Output directory')
    parser.add_argument('--name', '-n', default='SDFFont', 
                        help='Font name for output files')
    parser.add_argument('--spread', '-s', type=int, default=32,
                        help='SDF spread in pixels (default: 32)')
    parser.add_argument('--padding', '-p', type=int, default=4,
                        help='Padding between glyphs (default: 4)')
    parser.add_argument('--msdf', action='store_true',
                        help='Generate multi-channel SDF (better corners)')
    parser.add_argument('--atlas-size', type=int, default=0,
                        help='Force atlas size (default: auto)')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load glyph images
    glyphs = {}
    supported = {'.png', '.jpg', '.jpeg', '.bmp', '.tga', '.tiff'}
    
    for img_path in sorted(input_dir.iterdir()):
        if img_path.suffix.lower() in supported:
            name = img_path.stem
            img = Image.open(img_path)
            
            # Ensure RGBA for consistent alpha handling
            if img.mode != 'RGBA':
                img = img.convert('RGBA')
            
            glyphs[name] = img
            print(f"Loaded: {img_path.name} ({img.width}x{img.height})")
    
    if not glyphs:
        print(f"No glyph images found in {input_dir}")
        return 1
    
    print(f"\nProcessing {len(glyphs)} glyphs...")
    
    # Calculate atlas size
    glyph_sizes = [(img.width, img.height) for img in glyphs.values()]
    
    if args.atlas_size > 0:
        atlas_size = (args.atlas_size, args.atlas_size)
    else:
        atlas_size = calculate_atlas_size(glyph_sizes, args.padding)
    
    print(f"Atlas size: {atlas_size[0]}x{atlas_size[1]}")
    
    # Pack glyphs
    positions = pack_glyphs_shelf(glyphs, atlas_size, args.padding)
    
    # Generate SDF atlas
    print(f"Generating {'MSDF' if args.msdf else 'SDF'} with spread={args.spread}...")
    atlas = create_sdf_atlas(glyphs, positions, atlas_size, args.spread, args.msdf)
    
    # Calculate metrics
    max_height = max(img.height for img in glyphs.values())
    line_height = int(max_height * 1.2)
    baseline = max_height
    
    # Save outputs
    ext = 'png'
    atlas_path = output_dir / f"{args.name}_sdf.{ext}"
    metrics_path = output_dir / f"{args.name}_metrics.json"
    
    atlas.save(atlas_path)
    print(f"Saved atlas: {atlas_path}")
    
    metrics = generate_metrics(positions, atlas_size, line_height, baseline, args.spread)
    metrics["name"] = args.name
    
    with open(metrics_path, 'w') as f:
        json.dump(metrics, f, indent=2)
    print(f"Saved metrics: {metrics_path}")
    
    # Print UE import instructions
    print(f"""
╔══════════════════════════════════════════════════════════════════╗
║  UNREAL ENGINE IMPORT                                            ║
╠══════════════════════════════════════════════════════════════════╣
║  Option 1: UMG Font with SDF                                     ║
║  1. Import {atlas_path.name} as texture                          ║
║  2. Create Font Face asset → set "Use SDF" = true                ║
║  3. Configure using {metrics_path.name}                          ║
║                                                                  ║
║  Option 2: Custom SDF Material                                   ║
║  1. Import atlas texture (no compression or BC7)                 ║
║  2. Create material with SDF shader (smoothstep on distance)     ║
║  3. Use in Widget or TextRender component                        ║
║                                                                  ║
║  SDF Benefits:                                                   ║
║  • Scales perfectly at any size                                  ║
║  • Easy outline: adjust threshold                                ║
║  • Easy glow: blur + threshold                                   ║
║  • Easy shadow: offset sample + blend                            ║
╚══════════════════════════════════════════════════════════════════╝
    """)
    
    return 0


if __name__ == '__main__':
    exit(main())
