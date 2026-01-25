# FluidNinja Quick Reference Cheat Sheet

## Core Baking Workflow
```
1. Open FluidNinja level, Play in Selected Viewport
2. Select preset (or duplicate with double-floppy icon)
3. Configure input (Particles/Bitmaps/NinjaPaint)
4. Tweak Velocity & Density parameters
5. Set Canvas Size & FrameRange
6. Press Record → Density + Velocity flipbooks + Player Material
```

## Actual GUI Parameters

### Velocity Section
| Param | Range | Effect |
|-------|-------|--------|
| Offset X/Y | -1 to 1 | Uniform flow direction |
| Rotation | 0-360 | Circular flow |
| Amplify | 0-100+ | Field strength (negative=invert) |
| Turbulence | 0-2 | Swirling/vortex amount |
| Noise | 0-1 | Masks velocity field strength |

### Density Section
| Param | Range | Effect |
|-------|-------|--------|
| Input Weight | 0-1 | Density pressure (1=persistent) |
| Out Weight | 0-1 | Dissipation speed |
| Shading | 0-1 | Fake directional light depth |
| Contrast | -1 to 1 | Cartoony adjustment |
| Hue | 0-1 | Preview colorize (use materials!) |
| Iterations | 1-10 | Lower=faster at high res |
| Sharpen | -1 to 1 | Negative=blur |
| Shrp.Size | 0-10 | Sharpen frequency |

### Bake Section
| Param | Typical | Notes |
|-------|---------|-------|
| FrameRange | 16-64 | Total frames to capture |
| Rec. every | 3-7 | Skip frames (2-5 smooth, 12+ stopmotion) |
| Loopback | 0-32 | Crossfade frames for seamless loop (0=one-shot) |

## Effect Recipes (Correct Parameters)

### Fire/Flame
```
Amplify: 20-40
Turbulence: 0.5-1.0
Input Weight: 0.13-0.25
Out Weight: 0.75-0.85
Shading: 0.3-0.5
FrameRange: 24-32
Rec.every: 3-5
Loopback: 24 (for loop)
```

### Smoke
```
Amplify: 10-25
Turbulence: 0.3-0.6
Input Weight: 0.1-0.2
Out Weight: 0.6-0.8
Sharpen: -0.15 (slight blur)
FrameRange: 32-64
Rec.every: 5-8
Loopback: 32
```

### Explosion (Non-Looping)
```
Amplify: 40-60
Turbulence: 0.6-0.9
Input Weight: 0.3-0.5
Out Weight: 0.7-0.9
Contrast: 0.1-0.2
FrameRange: 16-24
Rec.every: 2-3
Loopback: 0 (one-shot!)
```

## Niagara Integration Checklist
- [ ] Emitter Sim Target: **GPU** (mandatory)
- [ ] Local Space: **ON**
- [ ] Add Sample Texture module
- [ ] Configure UV mapping (position -> 0-1 range)
- [ ] Link OUTPUT SampledColor to parameters
- [ ] Add solver module at bottom of stack

## UV Transform Formula
```
UV.x = (ParticlePos.X * Scale) + OffsetX
UV.y = (ParticlePos.Y * Scale) + OffsetY

Typical values:
Scale: 0.001 (adjust to grid size)
Offset: 0.5 (center)
```

## Velocity Transform Formula
```
WorldVelocity = (SampledRGB - 0.5) * Scale

Typical values:
Offset: (-0.5, -0.5, 0) or (-0.25, -0.25, 0)
Scale: 250-3000 (adjust to effect size)
Note: Negate Y if using FlowMap Painter
```

## Output Paths
```
/Game/FluidNinja/
├── Input/FluidPresets/    <- Preset DataTables
├── BakedData/             <- Output flipbooks & materials
├── Core/AtlasPlayer/      <- NinjaPlay base materials
├── Core/FlowPlayer/       <- FlowPlay base materials
└── Usecases/              <- Demo levels (100+)
```

## Key Documentation
- **Local Manual**: `references/FluidNinjaManual.pdf`
- Videos: youtube.com/playlist?list=PLVCUepYV6TvPnVELcyGoSDhpoLO7PyT6_
- Discord (free version): Search "FluidNinja Community"

## Performance Budget (IPP)
```
Simple playback (no blending): 36
Linear interpolation: 64
Velocity interpolation: 92
POM only: 147
RM only: 148
POM + RM full: 233
```

## Volume Types (UE5+)
- FVOL: Fog Volume (classic)
- CVOL: Cloud Volume
- HVOL: Heterogeneous Volume (UE5.4+)

## Common Fixes
| Problem | Solution |
|---------|----------|
| Particles not visible | Engine Scalability -> "EPIC" |
| Texture not sampling | Set Sim Target to GPU |
| No particle movement | Check solver module exists |
| Flipbook not animating | Connect Engine Time to Phase |
| Wrong direction | Negate Y scale, check coordinate space |
| RM shadows wrong | Update light vector each frame |
| **Straight pipe/bar in trail** | Increase Input Scale (30 → 100+) to zoom past emission origin |
| Trail too uniform | Increase Turbulence, add Curl, reduce Velocity Offset |

## Module Order (Particle Update)
```
1. Sample Texture / Flipbook
2. Acceleration Force
3. Drag
4. Collision (optional)
5. Solve Forces and Velocity (LAST)
```

## Memory Guidelines
- 1 effect: 1-4 MB total
- vs VDB equivalent: 100-400 MB
- Runtime: Zero sim cost (baked = just texture sampling)

## Non-Looping Explosions Trick
1. Set particle input to repeat the burst
2. PAUSE sim when cycle ends (before restart)
3. Set all parameters
4. Press RECORD (won't start until Play)
5. Press PLAY to begin capture
6. Set Loopback: 0
