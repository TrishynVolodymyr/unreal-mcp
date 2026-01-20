#!/usr/bin/env python3
"""
VFX Texture Generator - Gradient Ramp Generator
Generates gradient ramps for color over life, falloffs, and masks.
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
    parser = argparse.ArgumentParser(description='Generate gradient ramp texture')
    
    # Basic params
    parser.add_argument('--type', type=str, default='radial',
                       choices=['radial', 'directional', 'horizontal', 'vertical'],
                       help='Gradient type')
    parser.add_argument('--resolution', type=int, default=256,
                       choices=[64, 128, 256, 512, 1024], help='Output resolution')
    parser.add_argument('--output', type=str, default='./', help='Output directory')
    parser.add_argument('--name', type=str, default=None, help='Custom filename')
    
    # Radial specific
    parser.add_argument('--falloff', type=str, default='smooth',
                       choices=['linear', 'smooth', 'exp', 'inv_exp'])
    parser.add_argument('--radius', type=float, default=0.5, help='Radial gradient radius')
    parser.add_argument('--center_x', type=float, default=0.5, help='Center X')
    parser.add_argument('--center_y', type=float, default=0.5, help='Center Y')
    parser.add_argument('--softness', type=float, default=1.0, help='Edge softness')
    parser.add_argument('--power', type=float, default=2.0, help='Falloff power')
    
    # Directional specific
    parser.add_argument('--angle', type=float, default=0.0, help='Gradient angle (0=horizontal)')
    parser.add_argument('--start', type=float, default=0.0, help='Start value')
    parser.add_argument('--end', type=float, default=1.0, help='End value')
    parser.add_argument('--repeat', type=int, default=1, help='Repeat count')
    
    # Modifiers
    parser.add_argument('--invert', action='store_true', help='Invert output')
    parser.add_argument('--contrast', type=float, default=1.0, help='Contrast')
    parser.add_argument('--gamma', type=float, default=1.0, help='Gamma')
    
    # Alpha
    parser.add_argument('--alpha_source', type=str, default='value',
                       choices=['value', 'threshold'])
    parser.add_argument('--alpha_threshold', type=float, default=0.0)
    parser.add_argument('--alpha_softness', type=float, default=0.1)
    
    # 1D ramp option
    parser.add_argument('--ramp_1d', action='store_true',
                       help='Generate 1D ramp (Nx1 texture)')
    
    return parser.parse_args()


def generate_ramp(args):
    """Generate gradient ramp based on arguments."""
    gen = PatternGenerator()
    size = args.resolution
    
    # Handle 1D ramp
    if args.ramp_1d:
        return generate_1d_ramp(args)
    
    # Generate 2D gradient
    if args.type == 'radial':
        data = gen.radial_gradient(
            size, args.falloff, args.radius,
            args.center_x, args.center_y,
            args.softness, args.power
        )
    elif args.type == 'directional':
        data = gen.directional_gradient(
            size, args.angle, args.falloff,
            args.start, args.end, args.repeat
        )
    elif args.type == 'horizontal':
        data = gen.directional_gradient(
            size, 0.0, args.falloff,
            args.start, args.end, args.repeat
        )
    elif args.type == 'vertical':
        data = gen.directional_gradient(
            size, 90.0, args.falloff,
            args.start, args.end, args.repeat
        )
    else:
        raise ValueError(f"Unknown gradient type: {args.type}")
    
    # Apply modifiers
    data = PatternGenerator.apply_modifiers(
        data, args.invert, args.contrast, 0.0, args.gamma
    )
    
    # Generate alpha
    alpha = PatternGenerator.generate_alpha(
        data, args.alpha_source,
        args.alpha_threshold, args.alpha_softness, 1.0
    )
    
    return data, alpha


def generate_1d_ramp(args):
    """Generate 1D gradient ramp (Nx1)."""
    size = args.resolution
    data = np.zeros((1, size), dtype=np.float64)
    
    for x in range(size):
        t = x / (size - 1) if size > 1 else 0
        
        # Apply repeat
        t = (t * args.repeat) % 1.0
        
        # Apply falloff curve
        if args.falloff == 'linear':
            value = t
        elif args.falloff == 'smooth':
            value = t * t * (3 - 2 * t)
        elif args.falloff == 'exp':
            value = 1.0 - np.exp(-t * 3)
        elif args.falloff == 'inv_exp':
            value = np.exp(-(1.0 - t) * 3)
        else:
            value = t
        
        # Remap to start-end
        value = args.start + value * (args.end - args.start)
        data[0, x] = value
    
    # Apply modifiers
    data = PatternGenerator.apply_modifiers(
        data, args.invert, args.contrast, 0.0, args.gamma
    )
    
    alpha = data.copy()
    
    return data, alpha


def save_ramp(data: np.ndarray, alpha: np.ndarray, args):
    """Save ramp as PNG with alpha."""
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
        suffix = '1d' if args.ramp_1d else '2d'
        filename = f"ramp_{args.type}_{args.resolution}_{suffix}.png"
    
    # Ensure output directory exists
    os.makedirs(args.output, exist_ok=True)
    filepath = os.path.join(args.output, filename)
    
    img.save(filepath, 'PNG')
    print(f"Saved: {filepath}")
    print(f"  Size: {width}x{height}")
    return filepath


def main():
    args = parse_args()
    data, alpha = generate_ramp(args)
    save_ramp(data, alpha, args)


if __name__ == '__main__':
    main()
