# FluidNinja Volumetrics Reference

## UE5 Volume Types

| Type | Abbreviation | Use Case | UE Version |
|------|--------------|----------|------------|
| Fog Volume | FVOL | Classic exponential height fog | 4.16+ |
| Cloud Volume | CVOL | Volumetric clouds | 5.0+ |
| Heterogeneous Volume | HVOL | Detailed small-medium effects | 5.3+ |

FluidNinja 1.8+ provides unified volume function handling all three types.

## Setup Pipeline

1. Bake density flipbook with NinjaTools
2. Create Volume domain material
3. Use NinjaPlay to write flipbook output to RenderTarget
4. Sample RenderTarget in Volume material
5. Place volume mesh in scene

## Blueprint Requirements

**BP_WriteNinjaPlayOutputToRenderTarget**:
- Must be placed on level
- Executes every frame
- Fills data cache at first run (few empty slots initially)

## Volume Material Setup

```
Material Domain: Volume
Blend Mode: Additive (typical)

Inputs:
- RenderTarget (from NinjaPlay output)
- Density multiplier
- Color/Extinction settings
```

## Performance Notes

```cpp
// Console variable for quality
r.VolumetricFog.GridPixelSize 4  // High quality (~4ms on 2080Ti @ 1080p)
// Default: 8 (4x cheaper)
// Half value = 4x voxels = significantly higher cost
```

## Exponential Height Fog Setup

For volumetric fog integration:
1. Place Exponential Height Fog actor
2. Enable Volumetric Fog option
3. Set density to 0.00001 if you don't want global fog
4. Move actor to (0, 0, -9999999) to disable height-based falloff

## Integration with Particles

Combine flipbook volumetrics with cascade/Niagara particles:
- Use volumetric material on particles
- Particles receive scene lighting automatically
- Creates atmospheric integration for 2D flipbooks

## Source

FluidNinja VFX Tools 1.8+ documentation
