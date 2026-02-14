# PCG Pipeline Patterns — Detailed Recipes

This reference contains node-by-node pipeline recipes for common PCG use cases. Each pattern describes the nodes involved, how they connect, and which properties matter most.

Remember: **always search the node palette** to confirm class names before creating nodes. The names listed here reflect UE 5.4+ conventions but may change.

---

## Table of Contents

1. [Vegetation Scatter](#1-vegetation-scatter)
2. [Multi-Layer Environment](#2-multi-layer-environment)
3. [Grid-Based Placement](#3-grid-based-placement)
4. [Exclusion Zones](#4-exclusion-zones)
5. [Slope-Based Distribution](#5-slope-based-distribution)
6. [Path/Spline Following](#6-pathspline-following)
7. [Cluster Spawning](#7-cluster-spawning)
8. [Building/Structure Placement](#8-buildingstructure-placement)

---

## 1. Vegetation Scatter

The most common PCG pattern. Distributes trees, bushes, or grass across a landscape.

### Pipeline

```
Input → Surface Sampler → Density Filter → Transform Points → Static Mesh Spawner → Output
```

### Nodes and Roles

- **Surface Sampler** — Generates points on a surface (landscape or mesh). Key properties:
  - `PointsPerSquaredMeter`: Controls density. Start with 0.01–0.1 for trees, 0.5–2.0 for grass
  - `PointExtents`: Minimum spacing between points
  - `bUnbounded`: If true, samples entire landscape regardless of actor bounds (use carefully — expensive)

- **Density Filter** — Randomly removes a percentage of points to break up uniformity. Key property:
  - `LowerBound` / `UpperBound`: Range for the random density threshold. `(0.0, 0.5)` keeps ~50% of points

- **Transform Points** — Adds randomness to position, rotation, and scale so instances don't look copy-pasted. Key properties:
  - `RotationMin/Max`: Usually `(0,0,0)` to `(0,360,0)` for random yaw
  - `ScaleMin/Max`: e.g., `(0.8,0.8,0.8)` to `(1.2,1.2,1.2)` for subtle size variation
  - `OffsetMin/Max`: Small random position offsets to break grid patterns

- **Static Mesh Spawner** — Replaces each point with a mesh instance. Key properties:
  - `MeshEntries`: Array of mesh + weight pairs. Multiple entries = random selection per point
  - Note: MeshEntries is a nested array property — may require specialized tool support

### Tips

- For large forests, keep `PointsPerSquaredMeter` low (0.01–0.05) and rely on visual density from the meshes themselves
- Add a second Density Filter with noise-based density to create natural-looking clearings
- Align instances to surface normal by enabling the appropriate setting on the spawner

---

## 2. Multi-Layer Environment

Multiple asset types (trees + undergrowth + rocks) at different densities, all from one graph.

### Pipeline

```
Input ─┬→ Surface Sampler (sparse) → Filter → Tree Spawner ──────┐
       ├→ Surface Sampler (medium) → Filter → Bush Spawner ──────├→ Merge → Output
       └→ Surface Sampler (dense)  → Filter → Grass Spawner ─────┘
```

### Approach

Each layer is an independent branch from Input. This keeps layers decoupled — you can tune tree density without affecting grass.

- **Trees**: Low density (0.01–0.03), large spacing, significant scale variation
- **Bushes/Undergrowth**: Medium density (0.05–0.2), moderate spacing
- **Grass/Ground cover**: High density (0.5–5.0), minimal spacing, small scale

Use a **Merge/Union node** to combine all branches before Output.

### Tips

- Consider using exclusion between layers: sample bush points, then use a distance check to remove bushes too close to trees. This prevents overlap
- Each layer can have its own Transform Points node with layer-appropriate randomization

---

## 3. Grid-Based Placement

Structured layouts like orchards, fence posts, building plots, or tile grids.

### Pipeline

```
Input → Create Points Grid → Transform Points (optional jitter) → Spawner → Output
```

### Nodes and Roles

- **Create Points Grid** — Generates points in a regular grid. Key properties:
  - `GridExtents`: Size of the grid area
  - `CellSize`: Distance between points (in Unreal Units — 100 UU = 1 meter)

- **Transform Points** — Optional. Add small random offsets ("jitter") to break perfect grid alignment:
  - `OffsetMin/Max`: Small values like `(-20,0,-20,0,0,0)` to `(20,0,20,0,0,0)`

### Tips

- For fence-like linear placement, use a very thin grid (1 row) or consider the spline-based approach in Pattern 6
- Grid placement is deterministic — same settings always produce the same layout. Use seed variation for different results

---

## 4. Exclusion Zones

Keep generated content away from specific areas (roads, buildings, water).

### Pipeline

```
Input → Surface Sampler → Difference (subtract exclusion volume points) → Spawner → Output
```

### Approach

The **Difference** node takes two point sets and removes points from set A that overlap with set B. The exclusion volume generates set B.

Alternative approaches depending on available nodes:
- Use a **Bounds Filter** to remove points inside a box/sphere region
- Use a **Tag-based filter** if exclusion areas are tagged in the level
- Use an **Attribute filter** on custom point metadata

### Tips

- The exclusion volume actor needs to be in the level and referenced by the graph (or be a PCG volume with the right tags)
- For road exclusion, consider a spline-based exclusion with a distance threshold rather than a volume

---

## 5. Slope-Based Distribution

Different assets on different terrain angles — trees on flat ground, rocks on cliffs.

### Pipeline

```
Input → Surface Sampler → Get Slope Attribute → Branch ─┬→ Flat (< 30°) → Tree Spawner ──┐
                                                         └→ Steep (> 30°) → Rock Spawner ──┴→ Output
```

### Approach

After sampling the surface, calculate the slope angle at each point (the angle between the surface normal and world up). Then branch: points on gentle slopes go to one spawner, steep slopes to another.

The exact nodes for slope calculation and branching depend on what's available — search for "slope", "normal", "angle", or "attribute filter" in the palette.

### Tips

- Transition zones (25°–35°) benefit from randomized assignment — some trees, some rocks — to avoid a hard visual boundary
- Consider a third layer for moderate slopes (large bushes or shrubs) to smooth transitions

---

## 6. Path/Spline Following

Place objects along a spline path — fences, lampposts, walls, bridge supports.

### Pipeline

```
Spline Sampler → Transform Points → Spawner → Output
```

### Approach

The **Spline Sampler** generates points along a spline actor in the level at regular intervals.

Key properties:
- `PointsPerMeter` or equivalent spacing control
- Spline reference — the graph needs to know which spline actor to follow

After sampling, use Transform Points to align instances to the spline tangent (so fences follow curves naturally).

### Tips

- For fences/walls, orient mesh forward direction along the spline tangent
- For lampposts, maintain upright orientation but place at spline positions
- Combine with Grid-Based placement for repeated structures along a path (bridge planks, etc.)

---

## 7. Cluster Spawning

Groups of objects that appear together naturally — rock formations, mushroom clusters, flower patches.

### Pipeline

```
Input → Surface Sampler (sparse, cluster centers) → For Each → Sub-Sampler (dense, small radius) → Spawner → Output
```

### Approach

First, generate sparse "seed" points representing cluster centers. Then, for each seed point, generate a cluster of nearby points within a small radius. This creates organic groupings rather than uniform distribution.

The PCG **Loop/Subgraph** mechanism handles the "for each" iteration. Search for loop or iteration nodes in the palette.

### Tips

- Vary cluster size by randomizing the sub-sampler radius per seed point
- Mix mesh types within clusters for visual variety
- Keep total point counts in check — clusters can multiply quickly (100 seeds × 20 per cluster = 2000 instances)

---

## 8. Building/Structure Placement

Place buildings, structures, or large props on valid terrain.

### Pipeline

```
Input → Surface Sampler (very sparse) → Slope Filter (flat only) → Distance Filter (spacing) → Building Spawner → Output
```

### Approach

Buildings need flat ground and adequate spacing. The pipeline enforces both:

1. Sample at very low density (buildings are large, don't need many)
2. Filter to only flat terrain (slope < 5–10°)
3. Filter by minimum distance between points (buildings shouldn't overlap)
4. Spawn building meshes/Blueprint actors

### Tips

- For Blueprint-based buildings (with interiors, doors, etc.), you may need an **Actor Spawner** node instead of Static Mesh Spawner
- Consider snapping buildings to a grid for urban layouts
- Add a ground-flattening pass or foundation mesh to handle minor terrain irregularities

---

## General Tips Across All Patterns

- **Start simple, add complexity** — get the basic pipeline working before adding filters and transforms
- **Debug visualization first** — enable `bDebug` on the sampler node to see raw points before worrying about spawner output
- **Seed consistency** — during development, keep seeds constant so results are reproducible. Randomize seeds for final variation
- **Point counts** — mentally estimate: area × density = point count. If this exceeds 10,000, consider whether you need that many
- **Combine patterns** — real-world PCG graphs often combine several patterns. A forest scene might use Vegetation Scatter + Exclusion Zones + Slope Distribution + Cluster Spawning all together
