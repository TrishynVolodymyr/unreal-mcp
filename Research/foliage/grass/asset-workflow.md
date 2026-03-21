# Grass Card Workflow Reference (Girardot Method)

Source: Ghislain Girardot — "My workflow to create grass assets for realtime apps using Blender"
- YouTube: https://www.youtube.com/watch?v=BFnwkBv19TA
- ArtStation: https://www.artstation.com/artwork/a0RYBR
- 80.lv interview: https://80.lv/articles/technical-challenges-of-game-environment-production

Transcript extracted via `youtube-transcript-api` Python package (pip install youtube-transcript-api).

## Core Concept

Model 3D grass blades in Blender, arrange into **small clumps** (3-5 blades each), render via orthographic camera to get a texture with multiple separated clumps. Then trace low-poly cards over each clump and arrange cards into the final game asset.

**CRITICAL: The texture and cards are PAIRED. Each card is traced from a specific clump in the texture. The texture must have small separated clumps with gaps — NOT one continuous wall of grass.**

## Complete Workflow (from video transcript)

### Phase 1: Setup
- Delete everything, create orthographic camera pointing down
- Set render resolution (e.g., 1024×512)
- Color management: View Transform = Raw (no color correction)
- Film > Transparent = ON
- Background color = pure black (no ambient light)
- Create a canvas plane matching ortho width + aspect ratio (for reference)

### Phase 2: Model High-Poly Blades
- Create a plane, scale X to make a thin strip
- Add vertical edge loop (bevel weight = 1.0), add horizontal edge loops
- Proportional editing to taper tip, adjust curvature
- Add Bevel modifier (limit by weight, 1 segment)
- Add Subdivision modifier (2 viewport, 3 render)
- Add Solidify modifier (slight thickness)
- Shade smooth
- Result: one high-poly grass blade base model

### Phase 3: Create PBR Passes via Shader AOVs
- Create Shader AOVs: albedo, roughness, specular, opacity, normal, random, height, transmission
- In compositor: file output node with inputs matching each AOV
- In material: AOV Output nodes for each pass
- Use UV-based gradients for roughness/specular (center highlight, edge variation)
- Albedo: color gradients + per-object random value for tint variation
- Normal: remap world-space normals to 0-1 range, flip green channel
- Height: use a reference cube with object coordinates for vertical gradient
- Opacity: constant 1.0
- Transmission: top-down UV gradient

### Phase 4: Arrange Clumps on Canvas
**THIS IS THE KEY STEP WE GOT WRONG**
- Duplicate blade, enter edit mode, use proportional editing to create variants
- Arrange blades into **small clumps of 3-5 blades** each
- **Leave enough room between clumps** — prevents bleeding, allows individual card tracing
- Multiple small clumps spread across the canvas, NOT one dense wall
- Render → saves all PBR pass PNGs to disk

### Phase 5: Mip-Flood Dilation
- Problem: alpha-masked textures bleed black background color at distance (mip chain)
- Solution: mip-flooding (GDC 2019 technique) — dilate textures
- Alternative: GIMP value propagate filter, or duplicate+blur+merge layers
- Reapply original alpha channel after flooding
- Important for all passes, not just albedo

### Phase 6: Create Low-Poly Cards
- New scene, import opacity map as Image Mesh Plane
- Set material to use alpha clip, emission for visualization
- **In vertex mode, manually trace vertices around each clump silhouette**
- Create quads (F key) to surface the cards
- Check face orientation (normals), flip if red
- Add edge loops for WPO deformation
- Vertex placement considerations:
  - Never outside canvas bounds
  - Leave inner margin within clump
  - Leave outer margin between clumps
  - Minimize overdraw (transparent pixels)
  - Think about vertex animation deformability
  - Think about volume/bending potential

### Phase 7: LODs
- Create LOD0 (full detail), LOD1 (half verts), LOD2 (single quad or triangle)
- Use material slots (lod0, lod1, lod2) to tag cards for easy selection later
- Keep all LODs in same object (important for per-card random value consistency)

### Phase 8: Separate, Rotate, Curve
- Copy to new scene, separate cards into individual objects
- Recenter origins, apply location
- **Rotate 90° on X** (R X 90) — cards stand upright
- Apply rotation
- Use proportional editing to apply curvature:
  - Side view: bend tips slightly (sharp falloff)
  - Top view: gentle lateral bend
- Goal: each card has slight 3D volume, not perfectly flat

### Phase 9: Arrange Final Asset
- Create linked duplicate of collection
- Duplicate cards (Alt+D) 1-2 times per card
- Randomize transform to spread apart
- Manually fine-tune spacing, silhouette from all angles
- Ensure decent coverage and volume from any viewing direction
- This is the final game asset

### Phase 10: Bake & Export
- Bake pivot data (for shader tricks)
- Bake per-card random value in UVs (for wind phase offset, tint variation)
- Use material selection to isolate LODs
- Export each LOD separately as FBX
- Import to UE, set up LOD distances

## UE Material (from video)
- Albedo × top-down projected noise × per-instance/per-card random for color variation
- Roughness/specular lerped towards constants at distance (mip map tricks)
- Opacity with shadow pass switch, tip desaturation, fake light bleed
- Normal map: tangent-space normals used AS world-space (hack but effective)
- Subsurface scattering
- Wind animation via spherical reprojection material function
- Pixel depth offset at close distance (disabled at far distance)
- LOD2 (far distance): much cheaper shader — constant roughness/specular/normals, no WPO, no PDO

## Our Pipeline Adaptation

Since we use grayscale + alpha textures (colorized in UE material):
1. Blender blades use simple white/gray material with root-to-tip gradient
2. Arrange into **small separated clumps** (3-5 blades each) on canvas
3. Render to grayscale + alpha PNG — multiple clumps visible with gaps
4. Trace individual cards per clump
5. Arrange cards into final grass asset
6. UE material: tint with Wabi-Kawa palette (Sage #8BA888 / Green #86B97E)

## Lessons Learned (2026-03-14)

| What went wrong | Why | Correct approach |
|---|---|---|
| 75 blades packed edge-to-edge as one wall | Misunderstood "arrange into clump" as one big mass | Small clumps of 3-5 blades with gaps between them |
| One card covering entire texture | No distinct groups to trace | Individual cards per small clump |
| 1,440 tris per grass asset (37×5 grid) | Over-tessellated to match silhouette | ~160 tris target (8×3 grid per card, 5 cards) |
| Cross-plane meshes (SM_Grass_*) unrelated to textures | Created meshes and textures independently | Texture and card are paired — card traced FROM texture |
| FBX scale issues | global_scale=100 + apply_unit_scale=True = double scale | Use global_scale=1.0 with apply_unit_scale=True |
