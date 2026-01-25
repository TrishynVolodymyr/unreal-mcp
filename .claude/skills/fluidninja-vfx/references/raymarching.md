# FluidNinja Ray Marching & Parallax Occlusion Mapping Reference

This document details the techniques for adding 3D depth and self-shadowing to 2D fluid simulations.

## Overview

**Problem**: 3D volumetric simulations are GPU-heavy, VDB files are memory-heavy (128x more than 2D flipbooks at same resolution).

**Solution**: Use 2D data enhanced with depth illusion techniques for fixed-camera or background effects.

---

## Ray Marching (RM)

### Concept
Ray Marching shades 2D density data dynamically by sampling from light direction. Creates gorgeous self-shadowing that responds to scene lighting.

### How It Works
1. Light ray penetrates each point of density "landscape"
2. Lower density = lower light absorption (Beer-Lambert law)
3. Rays hit points until completely absorbed
4. Results in soft shadows with subsurface-scattering appearance

### Requirements
- **Unlit density data**: No pre-baked lighting/shadows in flipbook
- **Unsampled texture**: Texture Object input (not pre-processed)
- **Light vector input**: Direction to light source

### Node Setup
```
[Texture Object (Density)] → [RayMarchHeightMap]
[Light Vector (Vec3)]      → [RayMarchHeightMap] → Output
```

### Light Vector Calculation

**Directional Light:**
- Transform rotation to vector direction
- Pass as Vec3 parameter

**Point Light:**
- `LightVector = VFXMeshWorldPos - PointLightWorldPos`
- Normalize result
- Update every frame via Blueprint

### Frame Interpolation
RM node does NOT interpolate between flipbook frames. Solutions:
1. Use NinjaPlayBasic to write blended output to RenderTarget
2. Feed RenderTarget to RM node
3. NinjaPlay uses velocity data for smooth interpolation

### Performance
- ~148-182 IPP (instructions per pixel)
- Add ~50 IPP for advanced transparency

---

## Parallax Occlusion Mapping (POM)

### Concept
Adds 3D depth without geometry by splitting 2D texture into layers based on heightmap (density). Creates illusion that some texels are further from surface.

### How It Works
1. Density map interpreted as heightmap
2. Texture space split into "cardboard cutout" layers
3. UV offset applied based on camera vector
4. Texels appear at different depths

### Capabilities
- Offset UVs for depth illusion
- **Pixel Depth Offset**: Accurate cast shadow capture (opaque/masked only)
- Does NOT work with translucent materials for shadow capture

### Node Setup
```
[Texture Object (Density)] → [ParallaxOcclusionMapping]
[Camera Vector]            → [ParallaxOcclusionMapping] → Offset UVs
```

### Performance
- ~147 IPP base
- Lighter than Ray Marching
- Good for mid-distance effects

### Note
Self-shadow output broke in UE 4.24+ (somewhat redundant with RM anyway)

---

## Combined Setup

### Material Structure
Best results using both techniques together:

```
NinjaPlayBasic (Material)
    ↓ writes to
RenderTarget (Texture)
    ↓ samples by
Volume Material
    ├── RayMarchHeightMap (shadows)
    └── ParallaxOcclusionMapping (depth)
```

### Blueprint Requirements
**BP_WriteNinjaPlayOutputToRenderTarget**:
- Must be placed on level
- Executes every frame
- Fills data cache at first run (few empty slots initially)

### Project Structure
```
/ParallaxMapping/
├── BakedData/
│   ├── T_Density_Flipbook
│   ├── T_Velocity_Flipbook
│   └── MI_NinjaPlayer_*
├── RT_* (RenderTargets)
├── BaseMaterials/
│   ├── M_RayMarch_Base
│   └── M_POM_Base
├── MaterialInstances/
│   └── MI_Combined_*
└── BP_WriteNinjaPlayOutputToRenderTarget
```

---

## Performance Benchmarks

### Dry Ice Test Scene (GTX 1070, 1080p)

**Overall**:
- Memory: 1.25 MB texture memory (density + velocity flipbooks)
- GPU: 281 IPP (full pipeline)
- FPS: 182 (standard), 250 (optimized)

**Optimizations Applied**:
- Disabled additional volume particles
- Switched to linear frame blending
- Disabled DepthFade in transparency
- Baked scene lighting (only RM light dynamic)

### Material Variants (IPP)

| Configuration | Instructions |
|--------------|--------------|
| POM + RM + advanced transparency | 233 |
| POM + RM + basic transparency | 199 |
| RM only + advanced transparency | 182 |
| RM only + basic transparency | 148 |
| POM + basic transparency | 147 |

### Flipbook Player Variants (IPP)

| Configuration | Instructions |
|--------------|--------------|
| Velocity interpolation | 92 |
| Linear interpolation | 64 |
| No frame blending | 36 |

---

## Texture Assets Specifications

### Density Flipbook (DryIce Example)
- UAsset size: 1024 KB (1 MB)
- Frames: 32 (8x4 matrix)
- Resolution: 2048 x 1024
- Channels: Monochrome
- Compression: Alpha, BC4 (DX11)

### Velocity Flipbook (DryIce Example)
- UAsset size: 256 KB (0.25 MB)
- Frames: 32 (8x4 matrix)
- Resolution: 1024 x 512
- Channels: RGB
- Compression: Default, DXT1

---

## Example Levels

### Use Cases Additional 1
Grid layout of techniques:
- **Column 1**: Vector-field driven GPU particles
- **Column 2**: Flipbooks with NinjaPlay
- **Column 3**: RM + POM advanced materials
- **Column 4-6**: Simple materials (POM opaque, translucent, normal-based)

**Top row**: Interactive in-editor set - move light bulb, see RM/POM respond

### Use Cases Additional 2 (Cauldron Scene)
Three lighting setups:
- **(A)** Moving point light controlled by sine/cosine in LevelBlueprint
- **(B, C)** Static lights with manually matched material parameters

---

## In-Editor Lighting Interaction

**Blueprint**: BP_InEditorLigtingTest

Calculates light vector in Construction Script, enabling real-time preview without PIE.

### Light Vector to Material
1. Read light position from scene
2. Transform to direction vector
3. Push to material instance as "LightingDirection" Vec3 parameter
4. Update per frame (via Event Tick or Construction Script)

---

## Normal-Based Alternative

For mobile or performance-critical use:
- Generate normal maps from density data
- Use standard normal-based lighting/shading
- Much lighter than Ray Marching
- Online tool: http://cpetry.github.io/NormalMap-Online/

Material Function for normal generation: Available in FluidNinja package.

---

## Volumetric Fog Integration

Enhance 2D effects with UE Volumetric Fog:

1. Add cascade particles with volumetric material
2. Particles receive scene lighting automatically
3. Creates dim fog around main effect
4. Helps 2D flipbooks appear more integrated in 3D space

### Performance Note
```cpp
r.VolumetricFog.GridPixelSize 4  // ~4ms on 2080Ti @ 1080p
// Default: 8 (4x cheaper)
// Half value = 4x voxels = 4x+ cost
```

---

## Tutorial Resources

**Video**: FluidNinja VFX Tools 1.01 - Parallax Occlusion Mapping
https://youtu.be/-gDVyrPyvEs

**Project Download** (45 MB, UE 4.20+):
- Mirror 1: https://app.box.com/s/b6vaktriduhamucu5mgefl6j6hiffgo2
- Mirror 2: https://drive.google.com/file/d/1XHKq-X1kH_HM4xCuPz89F-hQ7xpWfi35

---

## Source

Based on article by Andras Ketzer (FluidNinja developer)
Full article: https://80.lv/articles/adding-depth-and-realism-to-2d-fluids-in-unreal
