#!/usr/bin/env python3
"""
VFX Texture Generator - Tileable Noise Generator
Generates seamlessly tileable noise textures.
"""

import argparse
import os
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from patterns import PatternGenerator


def parse_args():
    parser = argparse.ArgumentParser(description='Generate tileable noise texture')
    
    # Basic params
    parser.add_argument('--pattern', type=str, default='simplex',
                       choices=['simplex', 'fbm', 'voronoi', 'caustics'],
                       help='Pattern type')
    parser.add_argument('--resolution', type=int, default=512,
                       choices=[256, 512, 1024, 2048], help='Output resolution')
    parser.add_argument('--output', type=str, default='./', help='Output directory')
    parser.add_argument('--name', type=str, default=None, help='Custom filename')
    parser.add_argument('--seed', type=int, default=None, help='Random seed')
    parser.add_argument('--tileable', action='store_true', default=True,
                       help='Make tileable (default: True)')
    
    # Noise params
    parser.add_argument('--frequency', type=float, default=4.0, help='Noise frequency')
    parser.add_argument('--octaves', type=int, default=4, help='Noise octaves')
    parser.add_argument('--persistence', type=float, default=0.5, help='Octave persistence')
    parser.add_argument('--lacunarity', type=float, default=2.0, help='Octave lacunarity')
    
    # FBM specific
    parser.add_argument('--ridged', action='store_true', help='Ridged noise')
    parser.add_argument('--turbulence', action='store_true', help='Turbulent noise')
    
    # Voronoi specific
    parser.add_argument('--cells', type=int, default=16, help='Voronoi cell count')
    parser.add_argument('--metric', type=str, default='euclidean',
                       choices=['euclidean', 'manhattan', 'chebyshev'])
    parser.add_argument('--mode', type=str, default='f1',
                       choices=['f1', 'f2', 'f2-f1', 'edge'])
    parser.add_argument('--jitter', type=float, default=1.0, help='Cell jitter')
    
    # Caustics specific
    parser.add_argument('--scale', type=float, default=4.0, help='Caustics scale')
    parser.add_argument('--time', type=float, default=0.0, help='Caustics time')
    parser.add_argument('--intensity', type=float, default=1.0, help='Caustics intensity')
    parser.add_argument('--layers', type=int, default=3, help='Caustics layers')
    parser.add_argument('--distortion', type=float, default=0.5, help='Caustics distortion')
    
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


def generate_noise(args):
    """Generate tileable noise based on arguments."""
    gen = PatternGenerator(seed=args.seed)
    size = args.resolution
    
    # Generate pattern (always tileable for this script)
    if args.pattern == 'simplex':
        data = gen.simplex_2d(
            size, args.frequency, args.octaves,
            args.persistence, args.lacunarity, tileable=True
        )
    elif args.pattern == 'fbm':
        data = gen.fbm_2d(
            size, args.frequency, args.octaves,
            args.persistence, args.lacunarity,
            args.ridged, args.turbulence, tileable=True
        )
    elif args.pattern == 'voronoi':
        data = gen.voronoi_2d(
            size, args.cells, args.metric,
            args.mode, args.jitter, tileable=True
        )
    elif args.pattern == 'caustics':
        data = gen.caustics(
            size, args.scale, args.time,
            args.intensity, args.layers,
            args.distortion, tileable=True
        )
    else:
        raise ValueError(f"Unknown pattern: {args.pattern}")
    
    # Apply modifiers
    data = PatternGenerator.apply_modifiers(
        data, args.invert, args.contrast,
        args.brightness, args.gamma
    )
    
    # Generate alpha
    alpha = PatternGenerator.generate_alpha(
        data, args.alpha_source,
        args.alpha_threshold, args.alpha_softness,
        args.alpha_multiply
    )
    
    return data, alpha


def save_noise(data: np.ndarray, alpha: np.ndarray, args):
    """Save noise as PNG with alpha."""
    # Convert to 8-bit
    gray = (data * 255).astype(np.uint8)
    alpha_8 = (alpha * 255).astype(np.uint8)
    
    # Create RGBA image
    img = Image.new('RGBA', (args.resolution, args.resolution))
    for y in range(args.resolution):
        for x in range(args.resolution):
            v = gray[y, x]
            a = alpha_8[y, x]
            img.putpixel((x, y), (v, v, v, a))
    
    # Determine filename
    if args.name:
        filename = args.name
        if not filename.endswith('.png'):
            filename += '.png'
    else:
        filename = f"noise_{args.pattern}_{args.resolution}_tileable.png"
    
    # Ensure output directory exists
    os.makedirs(args.output, exist_ok=True)
    filepath = os.path.join(args.output, filename)
    
    img.save(filepath, 'PNG')
    print(f"Saved: {filepath}")
    return filepath


def main():
    args = parse_args()
    data, alpha = generate_noise(args)
    save_noise(data, alpha, args)


if __name__ == '__main__':
    main()
