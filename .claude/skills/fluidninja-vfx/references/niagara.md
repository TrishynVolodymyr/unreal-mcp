# FluidNinja Niagara Integration - Detailed Reference

This document contains step-by-step workflow extracted from official tutorials for integrating FluidNinja baked data with Niagara particle systems.

## Understanding Velocity Maps (Flowmaps)

Velocity maps encode motion direction and speed as color values:
- **Horizontal movement (X)**: Red channel
  - 50% = stationary
  - 100% = full speed right
  - 0% = full speed left
- **Vertical movement (Y)**: Green channel
  - 50% = stationary
  - 100% = full speed down
  - 0% = full speed up

### Creating Simple Flowmaps
Tool: FlowMap Painter v0.9.2 (free, 7MB)
- Download: http://teckartist.com/?page_id=107
- Toggle FlowLines/VertColor to visualize vectors
- Export and import directly to UE Content Browser

---

## Creating a Simple Texture Sampling Emitter

### Step 1: Create Niagara Emitter
1. Right-click Content Browser → FX → NiagaraEmitter
2. Choose "Create new emitter from template" → "Empty"
3. Save immediately

### Step 2: Add Grid Location
1. Click green (+) beside "Particle Spawn"
2. Select Location → Grid Location
3. Click "Fix Issue" when warning appears
4. "Spawn Particles in Grid" module auto-created under "Emitter Update"
5. Fix that issue too

### Step 3: Add Sample Texture Module
1. Right-click GridLocation module → Insert Above
2. Type "texture" → Select "Sample Texture"

### Step 4: Configure GPU Mode
1. Select "Emitter Properties" at top of stack
2. Set Sim Target: CPU → **GPU** (required for texture sampling)
3. Enable **Local Space**

### Step 5: Configure UV Mapping
The goal: Map particle XY position to texture UV coordinates (0-1 range)

1. In Sample Texture module, pick your texture
2. Click arrow beside UV data row → "Break Vector 2D"
3. For X (U): Click arrow → "Make Float from Vector" → Channel: X
4. For Y (V): Click arrow → "Make Float from Vector" → Channel: Y
5. Add transformation:
   - Click arrow beside X → "Add Vector"
   - Click arrow beside Vector A → "Multiply Vector by Float"
6. Link particle position:
   - Click arrow beside input → Link Inputs → Particles → "Particles Position"

### Step 6: Set Transform Values
- Float multiplier: 0.001 (scale factor)
- X,Y offset: 0.5 (center adjustment)

Adjust these based on your grid size and texture resolution.

### Step 7: Route to Particle Color
1. Select "Initialize Particle" module
2. Click arrow beside "Color" input
3. Link Inputs → Output → "OUTPUT SAMPLE TEXTURE SampledColor"
4. Set "Sprite Size" to ~15

### Step 8: Add Velocity from Sampled Data
1. Click green (+) beside "Particle Spawn"
2. Search "velo" → Select "Add Velocity"
3. Click "Fix Issue" to add solver
4. Drag "Add Velocity" ABOVE "Apply initial forces"

### Step 9: Configure Velocity Transform
Velocity data needs transformation from texture space (0-1) to world space:

1. Select "Add Velocity" module
2. Click arrow beside acceleration/velocity row → "Add Vector"
3. Click arrow beside Vector A → "Make Vector3 from Color"
4. Link sampled color output
5. Set Vector B (offset): **(-0.25, -0.25, 0)**
6. Set Scale: **(250, -250, 0)** (negative Y if FlowMap Painter used)

### Step 10: Final Tweaks
- Adjust lifetime in "Initialize Particle"
- Add "Scale Color" module at Particle Update
- Draw bell curve for alpha fade

---

## Creating a Flipbook Playing Emitter

Extends the simple emitter to use animated baked simulation data.

### Step 1: Modify Base Emitter
1. Disable "Add Velocity" in Particle Spawn
2. Drag "Sample Texture" module to Particle Update group
3. Add "Sub UV Texture Sample" module at Particle Update

### Step 2: Create Custom Module
1. Double-click Sample Texture to open graph editor
2. Click "Browse" to locate in Content Browser
3. Right-click → Duplicate → Name "SampleTextureFlipbook"
4. Save new module

### Step 3: Merge Module Functionality
1. Disable both original modules (Sample Texture, Sub UV)
2. Add your new module to Particle Update
3. Open both modules in graph editor (two tabs)
4. From SubUV tab: Copy MapGet and SubUVTextureCoordinates nodes
5. Paste into custom module
6. Connect nodes appropriately

### Step 4: Configure Flipbook
1. Select custom module in stack
2. Pick velocity flipbook as Texture input
3. Set X-count and Y-count (e.g., 7x6 for 42 frames)
4. Copy/paste UV setup from original Sample Texture
5. Connect "Engine Time" to Phase input for playback

### Step 5: Route to Acceleration
1. Add "Acceleration Force" module (Insert Above Solve Forces)
2. Configure transform chain:
   - Click arrow → "Multiply Vector by Float"
   - Click arrow beside vector → "Add Vector"
   - Link flipbook output to Vector A
3. Set Vector B: **(-0.5, -0.5, 0)**
4. Set Float multiplier: **3000**
5. Set coordinate space: **Local**

### Step 6: Add Drag
1. Add "Drag" module below Acceleration Force
2. Set Drag value: ~3

### Step 7: Fine Tuning
- Emitter State: Loop duration ~0.5
- Initialize Particle: Lifetime ~4, sprite size ~5
- Grid Location: Enable "Randomize Placement Within Cell"
- Add Collision module if needed

---

## Key Performance Settings

### Emitter Configuration
```
Sim Target: GPU (mandatory)
Local Space: ON (recommended)
Fixed Bounds: Enable for better culling
```

### Grid Spawning
```
X/Y Count: Match texture resolution ratio
Dimensions: Scale to world units
Randomize: Add variation per cell
```

### Flipbook Playback
```
Frame Count: Match baked flipbook
Phase: Engine.Time for automatic playback
UV Scale: Calculate from grid dimensions
```

---

## Common Module Chains

### Initial Position + Texture Color
```
Grid Location → Sample Texture → Initialize Particle (Color)
```

### Initial Velocity from Flowmap
```
Sample Texture → Add Velocity → Apply Initial Forces → Solve
```

### Continuous Acceleration from Flipbook
```
Sample Texture Flipbook → Acceleration Force → Drag → Solve
```

### Collision + Fluid Motion
```
Sample Flipbook → Acceleration → Drag → Collision → Solve
```

---

## Module Graph Patterns

### Sample Texture UV Setup (Graph)
```
[Particles.Position] → [Make Float from Vector (X)] → [Multiply by Float] → [Add Vector] → UV.X
[Particles.Position] → [Make Float from Vector (Y)] → [Multiply by Float] → [Add Vector] → UV.Y
```

### Velocity Transform (Graph)
```
[OUTPUT SampledColor] → [Make Vector3 from Color] → [Add Vector (offset)] → [Multiply by Float (scale)] → Velocity
```

---

## Troubleshooting

### Particles Don't Move
- Check "Apply force to velocity" flag in "Apply initial forces"
- Verify solver module exists and is positioned last
- Check velocity scale isn't zero

### Texture Not Sampling
- Confirm GPU Sim Target (CPU cannot sample)
- Check texture is assigned in module
- Verify UV transformation produces 0-1 range

### Flipbook Not Animating
- Connect Engine Time to Phase input
- Check X/Y count matches flipbook layout
- Verify SubUV coordinates are connected

### Wrong Direction
- FlowMap Painter inverts Y - use negative scale
- Check UV offset centers correctly
- Verify coordinate space (Local vs World)

---

## Example Assets

Tutorial assets (2.4 MB): https://drive.google.com/file/d/1kd29dSXH3iFIbOcLz08K-R7lw-NJLZmb

Contents:
- Flowmap example textures
- Velocity flipbook (Kármán vortex street)
- Complete Niagara emitters
- Custom sampling modules

---

## Source

Based on tutorial by Andras Ketzer (FluidNinja developer)
Full article: https://80.lv/articles/tutorial-driving-niagara-with-flowmaps-and-baked-fluidsim-data
