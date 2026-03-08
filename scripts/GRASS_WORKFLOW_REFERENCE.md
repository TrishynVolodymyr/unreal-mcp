# Grass Card Workflow Reference (Girardot Method)

Source: Ghislain Girardot — "My workflow to create grass assets for realtime apps using Blender"
- YouTube: https://www.youtube.com/watch?v=BFnwkBv19TA
- ArtStation: https://www.artstation.com/artwork/a0RYBR
- 80.lv interview: https://80.lv/articles/technical-challenges-of-game-environment-production

## Core Concept

Instead of drawing grass textures procedurally (PIL/Photoshop), **model 3D grass blades in Blender and render/bake them into a 2D texture**. This gives natural shadows, volume, soft edges — impossible with flat polygon drawing.

## Workflow Steps

### 1. Model Individual Grass Blades (Blender)
- Create a single blade as extruded curve or thin mesh strip
- Taper from wide base to thin tip
- Add slight curve/bend for natural look
- Vary: height, width, curve direction, lean angle

### 2. Arrange into Clump
- Duplicate blade 15-25 times
- Randomize: rotation, scale, lean, position within a small radius
- Front-facing arrangement (like a bouquet viewed from the side)
- Denser at base, sparser at top

### 3. Setup Render
- Orthographic camera, front-facing
- Transparent background (RGBA)
- Simple lighting: key light from above/front for subtle shadows
- Film > Transparent = ON
- Resolution matches target texture (512x512 or 1024x1024)

### 4. Material on Blades
- Simple diffuse/emission (grayscale for our pipeline)
- Gradient: darker at root, lighter at tips
- No complex PBR needed — we tint in UE material

### 5. Render to PNG
- Render with alpha channel
- Result: natural-looking grass clump texture with real shadows and depth

### 6. Repeat for Variants
- Rearrange blades, change seed, different blade counts
- Short grass: shorter blades, more compact
- Tall grass: taller, more dramatic curves

## UE Material Principles (from Girardot)

### Normals
- **Custom normals on grass blades** matching ground normals
- Seamless integration with terrain

### Texturing
- **World projected textures** on both terrain AND grass blades
- Same albedo/roughness where they clip → no visible seam at ground intersection

### Shading
- **Gradient root→tip** on albedo (push value at tips)
- Grass blades **don't cast shadows** (reduces visual noise)
- Two Sided Foliage shading model
- Blend Mode: Masked

### WPO Animation
- Bend, clump, tilt, wind effects via World Position Offset
- Vertex color R channel = root-to-tip gradient for WPO intensity
- Vertex color G channel = per-blade/per-clump phase offset

### Performance
- LODs (at least 2)
- Instanced rendering
- Landscape masking (hide grass under dirt blend via WPO push below ground)

## Our Pipeline Adaptation

Since we use grayscale + alpha textures (colorized in UE material):
1. Blender blades use simple white/gray material with root-to-tip gradient
2. Render to grayscale + alpha PNG
3. Each grass mesh variant gets its own baked texture
4. UE material: tint with Wabi-Kawa palette (Sage #8BA888 / Green #86B97E)

## Key Differences from Our Old Approach

| Old (PIL drawing) | New (Blender bake) |
|---|---|
| Flat polygon shapes | 3D rendered with real shadows |
| Sharp geometric edges | Soft natural edges from AA |
| No depth/volume | Subtle self-shadowing |
| Uniform brightness | Natural light variation |
| Looks "procedural" | Looks "artistic" |
