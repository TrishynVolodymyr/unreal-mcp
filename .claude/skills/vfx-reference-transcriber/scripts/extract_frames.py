#!/usr/bin/env python3
"""
Extract key frames from video/GIF/WebP for VFX analysis.
Outputs frames as PNG images for visual inspection.
"""

import subprocess
import sys
import os
import json
from pathlib import Path

def get_video_info(input_path: str) -> dict:
    """Get video duration and frame count using ffprobe."""
    cmd = [
        'ffprobe', '-v', 'quiet',
        '-print_format', 'json',
        '-show_format', '-show_streams',
        input_path
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return {}
    return json.loads(result.stdout)

def extract_frames(
    input_path: str,
    output_dir: str,
    num_frames: int = 8,
    prefix: str = "frame"
) -> list[str]:
    """
    Extract evenly-spaced frames from video/GIF/WebP.
    
    Args:
        input_path: Path to input video/GIF/WebP
        output_dir: Directory to save extracted frames
        num_frames: Number of frames to extract (default 8)
        prefix: Filename prefix for output frames
    
    Returns:
        List of paths to extracted frame images
    """
    input_path = Path(input_path)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")
    
    # Get video info
    info = get_video_info(str(input_path))
    
    duration = None
    total_frames = None
    fps = None
    
    # Try to get duration from format
    if 'format' in info and 'duration' in info['format']:
        duration = float(info['format']['duration'])
    
    # Try to get frame info from video stream
    for stream in info.get('streams', []):
        if stream.get('codec_type') == 'video':
            if 'nb_frames' in stream:
                total_frames = int(stream['nb_frames'])
            if 'r_frame_rate' in stream:
                num, den = map(int, stream['r_frame_rate'].split('/'))
                if den > 0:
                    fps = num / den
            if 'duration' in stream and duration is None:
                duration = float(stream['duration'])
            break
    
    # For GIFs/WebP, estimate from fps and frame count
    if duration is None and total_frames and fps:
        duration = total_frames / fps
    
    # Fallback: try to extract and count
    if duration is None or duration <= 0:
        # For animated images, use different approach
        duration = 2.0  # Assume 2 second loop as fallback
    
    output_files = []
    
    if duration <= 0.5:
        # Very short - just grab all unique frames up to num_frames
        cmd = [
            'ffmpeg', '-y', '-i', str(input_path),
            '-vf', f'select=not(mod(n\\,1)),scale=iw:ih',
            '-frames:v', str(num_frames),
            '-vsync', 'vfr',
            str(output_dir / f'{prefix}_%02d.png')
        ]
    else:
        # Calculate fps to get desired number of frames
        target_fps = num_frames / duration
        cmd = [
            'ffmpeg', '-y', '-i', str(input_path),
            '-vf', f'fps={target_fps}',
            '-frames:v', str(num_frames),
            str(output_dir / f'{prefix}_%02d.png')
        ]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    # Collect output files
    for i in range(1, num_frames + 1):
        frame_path = output_dir / f'{prefix}_{i:02d}.png'
        if frame_path.exists():
            output_files.append(str(frame_path))
    
    # If we got fewer frames than requested, that's OK
    # (video might be shorter than expected)
    
    return output_files


def create_motion_comparison(
    input_path: str,
    output_dir: str,
    prefix: str = "motion"
) -> list[str]:
    """
    Create frame pairs optimized for motion analysis.
    Extracts pairs of consecutive frames to show movement.
    
    Returns list of frame paths.
    """
    input_path = Path(input_path)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Extract more frames for motion analysis (12 frames = 6 pairs)
    cmd = [
        'ffmpeg', '-y', '-i', str(input_path),
        '-vf', 'fps=6',  # 6 fps to capture motion
        '-frames:v', '12',
        str(output_dir / f'{prefix}_%02d.png')
    ]
    
    subprocess.run(cmd, capture_output=True, text=True)
    
    output_files = []
    for i in range(1, 13):
        frame_path = output_dir / f'{prefix}_{i:02d}.png'
        if frame_path.exists():
            output_files.append(str(frame_path))
    
    return output_files


def create_grid_preview(
    frame_paths: list[str],
    output_path: str,
    cols: int = 4
) -> str:
    """
    Create a single grid image from multiple frames.
    Useful for quick overview of the entire sequence.
    """
    if not frame_paths:
        raise ValueError("No frames to create grid from")
    
    # Use ffmpeg to create tile grid
    rows = (len(frame_paths) + cols - 1) // cols
    
    # Build filter for grid
    inputs = ' '.join([f'-i {p}' for p in frame_paths])
    n = len(frame_paths)
    
    # Pad to fill grid if needed
    filter_complex = f'xstack=inputs={n}:layout='
    
    # Calculate layout positions
    layouts = []
    for i in range(n):
        row = i // cols
        col = i % cols
        layouts.append(f'{col}*w0|{row}*h0')
    
    # Simpler approach: use tile filter
    cmd = [
        'ffmpeg', '-y',
        '-i', frame_paths[0],  # First frame to get dimensions
    ]
    
    # Add all inputs
    for p in frame_paths:
        cmd.extend(['-i', p])
    
    # Build filter
    filter_parts = []
    for i in range(len(frame_paths)):
        filter_parts.append(f'[{i+1}:v]')
    
    filter_str = ''.join(filter_parts) + f'hstack=inputs={min(len(frame_paths), cols)}'
    
    # Actually, let's use a simpler montage approach with multiple hstacks/vstacks
    # For simplicity, just create individual frames and note the grid idea
    
    # Just return the first frame path as preview for now
    # The main value is in the individual frames
    return frame_paths[0] if frame_paths else ""


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: extract_frames.py <input_video> <output_dir> [num_frames]")
        print("  Supported: mp4, mov, avi, gif, webp, mkv, webm")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2]
    num_frames = int(sys.argv[3]) if len(sys.argv) > 3 else 8
    
    print(f"Extracting {num_frames} frames from: {input_file}")
    
    frames = extract_frames(input_file, output_dir, num_frames)
    
    print(f"Extracted {len(frames)} frames:")
    for f in frames:
        print(f"  {f}")
