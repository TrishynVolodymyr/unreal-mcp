# Stylized Trees in Unreal Engine - Process Breakdown

**Source:** "My process for creating stylized trees in Unreal Engine" by Girardot
**URL:** https://www.youtube.com/watch?v=Ai1KUsXUPU0

---

## High-Level Workflow (5 Steps)

1. Create the leaves/small branches texture set (albedo, opacity mask, normal, gradients, masks)
2. Create the mesh cards (geometry cutouts of branches/leaves)
3. Create the tree base mesh using curves in Blender
4. Instantiate mesh cards onto branches using the snap tool
5. Sculpt the tree mesh if needed, bake onto a low-poly mesh

The process is fast but highly iterative. Multiple passes are needed for desired volume, look, and feel. Key advice: try different leaf shapes, silhouettes, leaf density early and often. Check results in-engine as soon as possible to verify style fits environment.

---

## Step 1: Leaf Texture Creation

### Modeling Leaves
- Model high-poly leaves in Blender (nothing fancy, basic shapes)
- Use curves to create branch geometry
- Instantiate high-poly leaves along those curves
- Use orthographic camera to render the assembled meshes into a 2D texture sheet

### Texture Layout Planning
- Plan card types in advance: big branches with lots of leaves, smaller branches with few leaves, dead branches, etc.
- **Critical for LODs:** Leave margin/spacing between elements in the texture. Tightly packed textures make it impossible to create simplified LOD cards later.
- Try early to detour branches with the simplest geometry possible (e.g., a single quad) to foresee LOD trouble.
- Target layout: evenly spaced dot meshes with breathing room.

### Textures Rendered
Since everything is actual geometry, render as much information as possible:
- **Normal map** - using a shader to convert normals from model space to tangent space
- **Opacity mask**
- **Gradients and masks:**
  - Branch mask
  - Random value per leaf
  - Root-to-tip gradient
  - Edge gradient
- **Albedo** - hand-painted, kept simple. Too much detail in albedo leads to visual noise on foliage. Keep it minimal for stylized trees.

---

## Step 2: Mesh Cards

### Overdraw vs. Vertex Count Tradeoff
- Foliage has lots of transparency, so minimizing overdraw is best practice
- But too many vertices to tightly cut out card shapes hurts performance, especially with:
  - Poorly managed LODs
  - Complex vertex-based wind animation
- **Balance is key.** Profile with a huge forest: try high-poly cards (low overdraw) vs. low-poly cards (more overdraw) and find the ideal middle ground for your target hardware and view distance.

### Card Preparation
- Duplicate cards, fix orientation and pivots
- **Distort/bend cards** to give them volume. This drastically helps create denser-looking trees because each card is more visible from more angles.

---

## Step 3: Tree Base Mesh (Trunk + Branches)

### Curves Workflow in Blender
- Build the tree trunk and branches using curves (super fast, lots of control)
- Use Blender's 3D cursor to rapidly create arbitrary pivot points
- Rotate and duplicate groups of branches easily
- Tree base mesh done in minutes

### Two Approaches for Card Placement

**Approach A: Canopy First**
- Place cards to define the canopy silhouette and overall volume first
- Then model trunk/branches to fit around the cards
- Easier if you need a perfectly round/uniform tree shape

**Approach B: Trunk First (Recommended)**
- Model trunk and branches first
- Then duplicate cards onto the resulting mesh
- More organic result: the mesh structure constrains card placement, naturally adding randomness
- Creates decent variation in leaf density (not perfectly round or uniform)
- Harder to predict final result

### Card Placement Strategy
- Use big cards and smaller cards together for sense of volume
- Add as much randomness as possible

---

## Step 4: Sculpting and Low-Poly Creation

### ZBrush Pass
- Export tree base mesh to ZBrush for a quick, subtle sculpting pass
- Sculpt a simple tiling wood texture as well

### Low-Poly Mesh
- Hand-model the low-poly mesh rather than decimating the sculpt
- Gives much better control over final triangle count
- Keeps things as low-poly as possible
- Bake the high-poly onto the low-poly using Substance Painter

---

## Step 5: Normal Transfer for Leaf Cards

### Sphere Normal Projection
- For each tree, create a sphere
- Use Blender's **Data Transfer modifier** to project the sphere's normals onto the leaf cards
- **Effect:** Drastically removes the flat card look. Without this, one side of cards is lit and the other is dark, making cards obvious.

### Alternative Approaches
- For realistic foliage using Two-Sided Foliage shading model, you may want unmodified normals facing a specific direction
- For stylized: subsurface material with custom normals works well
- Experiment with different normal setups and shading models

---

## Step 6: UV Data Baking for Wind

- Pick a **random value per leaf card** and bake it into a second UV map
- Bake a **normalized length** value to control wind animation strength per-vertex
- These UV channels drive the wind animation in the material

---

## LOD Strategy (Manual, 3 Levels)

UE has a built-in LOD auto-generation tool, but manual LODs give more control, especially for cards with transparency.

### LOD 0 (Full Detail)
- All cards at full complexity with distortion/bending
- Full trunk detail

### LOD 1
- **Swap all cards with much simpler variants** (this is why texture layout margin matters)
- The LOD 0 -> LOD 1 switch can be noticeable: be careful with how much distortion/bend you add to LOD 0 cards, because that bend disappears on simpler LOD 1 cards
- Decimate trunk, collapse some vertices by hand
- Erase smaller branches
- **Testing technique:** Place camera at the desired switch distance, remove bits of geometry, check if you can spot the difference. If not, safe to delete.

### LOD 2
- Remove most cards, especially small ones
- Try to preserve the overall canopy silhouette
- Trunk further simplified, most branches gone

### LOD 3 (Billboard)
- Ideally a single quad billboard
- Hard to make look decent from all angles
- UE has built-in billboard LOD tools but author hasn't experimented with them yet

---

## Material Setup

### Trunk Material
- Sample maps baked from high-poly sculpt: color, normal, roughness, ambient occlusion, specular
- **Vertical color gradient:** Normalized vertical gradient with a color curve to customize color along tree height (base of trunk -> branch tips)
- **Detail texturing:** Tiling wood texture (from the sculpted tiling wood) layered on top for fine detail
- **Material switch for LODs:** Disable detail texturing feature on distant LODs (fine details not visible at distance, saves pixel instructions and texture samples)

### Leaf/Foliage Material
More complex than trunk:
- **Color map** + vertical color gradient (same principle as trunk, but uses color curve to skip a texture sampler, layers between two colors)
- **Specular and roughness:** Driven by edge gradient and root-to-tip gradient
- **Normal map:** Turned off on distant LODs
- **Opacity mask**
- **SSS (Subsurface Scattering):**
  - SSS color amount applied to leaves
  - SSS disabled on branches using the branch mask baked into the mask texture
- **Top-down color gradient**
- **Pixel Depth Offset (PDO):**
  - Offsets pixels in camera space based on random-value-per-leaf and height value
  - Helps hide the flat card look by making intersection lines between overlapping cards less apparent
  - **"A dirty trick"** but effective
  - Turned off on distant LODs
- **Wind animation:** Similar technique to the desert scene breakdown video (vertex-based, driven by UV-baked data)

### LOD Material Optimization Summary
Features disabled on distant LODs to save performance:
- Detail texturing (trunk)
- Normal map sampling (leaves)
- Pixel depth offset (leaves)

All controlled via material switches / material instances.

---

## Performance Considerations

1. **Overdraw vs. vertex count:** Find the balance for your target hardware. Profile with large forests.
2. **LOD management is critical:** Poorly managed LODs with complex wind vertex processing compound performance problems.
3. **Material LOD switches:** Disable expensive features (PDO, detail textures, normal maps) on distant LODs to reduce pixel shader cost and texture samples.
4. **Card distortion/bending:** Adds visual quality but increases LOD transition visibility. Budget accordingly.
5. **Texture layout spacing:** Enables simpler LOD cards. Plan from the start.
6. **Hand-made low-poly meshes:** Better triangle count control than automatic decimation.

---

## Techniques Applicable to General Foliage

- **Sphere normal projection** via Data Transfer modifier: applicable to any foliage cards (grass, bushes, hedges)
- **UV-baked data** (random value per element, normalized length): universal wind animation technique
- **Overdraw vs. vertex balance:** applies to all transparent foliage
- **Material LOD switches:** disable expensive features at distance for any vegetation
- **Pixel Depth Offset** to hide card intersections: works on any billboard/card-based foliage
- **Simple albedo for stylized look:** avoid visual noise, keep hand-painted textures clean
- **Gradient masks** (root-to-tip, edge, branch mask, random-per-leaf): reusable across all foliage types for material variation
- **Card distortion/bending for volume:** applicable to any card-based vegetation (grass clumps, bushes)
- **Texture sheet planning with LOD in mind:** always leave margin for simplified LOD card variants

---

## Referenced Resources
- Author's desert scene breakdown video (wind animation technique, similar workflow)
- GDC talks on foliage (linked in video description)
- Author's video on baking data into UVs
- Patreon: Cheerful World project with these trees available
