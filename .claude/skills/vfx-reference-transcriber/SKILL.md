---
name: vfx-reference-transcriber
description: >
  Translate visual references (videos, GIFs, images, or verbal descriptions) into precise VFX 
  specifications for Niagara particle systems. Use when user provides visual reference material 
  and needs technical VFX breakdown. ALWAYS trigger on: "here's a video", "look at this effect", 
  "I want something like", "reference video", "this is what I mean", describing motion/particles 
  they've seen, sharing URLs to VFX references, uploading images of effects, or any visual 
  reference that needs translation into implementable VFX parameters. Outputs structured specs 
  compatible with niagara-vfx-architect skill.
---

# VFX Reference Transcriber

Automatic video/GIF/image analysis → Niagara VFX specifications.

## Workflow

### Step 1: Extract Frames

When user uploads video/GIF/WebP, immediately extract frames:

```bash
python3 scripts/extract_frames.py <input_file> <output_dir> [num_frames]
```

**Supported formats**: mp4, mov, avi, gif, webp, mkv, webm

**Default**: 8 frames evenly distributed across duration

For complex motion, extract more frames (12-16) to track particle movement.

### Step 2: Analyze Frames Visually

View extracted frames and analyze:

**Frame-to-Frame Motion Analysis**
- Track individual particles across consecutive frames
- Estimate velocity from displacement between frames
- Identify acceleration/deceleration patterns
- Note turbulence and directional changes

**Per-Frame Analysis**
- Particle density and spawn rate
- Size distribution and variation
- Color values and gradients
- Opacity and emissive intensity
- Spawn source location/shape

### Step 3: Classify Effect

| Category | Visual Indicators |
|----------|-------------------|
| **Emission** | Particles leaving a source point/area |
| **Volume** | Particles filling 3D space (smoke, fog) |
| **Surface** | Particles on/from surfaces |
| **Trail** | Particles following motion paths |
| **Environmental** | Atmospheric particles (rain, dust) |
| **Energy** | Stylized non-physical particles |

### Step 4: Extract Parameters

**Spawn**
- `spawn_type`: burst | continuous | pulsing
- `spawn_rate`: count visible particles, estimate per-second rate
- `spawn_shape`: point | cone | sphere | line | mesh surface

**Motion** (from frame comparison)
- `velocity`: pixels moved ÷ frame interval → world units
- `direction`: track particle paths across frames
- `gravity`: particles curving down = gravity active
- `drag`: particles slowing = drag active
- `turbulence`: erratic paths = curl noise needed

**Appearance**
- `size`: measure particle diameter in pixels, estimate world scale
- `color`: sample RGB values, note if changing over sequence
- `opacity`: track fade patterns
- `emissive`: bright/glowing = HDR values needed

**Lifetime**
- Track particles from spawn to death across frames
- Note size/opacity/color changes over particle life

### Step 5: Output VFX Specification

```
## VFX Spec: [Effect Name]

### Classification
- Category: [type]
- Performance: Background | Gameplay | Hero
- Sim Target: GPU | CPU

### Emitter: [Name]
**Spawn**: [type], ~[rate]/s, [shape] source
**Motion**: [velocity], [direction], [forces]
**Appearance**: [size], [color], [opacity], [emissive]
**Lifetime**: [duration], [over-life curves]
**Renderer**: Sprite | Mesh | Ribbon

### Secondary Emitter (if needed)
[Same structure]

### Exposed Parameters
- SpawnRateMultiplier
- EffectScale
- [Effect-specific]

### Implementation Notes
- Key Niagara modules needed
- Performance considerations
- Reference matching priorities
```

## Frame Analysis Techniques

**Motion Tracking**
- Compare particle positions between frames 1-2, 2-3, etc.
- Consistent displacement = constant velocity
- Increasing displacement = acceleration
- Decreasing displacement = drag/deceleration
- Curved paths = forces (gravity, curl noise)

**Spawn Rate Estimation**
- Count new particles appearing between frames
- Multiply by frame rate for per-second rate

**Lifetime Estimation**
- Track recognizable particles through sequence
- Count frames visible × frame interval = lifetime

**Color Extraction**
- Sample particle colors at different life stages
- Note if hot→cold, bright→dim, or hue shifts

## Visual-to-VFX Vocabulary

See `references/visual-to-vfx-mappings.md` for:
- Fire/ember parameters
- Smoke/vapor parameters
- Magic/energy parameters
- Debris/physical parameters
- Motion descriptor translations

## Handoff to Niagara Skill

After generating spec:

> "Implement this VFX specification using Niagara MCP tools:"
> [Paste VFX Spec]

Spec format matches niagara-vfx-architect Step 1 requirements.
