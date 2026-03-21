# Virtual Textures & Custom UVs for Grass Instances

**Source:** "Using UE's Virtual Textures & custom UVs to augment your grass instances look"
**URL:** https://www.youtube.com/watch?v=oFKFRZZk3QA
**Author:** Girardot (YouTube)

---

## Problem Statement

Standard UE grass instancing (landscape grass layer auto-spawning mesh instances) has two visual problems:

1. **No density gradient** - Grass is either fully spawned or not. There is no partial density/fade-in. You paint, instances appear at full density, period.
2. **Grass follows landscape orientation** - On slopes, grass leans sideways following the terrain normal. Real grass grows vertically regardless of slope.

The technique shown goes from that broken look to grass that remains vertical on any terrain and fades in smoothly based on the painted landscape layer.

---

## Overview of the Technique

Instead of using UE's automatic grass instancing (landscape grass type), the author:

1. **Disables automatic grass instancing** on the landscape material's grass layer
2. **Places grass instances manually** (or via foliage tool) as regular foliage instances
3. Uses **World Position Offset (WPO)** in the grass material to:
   - Push flattened vertices upward (height)
   - Scale blades around their pivot points (thickness)
4. Uses **Runtime Virtual Textures (RVT)** to let the landscape material communicate a scale factor to the grass material, controlling height/thickness based on the painted grass layer

The landscape's painted grass layer value is written into the RVT mask channel. The grass material samples the RVT to read that mask and uses it as a scale factor for the WPO operations.

---

## Custom UV Channel Setup (3D Software / Blender)

The grass mesh uses **three UV channels** total:

### UV Channel 0 (default)
- Standard texture UVs for albedo, normal, etc.
- Set up normally in 3D software.

### UV Channel 1 — Pivot Points (X, Y)
- Stores the **X and Y world-space coordinates** of each grass blade's pivot point.
- Each blade's vertices share the same UV value = their blade's pivot position.
- These are NOT normalized 0-1 UVs. They store actual world-space positions in centimeters.
- Blender's UV editor will look strange because of this (coordinates outside 0-1 range).
- **Purpose:** Allows the material to compute each blade's pivot position after all blades are merged into a single mesh. Used for:
  - Wind animation (rotate each blade around its own pivot, not the mesh origin)
  - Scale/thickness control (collapse vertices toward their pivot)
- Scripts exist for baking pivot points in 3ds Max (linked in video description). Maya and Blender scripts also available. Author wrote a custom Blender script.

### UV Channel 2 — Height + Random Value
- **V (green/Y) channel:** Stores the normalized height of each vertex (0 at base, 1 at tip). Created by doing a side-view planar projection in the 3D software.
- **U (red/X) channel:** Stores a random value per blade, for use in shaders (color variation, animation offset, etc.).

### Export Step: Flatten the Mesh
- After UV setup, **flatten/squish the entire mesh** before exporting (scale Z to 0).
- The mesh is exported as a flat pancake. Height is reconstructed entirely via WPO in the material.

---

## Material Setup — World Position Offset

### Step 1: Height Reconstruction

1. Sample UV channel 2 (TexCoord index 2), take the **Green (Y) channel** = normalized height.
2. **Flip the Y axis** (multiply by -1) — required due to graphics API differences (some APIs have UV origin top-left, others bottom-left).
3. Multiply by desired height value (e.g., 100 for 100cm tall grass).
4. **Critical:** The height value is a float (scalar). Plugging a scalar into the WPO vector3 input causes it to fill X, Y, and Z equally, resulting in a 45-degree offset. Fix: multiply the scalar by a vector `(0, 0, 1)` to isolate the Z component only.
5. Result: vertices are pushed upward along world Z axis regardless of instance rotation.

```
HeightValue = TexCoord[2].G * -1.0 * HeightParameter
WPO_Height = HeightValue * float3(0, 0, 1)
```

### Step 2: Thickness/Scale via Pivot Points

1. **Convert pivot UVs to world coordinates:** A small material function converts the pivot points stored in UV channel 1 from local/UV space back to world-space positions.
2. **Compute scale vector:** `ScaleVector = (PivotWorldPos - VertexWorldPos)` — this gives the direction and distance each vertex needs to travel to reach its blade's pivot.
3. **Multiply by scale parameter:** `ScaleVector * ScaleFactor` — at factor 0, vertices collapse to pivot (invisible). At factor 1, full size.

```
PivotWS = ConvertPivotUVToWorldSpace(TexCoord[1])  // material function
ScaleVector = PivotWS - WorldPosition
WPO_Scale = ScaleVector * ScaleFactor
```

### Step 3: Combine Height and Scale

Both WPO contributions use a **common scale factor**:

```
ScaleFactor = (from RVT mask or parameter)

WPO_Height = HeightValue * ScaleFactor * float3(0, 0, 1)
WPO_Scale  = (PivotWS - WorldPosition) * ScaleFactor

FinalWPO = WPO_Height + WPO_Scale
```

When ScaleFactor = 0, grass is flat and collapsed (invisible). When ScaleFactor = 1, grass is at full height and thickness. Values in between give a smooth gradient.

---

## Runtime Virtual Texture (RVT) Integration

### Landscape Material Side (Writing to RVT)

The landscape material writes multiple channels into the RVT output node:

- **Z World Location** (height)
- **Albedo**
- **Roughness**
- **Normals**
- **Mask** — the grass layer value is sampled and written to this input

### RVT Configuration Requirement

By default, the RVT mask input is not active. You must configure the virtual texture asset to enable the mask channel. The author mentions a specific setting is needed (the exact property name is not stated, but it involves enabling the mask channel in the RVT asset settings).

**Note:** Enabling the mask channel increases VRAM usage.

### Grass Material Side (Reading from RVT)

1. Add a **Virtual Texture Sample** node in the grass material.
2. Sample the same RVT that the landscape writes to.
3. Use the **Mask output** as the `ScaleFactor` for the combined height + thickness WPO.

This is the key link: the landscape tells the grass how tall/thick to be via the RVT mask channel.

---

## Performance Implications

The author explicitly calls out several costs:

1. **Vertex instructions in grass material:** The WPO setup adds "a couple of vertex instructions" — the pivot-to-world conversion, the scale computation, the height computation.
2. **Virtual Texture sampling:** RVT samples are "slightly more costly than regular texture samples" but the author considers this acceptable.
3. **RVT mask channel:** Enabling the mask input on the RVT leads to "heavier memory usage" (additional VRAM for the extra channel).
4. **Profiling required:** The author emphasizes this may be too costly depending on target frame rate and platform. Profile your game.

The author considers these costs acceptable for the visual quality improvement.

---

## Benefits Over Standard Grass Instancing

1. **Smooth density gradient** — Grass fades in/out based on the painted landscape layer value, instead of binary on/off spawning.
2. **Always-vertical grass** — WPO pushes vertices along world Z, so grass stands upright on slopes.
3. **Landscape controls grass appearance** — The painted material layer directly influences grass height and thickness via RVT, giving artistic control.
4. **Works with regular foliage instances** — Not limited to landscape grass types. Manual foliage placement works too.
5. **Mesh exported flat** — The flattened export means the mesh itself has minimal bounding box, and height is entirely material-driven.

---

## Implementation Checklist

1. **3D Software (Blender/Max/Maya):**
   - Model grass blades as crossboards (two planes at 90 degrees)
   - UV0: standard texture UVs
   - UV1: bake pivot point X/Y world positions per blade
   - UV2: side-view projection for height (G channel) + random per-blade value (R channel)
   - Flatten mesh (squish Z to 0) before export

2. **UE Material — Grass:**
   - TexCoord[2].G * -1 * HeightParam * float3(0,0,1) = height WPO
   - Material function to convert UV1 pivot coords to world space
   - (PivotWS - VertexWS) * ScaleFactor = thickness WPO
   - Combine both, multiply by common ScaleFactor
   - Sample RVT, use Mask output as ScaleFactor

3. **UE Material — Landscape:**
   - Write grass layer value to RVT Mask output
   - Also write albedo, roughness, normals, Z-world-location to RVT

4. **UE RVT Asset:**
   - Enable mask channel in virtual texture settings (increases VRAM)

5. **UE Landscape:**
   - Disable automatic grass instancing on the grass layer
   - Grass layer now only contributes to albedo (and RVT mask)
   - Place grass as foliage instances manually or via foliage tool
