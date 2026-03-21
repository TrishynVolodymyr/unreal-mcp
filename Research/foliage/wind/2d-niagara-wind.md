# Girardot 2D Wind System — Niagara-Based Tiling Wind Generation

**Source:** "Experimenting with a novel technique to create 2D wind in UE5" by Girardot
**URL:** https://www.youtube.com/watch?v=P7lZ-D-Fkus
**Date:** Early 2024

---

## Overview

A 2D wind generation system built entirely in Niagara that produces a tiling, dynamically-generated wind texture. The wind texture encodes 2D velocity (direction + speed) and can change direction, speed, gust magnitude, and turbulence at runtime without the spatial coherence breakage that plagues traditional texture-scrolling approaches.

This system is specifically designed for **large-scale background wind** (medium-to-far distances). It is NOT designed for local player interactions (those would tile/repeat). Local interactions require a separate system layered on top.

---

## Three-Part Wind Architecture

Girardot frames wind systems as three distinct problems:

1. **Wind Generation** — simulate/produce wind data (THIS video's topic)
2. **Wind Application** — deform/animate foliage vertices using wind data
3. **Vertex Skinning** — reconstruct pivots/hierarchy for complex meshes (Pivot Painter, etc.)

Simple foliage (grass) can skip step 3 entirely. Complex trees need all three.

---

## Why Traditional Texture Scrolling Fails

The standard approach: scroll a noise texture in world space, use texture value to offset vertices or modulate sine waves. Problem: **changing scroll speed or direction at runtime breaks spatial coherence**.

**Root cause:** Scroll offset = `game_time * speed`. After 10 minutes (600s) at speed 1.0, offset = 600 units. Changing speed to 1.1 makes offset = 660 units — a 60-unit jump, causing a massive burst of movement. Same issue with direction changes. Rotation around a pivot point amplifies the problem at distance.

The God of War GDC tech talk describes a solution but it is complex (multiple texture samples, various tricks).

---

## Core Concept: Tiling Wave Particles

### The Tiling Principle

To make a particle tile within a region:
- Render the particle at **4 positions**, each offset by **half a tile width** from the particle center
- This covers all edge cases regardless of where the particle sits in the tile
- When a particle crosses an edge, wrap it by moving one full tile width in the opposite direction
- Sprites must not exceed tile width when scaled/rotated

### Implementation: 4 Sprite Renderers per Particle

Each Niagara particle uses **4 Sprite Renderers**, each with its position binding overridden to one of the 4 tiling offset positions. Sprites are:
- Oriented based on velocity
- Rendered with a simple radial gradient
- Colored based on 2D velocity

---

## Implementation v1: Scene Capture Component (Proof of Concept)

### Blueprint Actor Setup

A Blueprint Actor containing:
- **Scene Capture Component 2D** (top-down orthographic view)
- **Niagara Component**

**Construction Script configuration:**
- Scene Capture captures scene color in HDR
- Uses **Show Only List** containing only the Niagara system
- Niagara system set to **visible to scene captures only**
- Transform set to render particles from top-down orthographic view
- Orthographic width sent to Niagara system as parameter
- Captures every frame
- Renders only translucent particles

### Render Targets

Two render targets:
1. **Current wind RT** — receives scene capture output each frame
2. **Previous wind RT** — stores last frame's wind data (for velocity/motion vectors)

**RT Settings:**
- Size: **128x128** (as small as possible)
- Format: **16-bit, 2 channels** (RG for 2D wind velocity)
  - 32-bit is overkill
  - 8-bit requires normalization which is annoying since accumulated speed is arbitrary
  - 16-bit is the sweet spot

### Wind Parameter Exposure

Wind parameters stored in a **Material Parameter Collection (MPC)** so they are accessible from:
- Any material (foliage shaders)
- Niagara systems (via Niagara Parameter Collection linked to the MPC)
- Other game systems (rain particles reacting to wind, etc.)

Public functions on the Blueprint: `SetWindForce`, `SetWindDirection`, `SetWindSpeed`, etc.

### Niagara Emitter Settings (v1)

- **GPU Emitter**
- **Local Space**
- **Fixed Bounds** mode
- **Interpolated Spawning: OFF** (reduces cost, requires careful init logic)
- **Spawn Rate: 20**
- **Lifetime: 3 seconds**

**Initialize Particle:**
- Opacity set to 0 immediately (particles fade in; must be correct from frame 1 due to interpolated spawning being off)
- Size X = tile width, Size Y = tile width / 2
- Uniform random scale applied
- Spawned in a box based on tile width
- All particles face Z-axis

**Custom Tiling Module:**
- Input: tile width
- Gets particle position (irrelevant base, set to zero)
- Wraps position: offset by number of tiles exceeded in opposite direction (snapping to tile boundary)
- Generates 4 offset positions (each offset by half-tile) for tiling

**Particle Update:**
- Compute new velocity from current wind direction/speed (from Niagara Parameter Collection)
- Wind direction and speed **randomized per particle**
- **Lerp** towards new velocity (creates inertia)
- Apply velocity to move particles
- Re-apply tiling wrap to updated position
- Set particle color based on 2D velocity
- Curve sample on normalized age for fade in/out

### Previous-Frame Wind Copying (for Motion Vectors)

**Problem:** Foliage materials need both current AND previous frame wind to output correct velocity for TAA/TSR. Without it, temporal effects cause blur/ghosting artifacts.

**Solution with Scene Capture (problematic):**
- Use `Draw Material to Render Target` on Tick to copy current RT to previous RT before scene capture overwrites it
- Must enable "output negative values" option in material settings
- Only works in-game (bad editor experience)

**Solution with Niagara Grid2D (better):**
- Separate emitter with Grid2D Interface sized to match RT
- Simulation Stage 1: Read RT pixel values into grid cells
- Simulation Stage 2: Write grid cell values to second RT
- **One-frame delay trick:** Before overwriting with current value, store the existing value in a separate Vector2D — this creates the one-frame delay needed
- Write the delayed value to the "previous wind" RT
- Works in both editor and game

---

## Implementation v2: Custom Rasterization (No Scene Capture)

### Motivation

Scene Capture Component costs ~1ms even for a tiny 128x128 RT with simple translucent particles. Bottleneck is in the scene capture pipeline itself, not the particles.

### Core Insight

The sprite rendering is trivially simple (spherical gradient * velocity color * age fade). All inputs (position, age, velocity) are already known. No need to render sprites at all — just compute the pixel values directly in a Niagara simulation stage.

### Particle Storage: Vector4 Arrays

Each particle needs 12 floats:
- 2D position (2)
- 2D direction (2)
- Speed (1)
- Radius (1)
- Normalized age (1)
- Lifetime (1)
- Opacity (1)
- Wind reaction parameters (several)

Stored as **3 x Vector4** per particle in a Niagara array, for a fixed count of **60 particles**.

### Simulation Stage: Particle Update

A simulation stage loops through each particle with HLSL:
1. Get particle parameters from array
2. If age == 0: spawn (initialize all parameters)
3. Increment normalized age; if >= 1: recycle immediately (re-spawn)
4. Compute movement direction from wind direction (randomized per particle)
5. Lerp current direction/speed toward latest wind values (inertia)
6. Derive opacity from normalized age (bilinear gradient / curve)
7. Apply movement
8. Wrap position within render region (tiling)
9. Write updated parameters back to array

### Simulation Stage: Grid Rasterization (Brute Force)

Uses **Grid2D Interface** sized to match wind render target (128x128).

For each grid cell:
1. Loop through ALL 60 particles
2. Get particle parameters
3. Safety checks (alive, visible)
4. Compute particle's forward and right axes from movement direction
5. **For-loop: 4 tiling offsets** (same half-tile offset principle)
6. For each offset position: compute distance to grid cell
7. Derive two gradients: along forward axis and along right axis (based on particle radius) — creates a **box mask**
8. Accumulate wind contribution: particle velocity * mask * opacity
9. Also add **constant push** and **turbulence** into the mix

### Simulation Stage: Export to RT

Another simulation stage writes the Grid2D data to the render target.

### Performance Result

- **Scene Capture approach: ~1.0 ms**
- **Custom rasterization: ~0.05 ms** (on RTX 4070)
- **20x improvement**

### Bonus: RGBA Packing

Without scene capture, can use **RGBA render target** instead of just RG:
- **RG channels:** Current frame wind (2D velocity)
- **BA channels:** Previous frame wind (2D velocity)
- Single texture sample in grass material gives both current and previous wind

---

## Constant Wind + Turbulence Layer

An additional emitter renders a **single fullscreen quad** that outputs:
- A constant wind push value
- Animated noise for high-frequency turbulence

### Texture Scroll Fix for Turbulence

Instead of using `game_time * speed` for noise scrolling (which breaks on speed/direction changes), Girardot maintains a **Vector2D `texture_coordinate_offset`** that is incrementally updated each tick based on current wind direction and speed. Changing speed only affects the NEXT increment, not the accumulated offset — no burst of movement.

**Remaining unsolved issue:** Ideally the noise texture should also be rotated to face wind direction, which still has the rotation-around-pivot problem.

---

## Wind Application to Grass (Brief)

### Vertex Offset from Wind Texture

1. Sample wind RT in world space (tiled) — gives 2D velocity offset
2. Mask along grass blade length (options: UV, vertex color, baked gradient, local Z position)
3. If using local Z: value is in cm (non-normalized), scale wind accordingly

### Faking Rotation Without Pivots (Cheap Trick)

Instead of true rotation (requires baked pivots + trigonometry):
- Use **Spherical Arc Projection** material function
- Deforms mesh slightly with large offsets but usually not noticeable
- No pivot baking required
- Learned from the God of War GDC tech talk

### Normal Handling

- Use world-space normals
- Use the arc projection axis for the normal direction
- Two-sided foliage shader model

### Motion Vectors for TAA/TSR

- Use **Previous Frame Switch** node in material
- Feed current wind (RG) for this frame's offset
- Feed previous wind (BA from same RT in v2) for last frame's offset
- Material outputs correct velocity — eliminates temporal ghosting/blur

---

## UE5 Built-in Simple Wind Node (for comparison)

Girardot dissects UE's built-in `SimpleGrassWind` material function:

1. Takes XYZ world coordinates, scales them two ways to create two signals
2. Offsets coordinates based on game time (XYZ for signal 1, Y-only for signal 2)
3. Creates bilinear gradient (looping position)
4. Applies a **smoothing function** (similar to remapped sine/cosine but without trig — cheaper)
5. Converts XYZ to scalar via vector length and dot product — two values oscillating at different frequencies
6. Adds together for rotation angle
7. Rotates around fixed arbitrary axis and pivot point

**Limitation:** Boring, hard to create wind that feels alive — no positive/negative pressure waves moving across landscape with varying direction, speed, and intensity.

---

## Wind Preset System

### Struct-Based Presets

A struct listing all wind controls (direction, speed, gust magnitude, turbulence, etc.).

An **array of presets** (e.g., 12 presets mapping to the Beaufort scale).

### Single-Function Wind Control

`SetWindIntensity(Direction, Intensity)` where:
- Direction is explicit
- Intensity is 0-1 range
- 0 = first preset, 1 = last preset
- In-between values: **linear interpolation** between adjacent presets
- Creates smooth ramp-up in wind intensity

---

## Performance Summary

| Component | Cost |
|-----------|------|
| Scene Capture approach (v1) | ~1.0 ms (huge bottleneck, worse in UE5) |
| Custom Niagara rasterization (v2) | ~0.05 ms on RTX 4070 |
| Niagara particle simulation itself | Nearly negligible |
| Wind RT size | 128x128, 16-bit per channel |
| Particle count | 20 (v1 spawn rate) / 60 (v2 fixed array) |

---

## Key Technical Decisions

- **GPU Emitter** — required for Grid2D interface and HLSL custom modules
- **Local Space** — particles exist in local tile space, not world space
- **Fixed Bounds** — avoids dynamic bounds recalculation
- **Interpolated Spawning OFF** — cheaper but requires careful opacity initialization
- **MPC for wind parameters** — accessible from any material and Niagara system
- **Grid2D Interface** — enables per-pixel rasterization without render pipeline
- **3 x Vector4 per particle** — compact storage for 12 floats of particle state
- **Brute force rasterization** — loops all 60 particles per cell; acknowledged as non-optimal but sufficient

---

## Limitations

1. **No local interactions** — tiling means any interaction repeats across the landscape
2. **2D only** — no vertical wind component (sufficient for grass/ground cover)
3. **Low resolution** — 128x128 is intentionally coarse for large-scale coverage
4. **Noise rotation unsolved** — turbulence texture ideally should rotate to face wind direction but rotation-around-pivot problem remains
5. **Brute force rasterization** — O(cells * particles * 4 offsets); could be optimized

---

## References Mentioned

- **God of War GDC Tech Talk** — spherical arc projection trick for faking rotation without pivots; also discusses texture scroll coherence solutions
- **Smoothing/easing functions website** (linked in video description)
- **Girardot's previous videos:** Highpoly grass, waterfall (Previous Frame Switch node)
- **Patreon:** Wind system project files available (Tier 2); complex grass shader (Tier 3, upcoming)
