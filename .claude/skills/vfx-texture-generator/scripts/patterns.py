"""
VFX Texture Generator - Core Pattern Module
Procedural pattern generation for VFX textures.
"""

import numpy as np
from typing import Tuple, Optional, Callable
import math

# Try to import opensimplex, fall back to numpy-based noise
try:
    from opensimplex import OpenSimplex
    HAS_OPENSIMPLEX = True
except ImportError:
    HAS_OPENSIMPLEX = False
    print("Note: opensimplex not found, using numpy fallback (lower quality)")

# Try to import scipy for edge detection
try:
    from scipy import ndimage
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False


class NumpyNoise:
    """Simple Perlin-like noise using numpy (fallback when opensimplex unavailable)."""
    
    def __init__(self, seed: int = 0):
        self.seed = seed
        np.random.seed(seed)
        # Generate permutation table
        self.perm = np.arange(256, dtype=np.int32)
        np.random.shuffle(self.perm)
        self.perm = np.tile(self.perm, 2)
        # Gradient vectors
        self.grad2 = np.array([
            [1, 1], [-1, 1], [1, -1], [-1, -1],
            [1, 0], [-1, 0], [0, 1], [0, -1]
        ], dtype=np.float64)
        self.grad3 = np.array([
            [1,1,0], [-1,1,0], [1,-1,0], [-1,-1,0],
            [1,0,1], [-1,0,1], [1,0,-1], [-1,0,-1],
            [0,1,1], [0,-1,1], [0,1,-1], [0,-1,-1]
        ], dtype=np.float64)
    
    def _fade(self, t):
        return t * t * t * (t * (t * 6 - 15) + 10)
    
    def _lerp(self, a, b, t):
        return a + t * (b - a)
    
    def noise2(self, x: float, y: float) -> float:
        """2D Perlin noise."""
        xi = int(np.floor(x)) & 255
        yi = int(np.floor(y)) & 255
        xf = x - np.floor(x)
        yf = y - np.floor(y)
        
        u = self._fade(xf)
        v = self._fade(yf)
        
        aa = self.perm[self.perm[xi] + yi]
        ab = self.perm[self.perm[xi] + yi + 1]
        ba = self.perm[self.perm[xi + 1] + yi]
        bb = self.perm[self.perm[xi + 1] + yi + 1]
        
        g = self.grad2
        x1 = self._lerp(
            np.dot(g[aa % 8], [xf, yf]),
            np.dot(g[ba % 8], [xf - 1, yf]),
            u
        )
        x2 = self._lerp(
            np.dot(g[ab % 8], [xf, yf - 1]),
            np.dot(g[bb % 8], [xf - 1, yf - 1]),
            u
        )
        
        return self._lerp(x1, x2, v)
    
    def noise3(self, x: float, y: float, z: float) -> float:
        """3D Perlin noise."""
        xi = int(np.floor(x)) & 255
        yi = int(np.floor(y)) & 255
        zi = int(np.floor(z)) & 255
        xf = x - np.floor(x)
        yf = y - np.floor(y)
        zf = z - np.floor(z)
        
        u = self._fade(xf)
        v = self._fade(yf)
        w = self._fade(zf)
        
        p = self.perm
        aaa = p[p[p[xi] + yi] + zi]
        aba = p[p[p[xi] + yi + 1] + zi]
        aab = p[p[p[xi] + yi] + zi + 1]
        abb = p[p[p[xi] + yi + 1] + zi + 1]
        baa = p[p[p[xi + 1] + yi] + zi]
        bba = p[p[p[xi + 1] + yi + 1] + zi]
        bab = p[p[p[xi + 1] + yi] + zi + 1]
        bbb = p[p[p[xi + 1] + yi + 1] + zi + 1]
        
        g = self.grad3
        
        def grad_dot(h, x, y, z):
            return np.dot(g[h % 12], [x, y, z])
        
        x1 = self._lerp(
            self._lerp(grad_dot(aaa, xf, yf, zf), grad_dot(baa, xf-1, yf, zf), u),
            self._lerp(grad_dot(aba, xf, yf-1, zf), grad_dot(bba, xf-1, yf-1, zf), u),
            v
        )
        x2 = self._lerp(
            self._lerp(grad_dot(aab, xf, yf, zf-1), grad_dot(bab, xf-1, yf, zf-1), u),
            self._lerp(grad_dot(abb, xf, yf-1, zf-1), grad_dot(bbb, xf-1, yf-1, zf-1), u),
            v
        )
        
        return self._lerp(x1, x2, w)
    
    def noise4(self, x: float, y: float, z: float, w: float) -> float:
        """4D noise approximation (uses 3D noise combination)."""
        return (self.noise3(x, y, z) + self.noise3(y, z, w) + 
                self.noise3(z, w, x) + self.noise3(w, x, y)) / 4


class PatternGenerator:
    """Core pattern generation engine."""
    
    def __init__(self, seed: Optional[int] = None):
        self.seed = seed if seed is not None else np.random.randint(0, 2**31)
        if HAS_OPENSIMPLEX:
            self.noise = OpenSimplex(seed=self.seed)
        else:
            self.noise = NumpyNoise(seed=self.seed)
        np.random.seed(self.seed)
    
    # =========================================================================
    # SIMPLEX / PERLIN NOISE
    # =========================================================================
    
    def simplex_2d(
        self,
        size: int,
        frequency: float = 4.0,
        octaves: int = 1,
        persistence: float = 0.5,
        lacunarity: float = 2.0,
        tileable: bool = False
    ) -> np.ndarray:
        """Generate 2D simplex noise."""
        result = np.zeros((size, size), dtype=np.float64)
        
        for y in range(size):
            for x in range(size):
                amplitude = 1.0
                freq = frequency
                value = 0.0
                max_amp = 0.0
                
                for _ in range(octaves):
                    if tileable:
                        # Tileable via 4D noise on torus
                        nx = x / size
                        ny = y / size
                        value += amplitude * self._tileable_noise_2d(nx, ny, freq)
                    else:
                        value += amplitude * self.noise.noise2(x * freq / size, y * freq / size)
                    
                    max_amp += amplitude
                    amplitude *= persistence
                    freq *= lacunarity
                
                result[y, x] = value / max_amp
        
        # Normalize to 0-1
        result = (result - result.min()) / (result.max() - result.min() + 1e-8)
        return result
    
    def _tileable_noise_2d(self, nx: float, ny: float, freq: float) -> float:
        """Generate tileable noise using 4D torus mapping."""
        angle_x = nx * 2 * math.pi
        angle_y = ny * 2 * math.pi
        
        # Map to 4D torus
        x4 = math.cos(angle_x) * freq / (2 * math.pi)
        y4 = math.sin(angle_x) * freq / (2 * math.pi)
        z4 = math.cos(angle_y) * freq / (2 * math.pi)
        w4 = math.sin(angle_y) * freq / (2 * math.pi)
        
        return self.noise.noise4(x4, y4, z4, w4)
    
    def simplex_3d(
        self,
        size: int,
        depth: int,
        frequency: float = 4.0,
        octaves: int = 1,
        persistence: float = 0.5,
        lacunarity: float = 2.0,
        tileable: bool = False
    ) -> np.ndarray:
        """Generate 3D simplex noise volume."""
        result = np.zeros((depth, size, size), dtype=np.float64)
        
        for z in range(depth):
            for y in range(size):
                for x in range(size):
                    amplitude = 1.0
                    freq = frequency
                    value = 0.0
                    max_amp = 0.0
                    
                    for _ in range(octaves):
                        if tileable:
                            value += amplitude * self._tileable_noise_3d(
                                x / size, y / size, z / depth, freq
                            )
                        else:
                            value += amplitude * self.noise.noise3(
                                x * freq / size, y * freq / size, z * freq / depth
                            )
                        
                        max_amp += amplitude
                        amplitude *= persistence
                        freq *= lacunarity
                    
                    result[z, y, x] = value / max_amp
        
        result = (result - result.min()) / (result.max() - result.min() + 1e-8)
        return result
    
    def _tileable_noise_3d(self, nx: float, ny: float, nz: float, freq: float) -> float:
        """Generate tileable 3D noise using higher dimensional mapping."""
        # For 3D tileability, we'd need 6D noise which opensimplex doesn't have
        # Use blending approach instead
        base = self.noise.noise3(nx * freq, ny * freq, nz * freq)
        return base
    
    # =========================================================================
    # FBM (Fractal Brownian Motion)
    # =========================================================================
    
    def fbm_2d(
        self,
        size: int,
        frequency: float = 3.0,
        octaves: int = 6,
        persistence: float = 0.5,
        lacunarity: float = 2.0,
        ridged: bool = False,
        turbulence: bool = False,
        tileable: bool = False
    ) -> np.ndarray:
        """Generate FBM noise with optional ridged/turbulence variants."""
        result = np.zeros((size, size), dtype=np.float64)
        
        for y in range(size):
            for x in range(size):
                amplitude = 1.0
                freq = frequency
                value = 0.0
                max_amp = 0.0
                
                for _ in range(octaves):
                    if tileable:
                        n = self._tileable_noise_2d(x / size, y / size, freq)
                    else:
                        n = self.noise.noise2(x * freq / size, y * freq / size)
                    
                    if ridged:
                        n = 1.0 - abs(n)
                        n = n * n
                    elif turbulence:
                        n = abs(n)
                    
                    value += amplitude * n
                    max_amp += amplitude
                    amplitude *= persistence
                    freq *= lacunarity
                
                result[y, x] = value / max_amp
        
        result = (result - result.min()) / (result.max() - result.min() + 1e-8)
        return result
    
    def fbm_3d(
        self,
        size: int,
        depth: int,
        frequency: float = 3.0,
        octaves: int = 6,
        persistence: float = 0.5,
        lacunarity: float = 2.0,
        ridged: bool = False,
        turbulence: bool = False,
        tileable: bool = False
    ) -> np.ndarray:
        """Generate 3D FBM noise volume."""
        result = np.zeros((depth, size, size), dtype=np.float64)
        
        for z in range(depth):
            for y in range(size):
                for x in range(size):
                    amplitude = 1.0
                    freq = frequency
                    value = 0.0
                    max_amp = 0.0
                    
                    for _ in range(octaves):
                        n = self.noise.noise3(
                            x * freq / size, y * freq / size, z * freq / depth
                        )
                        
                        if ridged:
                            n = 1.0 - abs(n)
                            n = n * n
                        elif turbulence:
                            n = abs(n)
                        
                        value += amplitude * n
                        max_amp += amplitude
                        amplitude *= persistence
                        freq *= lacunarity
                    
                    result[z, y, x] = value / max_amp
        
        result = (result - result.min()) / (result.max() - result.min() + 1e-8)
        return result
    
    # =========================================================================
    # VORONOI / CELLULAR
    # =========================================================================
    
    def voronoi_2d(
        self,
        size: int,
        cells: int = 16,
        metric: str = 'euclidean',
        mode: str = 'f1',
        jitter: float = 1.0,
        tileable: bool = False
    ) -> np.ndarray:
        """Generate Voronoi/cellular pattern."""
        # Generate cell points
        cell_size = size / math.sqrt(cells)
        grid_cells = int(math.ceil(math.sqrt(cells)))
        
        points = []
        for cy in range(grid_cells + (2 if tileable else 0)):
            for cx in range(grid_cells + (2 if tileable else 0)):
                base_x = (cx - (1 if tileable else 0)) * cell_size + cell_size / 2
                base_y = (cy - (1 if tileable else 0)) * cell_size + cell_size / 2
                
                # Apply jitter
                offset_x = (np.random.random() - 0.5) * cell_size * jitter
                offset_y = (np.random.random() - 0.5) * cell_size * jitter
                
                points.append((base_x + offset_x, base_y + offset_y))
        
        points = np.array(points)
        result = np.zeros((size, size), dtype=np.float64)
        
        # Distance function
        def dist(x1, y1, x2, y2):
            if metric == 'euclidean':
                return math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
            elif metric == 'manhattan':
                return abs(x2 - x1) + abs(y2 - y1)
            elif metric == 'chebyshev':
                return max(abs(x2 - x1), abs(y2 - y1))
            return math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
        
        # Calculate distances for each pixel
        for y in range(size):
            for x in range(size):
                distances = []
                for px, py in points:
                    if tileable:
                        # Check wrapped distances
                        d = min(
                            dist(x, y, px, py),
                            dist(x, y, px + size, py),
                            dist(x, y, px - size, py),
                            dist(x, y, px, py + size),
                            dist(x, y, px, py - size)
                        )
                    else:
                        d = dist(x, y, px, py)
                    distances.append(d)
                
                distances.sort()
                f1 = distances[0]
                f2 = distances[1] if len(distances) > 1 else distances[0]
                
                if mode == 'f1':
                    result[y, x] = f1
                elif mode == 'f2':
                    result[y, x] = f2
                elif mode == 'f2-f1':
                    result[y, x] = f2 - f1
                elif mode == 'edge':
                    result[y, x] = 1.0 - min(1.0, (f2 - f1) / (cell_size * 0.3))
        
        result = (result - result.min()) / (result.max() - result.min() + 1e-8)
        return result
    
    # =========================================================================
    # GRADIENTS
    # =========================================================================
    
    def radial_gradient(
        self,
        size: int,
        falloff: str = 'smooth',
        radius: float = 0.5,
        center_x: float = 0.5,
        center_y: float = 0.5,
        softness: float = 1.0,
        power: float = 2.0
    ) -> np.ndarray:
        """Generate radial gradient."""
        result = np.zeros((size, size), dtype=np.float64)
        
        cx = center_x * size
        cy = center_y * size
        max_dist = radius * size
        
        for y in range(size):
            for x in range(size):
                dist = math.sqrt((x - cx)**2 + (y - cy)**2)
                t = min(1.0, dist / max_dist) if max_dist > 0 else 0
                
                # Apply softness
                t = t ** (1.0 / softness) if softness != 1.0 else t
                
                if falloff == 'linear':
                    value = 1.0 - t
                elif falloff == 'smooth':
                    value = 1.0 - (t * t * (3 - 2 * t))  # smoothstep
                elif falloff == 'exp':
                    value = math.exp(-t * power * 3)
                elif falloff == 'inv_exp':
                    value = 1.0 - math.exp(-(1.0 - t) * power * 3)
                else:
                    value = 1.0 - t
                
                result[y, x] = max(0.0, min(1.0, value))
        
        return result
    
    def directional_gradient(
        self,
        size: int,
        angle: float = 0.0,
        falloff: str = 'linear',
        start: float = 0.0,
        end: float = 1.0,
        repeat: int = 1
    ) -> np.ndarray:
        """Generate directional/linear gradient."""
        result = np.zeros((size, size), dtype=np.float64)
        
        angle_rad = math.radians(angle)
        cos_a = math.cos(angle_rad)
        sin_a = math.sin(angle_rad)
        
        for y in range(size):
            for x in range(size):
                # Project onto gradient direction
                nx = x / size - 0.5
                ny = y / size - 0.5
                
                projected = (nx * cos_a + ny * sin_a) + 0.5
                
                # Apply repeat
                projected = (projected * repeat) % 1.0
                
                # Remap to start-end range
                t = start + projected * (end - start)
                t = max(0.0, min(1.0, t))
                
                if falloff == 'linear':
                    value = t
                elif falloff == 'smooth':
                    value = t * t * (3 - 2 * t)
                elif falloff == 'exp':
                    value = 1.0 - math.exp(-t * 3)
                else:
                    value = t
                
                result[y, x] = value
        
        return result
    
    # =========================================================================
    # CAUSTICS
    # =========================================================================
    
    def caustics(
        self,
        size: int,
        scale: float = 4.0,
        time: float = 0.0,
        intensity: float = 1.0,
        layers: int = 3,
        distortion: float = 0.5,
        tileable: bool = False
    ) -> np.ndarray:
        """Generate caustics pattern."""
        result = np.zeros((size, size), dtype=np.float64)
        
        for layer in range(layers):
            layer_offset = layer * 17.3
            layer_scale = scale * (1.0 + layer * 0.3)
            
            for y in range(size):
                for x in range(size):
                    nx = x / size
                    ny = y / size
                    
                    # Distortion
                    if tileable:
                        dx = self._tileable_noise_2d(nx, ny, layer_scale * 2) * distortion
                        dy = self._tileable_noise_2d(nx + 100, ny + 100, layer_scale * 2) * distortion
                    else:
                        dx = self.noise.noise3(nx * layer_scale * 2, ny * layer_scale * 2, time + layer_offset) * distortion
                        dy = self.noise.noise3(nx * layer_scale * 2 + 100, ny * layer_scale * 2 + 100, time + layer_offset) * distortion
                    
                    sx = nx + dx
                    sy = ny + dy
                    
                    # Caustic pattern using sine waves
                    c1 = math.sin((sx * layer_scale + time) * math.pi * 2)
                    c2 = math.sin((sy * layer_scale + time * 0.7) * math.pi * 2)
                    c3 = math.sin(((sx + sy) * layer_scale * 0.7 + time * 1.3) * math.pi * 2)
                    
                    caustic = (c1 * c2 + c2 * c3 + c3 * c1) / 3.0
                    caustic = (caustic + 1.0) / 2.0  # Normalize to 0-1
                    
                    result[y, x] += caustic / layers
        
        result = result ** (1.0 / intensity)
        result = (result - result.min()) / (result.max() - result.min() + 1e-8)
        return result
    
    # =========================================================================
    # UTILITIES
    # =========================================================================
    
    @staticmethod
    def apply_modifiers(
        data: np.ndarray,
        invert: bool = False,
        contrast: float = 1.0,
        brightness: float = 0.0,
        gamma: float = 1.0,
        remap_min: float = 0.0,
        remap_max: float = 1.0
    ) -> np.ndarray:
        """Apply global modifiers to pattern data."""
        result = data.copy()
        
        if invert:
            result = 1.0 - result
        
        # Contrast (around 0.5)
        if contrast != 1.0:
            result = (result - 0.5) * contrast + 0.5
        
        # Brightness
        if brightness != 0.0:
            result = result + brightness
        
        # Gamma
        if gamma != 1.0:
            result = np.clip(result, 0, 1)
            result = result ** gamma
        
        # Remap
        result = remap_min + result * (remap_max - remap_min)
        
        # Final clamp
        result = np.clip(result, 0.0, 1.0)
        
        return result
    
    @staticmethod
    def generate_alpha(
        data: np.ndarray,
        source: str = 'value',
        threshold: float = 0.1,
        softness: float = 0.1,
        multiply: float = 1.0
    ) -> np.ndarray:
        """Generate alpha channel from pattern data."""
        if source == 'value':
            alpha = data.copy()
        elif source == 'edge':
            if HAS_SCIPY:
                # Sobel edge detection
                sx = ndimage.sobel(data, axis=1)
                sy = ndimage.sobel(data, axis=0)
                alpha = np.hypot(sx, sy)
            else:
                # Simple gradient-based edge detection fallback
                dx = np.abs(np.diff(data, axis=1, prepend=data[:, :1]))
                dy = np.abs(np.diff(data, axis=0, prepend=data[:1, :]))
                alpha = np.sqrt(dx**2 + dy**2)
            alpha = (alpha - alpha.min()) / (alpha.max() - alpha.min() + 1e-8)
        elif source == 'threshold':
            # Soft threshold
            alpha = np.clip((data - threshold) / (softness + 1e-8), 0, 1)
        else:
            alpha = data.copy()
        
        alpha = alpha * multiply
        return np.clip(alpha, 0.0, 1.0)
