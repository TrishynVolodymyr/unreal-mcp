# Virtual Texture Terrain Projection for Grass Instances

**Source:** "Leveraging UE's Virtual Textures to better project grass instances onto terrains" by Adrien Girardot
**URL:** https://www.youtube.com/watch?v=TdS8Q5xo3-E

---

## Problem Statement

When placing grass instances on sloped terrain using the foliage tool, grass blades either:
- Align to the surface normal (causing them to tilt on slopes, looking unnatural)
- Stay vertical but hover above or clip through the terrain on uneven surfaces

The goal: keep grass blades **vertical** (straight up) while having each blade individually **flush against the landscape surface**, regardless of slope.

---

## Core Technique: Virtual Texture Height Sampling

### Overview

Use a Runtime Virtual Texture (RVT) to write the landscape's Z height, then sample that height in the grass material at each blade's pivot point. This gives per-blade vertical offset to snap grass flush to the terrain.

### Step-by-Step Implementation

#### Step 1: Landscape Material — Write Z Height into Virtual Texture

In the **landscape material**, output the landscape's **world-space Z height** into a Runtime Virtual Texture.

- Use a **Runtime Virtual Texture Output** node in the landscape material
- Write the Z (height) component of the landscape's world position into the RVT
- This bakes a height map of the terrain that can be sampled by other materials at runtime

#### Step 2: Bake Grass Pivot Points into UVs

Use a tool like **Pivot Painter** (or equivalent scripts) to bake the **pivot point position** of each grass blade into the mesh's UV channels.

- Pivot Painter stores per-instance/per-element pivot data in UV channels or vertex colors
- The pivot point represents the base/root of each grass blade where it contacts the ground
- This data is used in the grass material to know where each blade "stands"

#### Step 3: Grass Material — Sample Virtual Texture at Pivot Points

In the **grass material**:

1. **Sample the Runtime Virtual Texture** using the pivot point XY world position (from the baked UVs) as the sampling coordinate
2. This returns the **landscape Z height directly underneath each grass blade's pivot**
3. **Subtract the mesh's Z world position** from the sampled landscape Z height
4. The result is the **vertical offset** needed to push each blade up or down to be flush with the terrain
5. Apply this offset to the grass mesh's **World Position Offset (WPO)**
6. Optionally add a small additional **vertical offset on top** to prevent z-fighting or slight ground clipping

### Formula

```
vertical_offset = sampled_landscape_z (from RVT at pivot XY) - mesh_world_z
final_WPO_z = vertical_offset + small_additional_offset
```

---

## Foliage Tool Settings

When painting grass with this technique:

- **Deactivate "Align to Normal"** in the foliage tool settings
- This keeps all grass instances vertical (world-up aligned)
- The material-driven WPO handles the per-blade terrain snapping instead of geometric alignment

---

## Bonus Technique: Spline Thicken for Grass Volume

UE has a built-in **Material Function** called **SplineThicken** (found in the engine's material function library).

### How It Works

- Model grass blades as **extremely thin/flat geometry** (nearly 2D planes)
- Use the SplineThicken function in the grass material to **thicken the geometry based on view direction**
- Each grass blade will always **face the camera** (billboard-like behavior per blade)
- Gives grass **more visual volume** and avoids the flat-plane look when viewed from the side

### Why This Matters

Flat grass planes look paper-thin from side angles. SplineThicken solves this without increasing actual vertex count — it's a material-level trick that works with the view direction to ensure grass always has apparent thickness.

---

## Key Nodes and Systems Referenced

| Node / System | Purpose |
|---|---|
| **Runtime Virtual Texture (RVT)** | Stores landscape height data for sampling by other materials |
| **Runtime Virtual Texture Output** (material node) | Writes data (Z height) from landscape material into the RVT |
| **Runtime Virtual Texture Sample** (material node) | Reads RVT data in grass material using pivot XY as UV |
| **Pivot Painter** (tool/script) | Bakes per-blade pivot positions into mesh UV channels |
| **World Position Offset (WPO)** | Material output that displaces vertices at runtime |
| **SplineThicken** (material function) | View-dependent thickening for flat grass geometry |
| **Foliage Tool > Align to Normal** | Must be **disabled** for this technique |

---

## Performance Considerations

- **RVT sampling is efficient** — virtual textures are designed for large-world streaming and are GPU-cached; sampling in a material is cheap compared to CPU-based terrain queries
- **No CPU overhead per blade** — all height projection is done in the material shader (GPU-side WPO)
- **Pivot Painter data** is baked into UVs at asset time — zero runtime cost for the pivot lookup itself
- **SplineThicken** adds minimal shader cost — it's a simple view-direction-based vertex offset
- The technique works with standard foliage instancing — no special instance management needed

---

## Summary

This is a GPU-only technique that uses Runtime Virtual Textures as a communication channel between the landscape material and the grass material. The landscape writes its height; the grass reads it per-blade and offsets itself to match. Combined with disabled "Align to Normal" in the foliage tool, this keeps grass vertical and terrain-hugging on any slope — all at material-level with no CPU cost.
