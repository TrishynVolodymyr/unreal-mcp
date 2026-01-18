#!/usr/bin/env python3
"""
VFX Texture Generator - Flipbook Generator
Generates animated flipbook sprites with alpha channel.
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
    parser = argparse.ArgumentParser(description='Generate VFX flipbook texture')
    
    # Basic params
    parser.add_argument('--pattern', type=str, default='fbm',
                       choices=['simplex', 'fbm', 'voronoi', 'caustics'],
                       help='Pattern type')
    parser.add_argument('--resolution', type=int, default=512,
                       choices=[256, 512, 1024, 2048], help='Per-frame resolution')
    parser.add_argument('--grid', type=str, default='4x4', help='Grid size (e.g., 8x8)')
    parser.add_argument('--output', type=str, default='./', help='Output directory')
    parser.add_argument('--name', type=str, default=None, help='Custom filename')
    parser.add_argument('--seed', type=int, default=None, help='Random seed')
    
    # Animation params
    parser.add_argument('--animation', type=str, default='evolve',
                       help='Animation type: evolve, scale, rotate, or combinations like evolve+rotate')
    parser.add_argument('--speed', type=float, default=1.0, help='Animation speed multiplier')
    
    # Evolve animation
    parser.add_argument('--evolve_speed', type=float, default=1.0, help='Evolve Z-offset per frame')
    parser.add_argument('--evolve_direction', type=str, default='z',
                       choices=['x', 'y', 'z', 'random'])
    
    # Scale animation
    parser.add_argument('--scale_start', type=float, default=1.0, help='Starting scale')
    parser.add_argument('--scale_end', type=float, default=2.0, help='Ending scale')
    parser.add_argument('--scale_curve', type=str, default='linear',
                       choices=['linear', 'ease_in', 'ease_out', 'ease_both'])
    
    # Rotate animation
    parser.add_argument('--rotate_amount', type=float, default=360, help='Total rotation degrees')
    parser.add_argument('--rotate_direction', type=str, default='cw', choices=['cw', 'ccw'])
    
    # Noise params
    parser.add_argument('--frequency', type=float, default=3.0, help='Noise frequency')
    parser.add_argument('--octaves', type=int, default=6, help='Noise octaves')
    parser.add_argument('--persistence', type=float, default=0.5, help='Octave persistence')
    parser.add_argument('--lacunarity', type=float, default=2.0, help='Octave lacunarity')
    parser.add_argument('--tileable', action='store_true', help='Make tileable')
    
    # FBM specific
    parser.add_argument('--ridged', action='store_true', help='Ridged noise')
    parser.add_argument('--turbulence', action='store_true', help='Turbulent noise')
    
    # Voronoi specific
    parser.add_argument('--cells', type=int, default=12, help='Voronoi cell count')
    parser.add_argument('--metric', type=str, default='euclidean')
    parser.add_argument('--mode', type=str, default='f1')
    parser.add_argument('--jitter', type=float, default=0.9, help='Cell jitter')
    
    # Caustics specific
    parser.add_argument('--scale', type=float, default=4.0, help='Caustics scale')
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


def parse_grid(grid_str: str) -> tuple:
    """Parse grid string like '8x8' into (cols, rows)."""
    parts = grid_str.lower().split('x')
    return int(parts[0]), int(parts[1])


def apply_curve(t: float, curve: str) -> float:
    """Apply easing curve to normalized time."""
    if curve == 'linear':
        return t
    elif curve == 'ease_in':
        return t * t
    elif curve == 'ease_out':
        return 1 - (1 - t) * (1 - t)
    elif curve == 'ease_both':
        return t * t * (3 - 2 * t)
    return t


def generate_frame(
    gen: PatternGenerator,
    args,
    frame: int,
    total_frames: int,
    size: int
) -> np.ndarray:
    """Generate a single frame with animation applied."""
    t = frame / max(1, total_frames - 1)  # Normalized time 0-1
    
    # Parse animation types
    animations = args.animation.lower().split('+')
    
    # Calculate animation parameters
    z_offset = 0.0
    scale_factor = 1.0
    rotation = 0.0
    
    if 'evolve' in animations:
        z_offset = frame * args.evolve_speed * args.speed
    
    if 'scale' in animations:
        scale_t = apply_curve(t, args.scale_curve)
        scale_factor = args.scale_start + (args.scale_end - args.scale_start) * scale_t
    
    if 'rotate' in animations:
        rot_dir = 1 if args.rotate_direction == 'cw' else -1
        rotation = args.rotate_amount * t * rot_dir * args.speed
    
    # Generate base pattern with Z offset for evolution
    if args.pattern == 'simplex':
        # Use 3D noise with Z as time
        data = generate_evolved_simplex(gen, size, args, z_offset, scale_factor, rotation)
    elif args.pattern == 'fbm':
        data = generate_evolved_fbm(gen, size, args, z_offset, scale_factor, rotation)
    elif args.pattern == 'voronoi':
        # Voronoi needs different seed per frame for evolution
        frame_gen = PatternGenerator(seed=(gen.seed + frame * 1000) if 'evolve' in animations else gen.seed)
        data = frame_gen.voronoi_2d(
            size, args.cells, args.metric,
            args.mode, args.jitter, args.tileable
        )
    elif args.pattern == 'caustics':
        time_offset = frame * 0.1 * args.speed if 'evolve' in animations else args.speed
        data = gen.caustics(
            size, args.scale * scale_factor, time_offset,
            args.intensity, args.layers, args.distortion, args.tileable
        )
    else:
        raise ValueError(f"Unknown pattern: {args.pattern}")
    
    # Apply rotation if needed
    if rotation != 0 and args.pattern not in ['caustics']:
        data = rotate_array(data, rotation)
    
    return data


def generate_evolved_simplex(gen, size, args, z_offset, scale_factor, rotation):
    """Generate simplex noise with evolution."""
    result = np.zeros((size, size), dtype=np.float64)
    freq = args.frequency * scale_factor
    
    for y in range(size):
        for x in range(size):
            # Apply rotation to coordinates
            cx, cy = x - size/2, y - size/2
            if rotation != 0:
                rad = math.radians(rotation)
                rx = cx * math.cos(rad) - cy * math.sin(rad)
                ry = cx * math.sin(rad) + cy * math.cos(rad)
                cx, cy = rx, ry
            cx, cy = cx + size/2, cy + size/2
            
            amplitude = 1.0
            f = freq
            value = 0.0
            max_amp = 0.0
            
            for _ in range(args.octaves):
                value += amplitude * gen.noise.noise3(
                    cx * f / size, cy * f / size, z_offset
                )
                max_amp += amplitude
                amplitude *= args.persistence
                f *= args.lacunarity
            
            result[int(y), int(x)] = value / max_amp
    
    result = (result - result.min()) / (result.max() - result.min() + 1e-8)
    return result


def generate_evolved_fbm(gen, size, args, z_offset, scale_factor, rotation):
    """Generate FBM noise with evolution."""
    result = np.zeros((size, size), dtype=np.float64)
    freq = args.frequency * scale_factor
    
    for y in range(size):
        for x in range(size):
            cx, cy = x - size/2, y - size/2
            if rotation != 0:
                rad = math.radians(rotation)
                rx = cx * math.cos(rad) - cy * math.sin(rad)
                ry = cx * math.sin(rad) + cy * math.cos(rad)
                cx, cy = rx, ry
            cx, cy = cx + size/2, cy + size/2
            
            amplitude = 1.0
            f = freq
            value = 0.0
            max_amp = 0.0
            
            for _ in range(args.octaves):
                n = gen.noise.noise3(cx * f / size, cy * f / size, z_offset)
                
                if args.ridged:
                    n = 1.0 - abs(n)
                    n = n * n
                elif args.turbulence:
                    n = abs(n)
                
                value += amplitude * n
                max_amp += amplitude
                amplitude *= args.persistence
                f *= args.lacunarity
            
            result[int(y), int(x)] = value / max_amp
    
    result = (result - result.min()) / (result.max() - result.min() + 1e-8)
    return result


def rotate_array(data: np.ndarray, angle: float) -> np.ndarray:
    """Rotate array by angle degrees."""
    try:
        from scipy.ndimage import rotate
        return rotate(data, angle, reshape=False, mode='wrap')
    except ImportError:
        # Simple rotation fallback using PIL
        from PIL import Image
        img = Image.fromarray((data * 255).astype(np.uint8))
        rotated = img.rotate(angle, resample=Image.BILINEAR, expand=False)
        return np.array(rotated).astype(np.float64) / 255.0


def generate_flipbook(args):
    """Generate all frames and compose into flipbook grid."""
    gen = PatternGenerator(seed=args.seed)
    cols, rows = parse_grid(args.grid)
    total_frames = cols * rows
    frame_size = args.resolution
    
    # Output image size
    out_width = frame_size * cols
    out_height = frame_size * rows
    
    # Create output arrays
    flipbook_data = np.zeros((out_height, out_width), dtype=np.float64)
    flipbook_alpha = np.zeros((out_height, out_width), dtype=np.float64)
    
    print(f"Generating {total_frames} frames ({cols}x{rows} grid)...")
    
    for frame in range(total_frames):
        row = frame // cols
        col = frame % cols
        
        # Generate frame
        data = generate_frame(gen, args, frame, total_frames, frame_size)
        
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
        
        # Place in grid
        y_start = row * frame_size
        x_start = col * frame_size
        flipbook_data[y_start:y_start+frame_size, x_start:x_start+frame_size] = data
        flipbook_alpha[y_start:y_start+frame_size, x_start:x_start+frame_size] = alpha
        
        print(f"  Frame {frame + 1}/{total_frames}")
    
    return flipbook_data, flipbook_alpha, cols, rows


def save_flipbook(data: np.ndarray, alpha: np.ndarray, args, cols: int, rows: int):
    """Save flipbook as PNG with alpha."""
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
        filename = f"flipbook_{args.pattern}_{args.resolution}_{cols}x{rows}.png"
    
    # Ensure output directory exists
    os.makedirs(args.output, exist_ok=True)
    filepath = os.path.join(args.output, filename)
    
    img.save(filepath, 'PNG')
    print(f"Saved: {filepath}")
    print(f"  Total size: {width}x{height}")
    print(f"  Frame size: {args.resolution}x{args.resolution}")
    print(f"  Grid: {cols}x{rows} ({cols * rows} frames)")
    return filepath


def main():
    args = parse_args()
    data, alpha, cols, rows = generate_flipbook(args)
    save_flipbook(data, alpha, args, cols, rows)


if __name__ == '__main__':
    main()
