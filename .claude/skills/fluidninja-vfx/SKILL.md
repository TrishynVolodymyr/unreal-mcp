---
name: fluidninja-vfx
description: "FluidNinja VFX Tools workflow guide for Unreal Engine fluid simulation baking. Use when creating fire, smoke, explosions, clouds, fog, water, magic effects, environmental particles, flipbook generation, or any VFX requiring fluid dynamics. Covers NinjaTools (baking to flipbooks), Niagara GPU particle integration, Ray Marching self-shadowing, Parallax Occlusion Mapping depth, and UE5 Volumetrics. Triggers on FluidNinja, fluid simulation, bake flipbook, velocity map, flowmap, volumetric fog driven by simulation, Niagara texture sampling, VFX flipbook workflow."
---

# FluidNinja VFX Tools

FluidNinja NinjaTools is a 2D fluid simulation baking toolkit for Unreal Engine producing small baked assets (1-4 MB vs 100-400 MB VDBs).

> **Warning**: "Ninja is not a Drag-n-drop Tool, learning it takes a week." - Developer Andras Ketzer

## Products

| Product | Price | Use Case |
|---------|-------|----------|
| NinjaTools | $99 (free student) | Bake to flipbooks. Zero runtime cost. |
| NinjaLive | $150 (free student) | Real-time simulation. Tracks MetaHuman bones. |
| NinjaPlay | Free | Standalone flipbook player |

## Documentation

- **Manual PDF**: `references/FluidNinjaManual.pdf` (local copy)
- **Niagara PDF**: https://drive.google.com/file/d/19FAnIfwJHfMHNUnkEApoVu18LHgPjVN4
- **Videos**: https://www.youtube.com/playlist?list=PLVCUepYV6TvPnVELcyGoSDhpoLO7PyT6_
- **Discord**: FluidNinja Community Server (free student version)
- **Forum**: https://forums.unrealengine.com/t/fluidninja-vfx-tools/133339

## Core Workflow

1. Open FluidNinja level, Play in Selected Viewport or New Editor Window
2. Select/duplicate preset from dropdown
3. Configure input (Particles, Bitmaps, or NinjaPaint)
4. Adjust simulation parameters
5. Set frame count & resolution in Canvas/Bake
6. Press Record -> Outputs: Density flipbook, Velocity flipbook, Player Material

---

## GUI Parameters (Actual)

### Canvas Section
| Parameter | Description |
|-----------|-------------|
| **Size** | Canvas resolution (width x height), e.g., 512x256 |
| **Offset X/Y** | Pan the simulation area |
| **Scale** | Zoom level of the canvas |
| **Rotation** | Rotate the canvas |
| **Tiling** | Enable tiling/wrapping for seamless loops |

### Bake Section
| Parameter | Description |
|-----------|-------------|
| **Play/Pause/Record** | Control simulation and start baking |
| **FrameRange** | Number of frames to record (e.g., 16, 24, 32) |
| **Prefix** | Name prefix for saved assets + color picker |
| **Rec. every N frame** | Skip frames (2-5 ideal for smooth, 12+ for stop-motion) |
| **Loopback** | Crossfade N ending frames to start for smooth loop |
| **Rec. mode: atlas** | Save as single atlas/flipbook image |
| **Rec. mode: sequence** | Save as image sequence (for After Effects, GIFs) |
| **masked** | Mask velocity output with density |

### Velocity Section
| Parameter | Description |
|-----------|-------------|
| **Offset X** | Horizontal velocity field offset (uniform direction) |
| **Offset Y** | Vertical velocity field offset |
| **Rotation** | Rotate velocity field (creates circular flow) |
| **Amplify** | Field strength multiplier (negative inverts direction) |
| **Turbulence** | Amount of swirling/vortex motion |
| **Noise** | Masks strength of input velocity field |

### Density Section
| Parameter | Description |
|-----------|-------------|
| **Input Weight** | Virtual density of input (higher = more pressure, faster flow). Set to 1.0 for persistent patterns |
| **Out Weight** | Speed of dissolve/dissipation (density particle "lifetime") |
| **Shading** | Fakes directional light for depth on smokes/flames |
| **Contrast** | Cartoony look adjustment |
| **Hue** | Colorize density (preview only - colorize in materials instead) |
| **Iterations** | Lower values improve performance at extreme resolutions |
| **Sharpen** | Edge sharpening (sub-zero values cause blur!) |
| **Shrp.Size** | Frequency domain for sharpening |

### Input Section
| Parameter | Description |
|-----------|-------------|
| **Density from Particles** | Select Niagara/Cascade system as density source |
| **Scale** | Scale of particle input |
| **Offset X/Y** | Position particle emitter in simulation area |
| **Density from Bitmap** | Use texture as density source |
| **Brush (Size, Strength, Hardness, Type)** | NinjaPaint interactive brush settings |
| **Save painted map** | Save brush painting as reusable bitmap |
| **Velocity from Bitmap** | Use texture for velocity input |
| **Velocity from mouse cursor Weight** | Inject brush velocity into sim |

### Export FGA Section
| Parameter | Description |
|-----------|-------------|
| **Samples X/Y** | Spatial resolution of vector field (32-128 ideal) |
| **Samples Z** | Depth/temporal layers (use NinjaFields for multi-frame) |
| **Scale** | World unit scale of exported field |
| **Source** | Toggle between velocity input vs output |

---

## Effect Recipes

### Fire/Flame
- **Amplify**: 10-40 (high for active flames)
- **Turbulence**: 0.4-1.1 (creates flickering edges)
- **Input Weight**: 0.1-0.3 (prevents over-density)
- **Out Weight**: 0.7-0.9 (quick dissipation)
- **Shading**: 0.3-0.6 (depth on flame edges)
- **Frames**: 16-32
- **Rec. every**: 3-7 frames

### Smoke
- **Amplify**: 5-20 (lower than fire)
- **Turbulence**: 0.2-0.5 (subtle swirling)
- **Input Weight**: 0.1-0.2
- **Out Weight**: 0.5-0.8 (slower fade)
- **Sharpen**: -0.1 to -0.3 (blur for softness)
- **Frames**: 32-64
- **Rec. every**: 5-10 frames

### Explosion
- **Amplify**: 30-60 (high initial burst)
- **Turbulence**: 0.5-0.8
- **Input Weight**: 0.3-0.5 (strong initial density)
- **Out Weight**: 0.6-0.9 (fast dissipation)
- **Frames**: 8-24 (non-looping)
- **Loopback**: 0 (one-shot effect)
- **Rec. every**: 1-3 frames (capture fast motion)

---

## Output Integration Options

1. **Direct Flipbook**: NinjaPlay materials for simple playback
2. **Niagara GPU Particles**: Sample baked data to drive particles (see `references/niagara.md`)
3. **Volumetrics**: Drive Fog/Cloud/Heterogeneous volumes UE5+ (see `references/volumetrics.md`)
4. **Ray Marching + POM**: Real-time self-shadowing and depth (see `references/raymarching.md`)

---

## Niagara Integration (Quick)

Requirements:
- Emitter Sim Target: **GPU** (mandatory)
- Local Space: **ON**

Key steps:
1. Add Sample Texture module
2. Configure UV: `UV = (ParticlePos * Scale) + Offset`
3. Transform velocity: `WorldVel = (SampledRGB - 0.5) * VelocityScale`
4. Route OUTPUT SampledColor to particle parameters
5. Add solver module at stack bottom

For detailed Niagara workflow, see `references/niagara.md`.

---

## Performance Reference

**Memory**: 1-4 MB per effect (vs 100-400 MB VDB)

**Material IPP** (instructions per pixel):
- Simple playback: 36-64 IPP
- Velocity interpolation: 92 IPP
- POM or RM alone: ~148 IPP
- POM + RM combined: ~233 IPP

**Runtime**: Zero simulation cost for baked flipbooks.

---

## Folder Structure

```
/Game/FluidNinja/
├── Input/FluidPresets/   # Simulation presets (DataTables)
├── BakedData/            # Output flipbooks & materials
├── Core/                 # Base materials, functions
│   ├── AtlasPlayer/      # NinjaPlay materials
│   └── FlowPlayer/       # FlowPlay materials
└── Usecases/             # Demo levels (100+ examples)
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Particles not visible | Set Engine Scalability to "EPIC" |
| Texture not sampling in Niagara | Set Sim Target to GPU |
| No particle movement | Add solver module |
| Flipbook not animating | Connect Engine Time to Phase |
| Wrong velocity direction | Negate Y scale, check coordinate space |
| Tiling artifacts | Use single-row flipbook (modify DT_AtlasMatrices) |
| **Straight pipe/bar trail** | Increase **Input Scale** (e.g., 30 → 100+) to zoom past particle emission origin |
| Trail too uniform/laminar | Increase Turbulence (1.0+), add Curl (0.3-0.5), reduce Velocity Offset X |

---

## Recording Non-Looped Sequences (Explosions)

1. Set particle input to repeat the non-looped burst
2. Stop simulation when cycle ends (before new starts)
3. Set up parameters, press Record
4. Baking won't start until you press Play
5. Set Loopback to 0

---

## References

- `references/FluidNinjaManual.pdf` - Complete official manual (26 pages)
- `references/niagara.md` - Detailed Niagara integration workflow
- `references/raymarching.md` - Ray Marching and Parallax setup
- `references/cheatsheet.md` - Quick parameter reference
