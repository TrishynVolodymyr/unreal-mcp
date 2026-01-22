# Pattern Parameters Reference

Complete parameter reference for all procedural patterns.

---

## Simplex/Perlin Noise

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--frequency` | float | 4.0 | 0.5-32 | Base frequency (higher = more detail) |
| `--octaves` | int | 1 | 1-8 | Noise layers (more = finer detail) |
| `--persistence` | float | 0.5 | 0.1-1.0 | Amplitude decay per octave |
| `--lacunarity` | float | 2.0 | 1.5-4.0 | Frequency multiplier per octave |
| `--contrast` | float | 1.0 | 0.5-3.0 | Output contrast adjustment |
| `--brightness` | float | 0.0 | -0.5-0.5 | Output brightness offset |

**Typical presets:**
- Soft clouds: `--frequency 2 --octaves 3 --persistence 0.6`
- Sharp smoke: `--frequency 6 --octaves 5 --persistence 0.4`
- Subtle distortion: `--frequency 8 --octaves 2 --persistence 0.5`

---

## FBM (Fractal Brownian Motion)

Extends simplex with better layering control.

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--frequency` | float | 3.0 | 0.5-16 | Base frequency |
| `--octaves` | int | 6 | 1-10 | Fractal layers |
| `--persistence` | float | 0.5 | 0.1-0.9 | Amplitude falloff |
| `--lacunarity` | float | 2.0 | 1.5-4.0 | Frequency growth |
| `--ridged` | bool | false | — | Use ridged noise variant |
| `--turbulence` | bool | false | — | Use absolute value (turbulent) |

**Typical presets:**
- Billowing smoke: `--frequency 3 --octaves 6 --persistence 0.5`
- Fire turbulence: `--frequency 4 --octaves 5 --turbulence`
- Ridged magic: `--frequency 2 --octaves 4 --ridged`

---

## Voronoi / Cellular

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--cells` | int | 16 | 4-64 | Number of cells (approx) |
| `--metric` | str | euclidean | euclidean, manhattan, chebyshev | Distance metric |
| `--mode` | str | f1 | f1, f2, f2-f1, edge | Output mode |
| `--jitter` | float | 1.0 | 0.0-1.0 | Cell point randomization |
| `--invert` | bool | false | — | Invert output values |

**Output modes:**
- `f1`: Distance to nearest cell (organic)
- `f2`: Distance to second-nearest (cells with borders)
- `f2-f1`: Difference (clean cell edges)
- `edge`: Edge detection only

**Typical presets:**
- Organic cells: `--cells 12 --mode f1 --jitter 0.9`
- Sharp cracks: `--cells 20 --mode f2-f1 --jitter 1.0`
- Hex-like: `--cells 16 --mode f1 --jitter 0.3`

---

## Radial Gradient

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--falloff` | str | smooth | linear, smooth, exp, inv_exp | Falloff curve |
| `--radius` | float | 0.5 | 0.1-1.0 | Gradient radius (0.5 = half image) |
| `--center_x` | float | 0.5 | 0.0-1.0 | Center X position |
| `--center_y` | float | 0.5 | 0.0-1.0 | Center Y position |
| `--softness` | float | 1.0 | 0.1-5.0 | Edge softness multiplier |
| `--power` | float | 2.0 | 0.5-5.0 | Curve power (for exp/inv_exp) |

**Falloff types:**
- `linear`: Straight line falloff
- `smooth`: Smoothstep (default, nice soft edges)
- `exp`: Exponential (bright center, fast falloff)
- `inv_exp`: Inverse exponential (soft center, hard edge)

**Typical presets:**
- Soft particle: `--falloff smooth --radius 0.45 --softness 1.2`
- Point light: `--falloff exp --radius 0.3 --power 3`
- Glow halo: `--falloff inv_exp --radius 0.6`

---

## Directional Gradient

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--angle` | float | 0 | 0-360 | Gradient angle (0 = left-to-right) |
| `--falloff` | str | linear | linear, smooth, exp | Falloff curve |
| `--start` | float | 0.0 | 0.0-1.0 | Gradient start position |
| `--end` | float | 1.0 | 0.0-1.0 | Gradient end position |
| `--repeat` | int | 1 | 1-8 | Number of gradient repeats |

**Typical presets:**
- Horizontal ramp: `--angle 0 --falloff linear`
- Vertical soft: `--angle 90 --falloff smooth`
- Diagonal stripe: `--angle 45 --repeat 4`

---

## Caustics

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--scale` | float | 4.0 | 1-16 | Pattern scale |
| `--time` | float | 0.0 | 0-10 | Animation time offset |
| `--intensity` | float | 1.0 | 0.5-3.0 | Brightness intensity |
| `--layers` | int | 3 | 1-5 | Number of overlapping layers |
| `--distortion` | float | 0.5 | 0.0-2.0 | Refraction distortion amount |

**Typical presets:**
- Underwater: `--scale 6 --layers 3 --distortion 0.8`
- Magic pool: `--scale 3 --layers 4 --intensity 1.5`
- Subtle shimmer: `--scale 8 --layers 2 --distortion 0.3`

---

## Animation Parameters (Flipbooks)

### Evolve Animation

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--evolve_speed` | float | 1.0 | 0.1-5.0 | Z-offset change per frame |
| `--evolve_direction` | str | z | x, y, z, random | Offset direction |

### Scale Animation

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--scale_start` | float | 1.0 | 0.1-5.0 | Starting scale |
| `--scale_end` | float | 2.0 | 0.1-5.0 | Ending scale |
| `--scale_curve` | str | linear | linear, ease_in, ease_out, ease_both | Scale curve |

### Rotate Animation

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `--rotate_amount` | float | 360 | 0-720 | Total rotation in degrees |
| `--rotate_direction` | str | cw | cw, ccw | Rotation direction |

---

## Global Modifiers

Available for all patterns:

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `--invert` | bool | false | Invert output values |
| `--contrast` | float | 1.0 | Contrast adjustment |
| `--brightness` | float | 0.0 | Brightness offset |
| `--gamma` | float | 1.0 | Gamma correction |
| `--clamp` | bool | true | Clamp output to 0-1 |
| `--remap_min` | float | 0.0 | Remap minimum value |
| `--remap_max` | float | 1.0 | Remap maximum value |

---

## Alpha Control

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `--alpha_source` | str | value | value, edge, threshold |
| `--alpha_threshold` | float | 0.1 | Threshold for alpha cutoff |
| `--alpha_softness` | float | 0.1 | Threshold edge softness |
| `--alpha_multiply` | float | 1.0 | Alpha multiplier |

**Alpha sources:**
- `value`: Alpha = grayscale value (default)
- `edge`: Alpha from edge detection
- `threshold`: Binary alpha with soft edge
