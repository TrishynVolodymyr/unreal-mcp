---
name: vfx-texture-generator
description: >
  Procedural VFX texture and sprite generator for Unreal Engine 5.
  Creates grayscale + alpha textures optimized for Niagara particle systems and materials.
  Asset types: single sprites, animated flipbooks, tileable noise, volume textures (tiled slices), gradient ramps.
  Pattern types: Perlin/Simplex noise, FBM, Voronoi/cellular, radial gradients, directional gradients, caustics.
  ALWAYS trigger on: VFX texture, particle sprite, flipbook generation, noise texture, volume texture,
  tileable noise, procedural texture, soft particle, smoke texture, fire texture, magic texture,
  distortion texture, gradient ramp, caustics texture, cellular pattern,
  or phrases like "generate sprite", "create flipbook", "make noise texture", "procedural VFX asset".
---

# VFX Texture Generator

Procedural generation of grayscale + alpha textures for Unreal Engine VFX pipelines.

## Core Principles

1. **Grayscale + Alpha** — Color applied in-engine via curves/parameters
2. **Flexible resolution** — Any size supported; power-of-two (256, 512, 1024, 2048) recommended for UE5
3. **PNG with alpha** — Premultiplied alpha, sRGB for sprites, Linear for data textures
4. **UE5-ready** — Naming and structure for direct import

---

## Asset Types

| Type | Description | Use Case |
|------|-------------|----------|
| **Single Sprite** | One image with alpha | Smoke puffs, sparks, soft particles |
| **Flipbook** | Animated frames in grid | Evolving smoke, fire, magic effects |
| **Tileable Noise** | Seamless repeating pattern | Distortion, material effects |
| **Volume Texture** | 3D slices in 2D grid | Volumetric fog, clouds, 3D noise fields |
| **Gradient Ramp** | 1D or 2D gradient | Color over life, falloff curves |

---

## Pattern Types

| Pattern | Description | Best For |
|---------|-------------|----------|
| **Perlin/Simplex** | Smooth organic noise | Smoke, clouds, soft effects |
| **FBM** | Layered fractal noise | Detailed turbulence, fire |
| **Voronoi** | Cellular patterns | Cracks, scales, energy fields |
| **Radial Gradient** | Center-to-edge falloff | Soft particles, glows |
| **Directional Gradient** | Linear falloff | Ramps, masks |
| **Caustics** | Light refraction patterns | Water, magic, energy |

---

## Workflow

### Step 1: Define Requirements

Clarify before generating:
- **Asset type**: Sprite, flipbook, noise, volume, ramp?
- **Pattern**: Which procedural pattern?
- **Resolution**: 256, 512, 1024, 2048?
- **For flipbooks**: Grid size (4x4, 8x8), frame count
- **For volumes**: Slice count (8, 16, 32)
- **Tileability**: Required for noise/volumes?

### Step 2: Configure Parameters

Each pattern has specific parameters. See `references/pattern-params.md` for complete reference.

### Step 3: Generate

Run appropriate script from `scripts/`:
```bash
python3 scripts/generate_sprite.py --pattern simplex --resolution 512 --output ./output/
python3 scripts/generate_flipbook.py --pattern fbm --resolution 512 --grid 8x8 --output ./output/
python3 scripts/generate_noise.py --pattern voronoi --resolution 512 --tileable --output ./output/
python3 scripts/generate_volume.py --pattern simplex --resolution 256 --slices 16 --output ./output/
python3 scripts/generate_ramp.py --type radial --resolution 256 --output ./output/
```

### Step 4: Verify Output

- Check alpha channel presence
- Verify tileability if required
- Confirm resolution matches request

---

## Output Structure

```
project-root/
└── vfx-texture-generator/
    └── {domain}/
        └── {type}_{pattern}_{resolution}[_{extra}].png
```

**Naming examples:**
- `sprite_simplex_512.png`
- `flipbook_fbm_512_8x8.png`
- `noise_voronoi_512_tileable.png`
- `volume_simplex_256_16slices.png`
- `ramp_radial_256.png`

---

## UE5 Import Settings

After generation, import to UE5 with these settings:

| Asset Type | Compression | sRGB | LOD Group |
|------------|-------------|------|-----------|
| Sprite/Flipbook | BC7 (DX11) | ON | UI |
| Noise (distortion) | BC4 | OFF | World |
| Volume Texture | VolumeTexture | OFF | World |
| Gradient Ramp | BC7 | ON | UI |

For flipbooks, set **Texture Group** and configure **SubUV** in Niagara.

---

## Script Reference

All scripts in `scripts/` folder. Common parameters:

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--pattern` | Pattern type | simplex |
| `--resolution` | Output size | 512 |
| `--output` | Output directory | ./ |
| `--seed` | Random seed | random |
| `--name` | Custom filename | auto-generated |

### Pattern-Specific Parameters

See `references/pattern-params.md` for:
- Noise octaves, frequency, lacunarity
- Voronoi cell count, distance metric
- FBM layers and persistence
- Caustics scale and intensity
- Gradient curve controls

### Flipbook-Specific Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--grid` | Grid dimensions (e.g., 8x8) | 4x4 |
| `--animation` | Animation type: evolve, scale, rotate | evolve |
| `--speed` | Animation speed multiplier | 1.0 |

### Flipbook Radial Alpha (Soft Circular Edges)

For particle flipbooks that need soft circular edges per frame (fire, smoke, magic), use radial alpha:

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--alpha_source` | Set to `radial` for per-frame circular mask | value |
| `--radial_radius` | Mask radius relative to frame (0-0.5) | 0.45 |
| `--radial_softness` | Edge falloff width | 0.2 |
| `--radial_power` | Falloff curve exponent (higher = sharper) | 1.5 |
| `--radial_combine` | How to combine with pattern: `value`, `multiply`, `max` | multiply |

**Combine modes:**
- `value` — Radial mask only (ignores pattern values)
- `multiply` — Radial mask × pattern value (recommended for fire/smoke)
- `max` — Max of radial and pattern (for glow effects)

**Example - Fire flipbook with soft edges:**
```bash
python3 scripts/generate_flipbook.py --pattern fbm --turbulence --grid 4x2 --resolution 256 \
    --alpha_source radial --radial_radius 0.42 --radial_softness 0.2 --radial_combine multiply
```

**Note:** RGB is premultiplied by alpha, so pixels outside the mask are black (clean edges in image editors).

### Volume-Specific Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--slices` | Z-depth slice count | 16 |
| `--tileable` | Make tileable in XYZ | false |

---

## Animation Types (Flipbooks)

| Type | Effect | Use Case |
|------|--------|----------|
| **evolve** | Noise offset shifts each frame | Drifting smoke, flickering fire |
| **scale** | Pattern zooms over frames | Energy buildup, impacts |
| **rotate** | Pattern rotates over frames | Vortex, swirl effects |

Can be combined: `--animation evolve+rotate`

---

## Technical Notes

### Alpha Generation

Alpha derived from pattern values:
- **Noise patterns**: Direct value mapping (0-1)
- **Radial**: Distance-based falloff with curve control
- **Voronoi**: Cell edge detection for alpha

### Tileability

For tileable textures, patterns wrap at edges using:
- Periodic noise functions
- Toroidal mapping for gradients
- Edge blending for complex patterns

### Volume Slice Layout

Volume textures stored as 2D grid of slices:
```
For 16 slices at 256px each → 1024x1024 output (4x4 grid)
For 32 slices at 256px each → 2048x1024 output (8x4 grid)
```

Slice arrangement: left-to-right, top-to-bottom, Z increasing.

---

## Dependencies

Scripts require:
```bash
pip install pillow numpy opensimplex scipy
```

---

## Limitations

- **Procedural only** — No simulation-based smoke/fire
- **Grayscale output** — Color applied in-engine
- **PNG format** — EXR/HDR not supported
- For high-fidelity realistic flipbooks, use EmberGen/Houdini

---

## Quick Examples

**Soft particle sprite:**
```bash
python3 scripts/generate_sprite.py --pattern radial --resolution 256
```

**Evolving smoke flipbook 8x8:**
```bash
python3 scripts/generate_flipbook.py --pattern fbm --resolution 512 --grid 8x8 --animation evolve
```

**Fire flipbook with soft circular edges (4x2 grid):**
```bash
python3 scripts/generate_flipbook.py --pattern fbm --turbulence --grid 4x2 --resolution 256 \
    --alpha_source radial --radial_radius 0.42 --radial_softness 0.2 --radial_combine multiply
```

**Tileable distortion noise:**
```bash
python3 scripts/generate_noise.py --pattern simplex --resolution 512 --tileable --octaves 4
```

**Volume fog texture:**
```bash
python3 scripts/generate_volume.py --pattern fbm --resolution 256 --slices 16 --tileable
```
