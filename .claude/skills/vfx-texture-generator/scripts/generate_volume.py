#!/usr/bin/env python3
"""
VFX Texture Generator - Volume Texture Generator
Generates 3D volume textures as 2D slice grids for UE5.
"""

import argparse
import os
import sys
import math
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from patterns import PatternGenerator


def parse_args():
    parser = argparse.ArgumentParser(description='Generate volume texture (tiled slices)')
    
    # Basic params
    parser.add_argument('--pattern', type=str, default='simplex',
                       choices=['simplex', 'fbm'],
                       help='Pattern type (3D capable)')
    parser.add_argument('--resolution', type=int, default=256,
                       choices=[64, 128, 256, 512], help='Per-slice resolution')
    parser.add_argument('--slices', type=int, default=16,
                       choices=[8, 16, 32, 64], help='Number of Z slices')
    parser.add_argument('--output', type=str, default='./', help='Output directory')
    parser.add_argument('--name', type=str, default=None, help='Custom filename')
    parser.add_argument('--seed', type=int, default=None, help='Random seed')
    parser.add_argument('--tileable', action='store_true', help='Make tileable in XY')
    
    # Noise params
    parser.add_argument('--frequency', type=float, default=4.0, help='Noise frequency')
    parser.add_argument('--octaves', type=int, default=4, help='Noise octaves')
    parser.add_argument('--persistence', type=float, default=0.5, help='Octave persistence')
    parser.add_argument('--lacunarity', type=float, default=2.0, help='Octave lacunarity')
    
    # FBM specific
    parser.add_argument('--ridged', action='store_true', help='Ridged noise')
    parser.add_argument('--turbulence', action='store_true', help='Turbulent noise')
    
    # Modifiers
    parser.add_argument('--invert', action='store_true', help='Invert output')
    parser.add_argument('--contrast', type=float, default=1.0, help='Contrast')
    parser.add_argument('--brightness', type=float, default=0.0, help='Brightness')
    parser.add_argument('--gamma', type=float, default=1.0, help='Gamma')
    
    # Alpha
    parser.add_argument('--alpha_source', type=str, default='value',
                       choices=['value', 'edge', 'threshold'])
    parser.add_argument('--alpha_threshold', type=float, default=0.1)
    parser.add_argument('--alpha_softness', type=float, default=0.1)
    parser.add_argument('--alpha_multiply', type=float, default=1.0)
    
    return parser.parse_args()


def calculate_grid_layout(slices: int) -> tuple:
    """Calculate optimal grid layout for slices."""
    # Try to make it as square as possible
    cols = int(math.ceil(math.sqrt(slices)))
    rows = int(math.ceil(slices / cols))
    return cols, rows


def generate_volume(args):
    """Generate 3D volume data."""
    gen = PatternGenerator(seed=args.seed)
    size = args.resolution
    depth = args.slices
    
    print(f"Generating {depth} slices of {size}x{size}...")
    
    # Generate 3D pattern
    if args.pattern == 'simplex':
        volume = gen.simplex_3d(
            size, depth, args.frequency, args.octaves,
            args.persistence, args.lacunarity, args.tileable
        )
    elif args.pattern == 'fbm':
        volume = gen.fbm_3d(
            size, depth, args.frequency, args.octaves,
            args.persistence, args.lacunarity,
            args.ridged, args.turbulence, args.tileable
        )
    else:
        raise ValueError(f"Unknown pattern: {args.pattern}")
    
    # Apply modifiers to each slice
    for z in range(depth):
        volume[z] = PatternGenerator.apply_modifiers(
            volume[z], args.invert, args.contrast,
            args.brightness, args.gamma
        )
    
    return volume


def compose_volume_grid(volume: np.ndarray, args) -> tuple:
    """Compose 3D volume into 2D grid of slices."""
    depth, height, width = volume.shape
    cols, rows = calculate_grid_layout(depth)
    
    out_width = width * cols
    out_height = height * rows
    
    grid_data = np.zeros((out_height, out_width), dtype=np.float64)
    grid_alpha = np.zeros((out_height, out_width), dtype=np.float64)
    
    for z in range(depth):
        row = z // cols
        col = z % cols
        
        y_start = row * height
        x_start = col * width
        
        slice_data = volume[z]
        slice_alpha = PatternGenerator.generate_alpha(
            slice_data, args.alpha_source,
            args.alpha_threshold, args.alpha_softness,
            args.alpha_multiply
        )
        
        grid_data[y_start:y_start+height, x_start:x_start+width] = slice_data
        grid_alpha[y_start:y_start+height, x_start:x_start+width] = slice_alpha
    
    return grid_data, grid_alpha, cols, rows


def save_volume(data: np.ndarray, alpha: np.ndarray, args, cols: int, rows: int):
    """Save volume texture as PNG with alpha."""
    height, width = data.shape
    
    # Convert to 8-bit
    gray = (data * 255).astype(np.uint8)
    alpha_8 = (alpha * 255).astype(np.uint8)
    
    # Create RGBA image
    img = Image.new('RGBA', (width, height))
    for y in range(height):
        for x in range(width):
            v = gray[y, x]
            a = alpha_8[y, x]
            img.putpixel((x, y), (v, v, v, a))
    
    # Determine filename
    if args.name:
        filename = args.name
        if not filename.endswith('.png'):
            filename += '.png'
    else:
        filename = f"volume_{args.pattern}_{args.resolution}_{args.slices}slices.png"
    
    # Ensure output directory exists
    os.makedirs(args.output, exist_ok=True)
    filepath = os.path.join(args.output, filename)
    
    img.save(filepath, 'PNG')
    print(f"Saved: {filepath}")
    print(f"  Total size: {width}x{height}")
    print(f"  Slice size: {args.resolution}x{args.resolution}")
    print(f"  Grid: {cols}x{rows} ({args.slices} slices)")
    print(f"\nUE5 Import:")
    print(f"  Set Texture Group: VolumeTexture")
    print(f"  Tile X: {cols}, Tile Y: {rows}")
    return filepath


def main():
    args = parse_args()
    volume = generate_volume(args)
    data, alpha, cols, rows = compose_volume_grid(volume, args)
    save_volume(data, alpha, args, cols, rows)


if __name__ == '__main__':
    main()
