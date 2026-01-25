# Environmental Effects: Rain, Weather, and Ambient Particles

Large-scale, persistent effects covering broad areas. Performance is critical.

## Key Constraints

Environmental effects must:
- Cover large areas (often entire visible scene)
- Run continuously (not burst)
- Not dominate frame budget
- Scale with quality settings

---

## Rain

### The Challenge

Rain needs thousands of visible drops but can't actually simulate thousands of particles.

### Technique Hierarchy

| Technique | Coverage | Cost | Realism |
|-----------|----------|------|---------|
| Screen-space overlay | Full screen | Cheap | Low (2D) |
| Local particle box | Around camera | Medium | Medium |
| Full scene particles | Entire level | Expensive | High |
| Hybrid | Combination | Optimized | Best |

### Industry Standard: Local Particle Box

**Concept:** Spawn rain only in a box around the camera. Box moves with camera.

**Setup:**
- Box size: ~20-40 meters around camera
- Particles spawn at top, fall to bottom
- When particle exits bottom, respawn at top
- When camera moves, box moves (particles stay in world space or respawn)

**Particle count:** 500-2000 (not millions)

**Why it works:** Player only sees rain near camera anyway. Distant rain is fog/atmosphere.

### Rain Components

| Component | Technique |
|-----------|-----------|
| Drops (near) | Velocity-stretched sprites or mesh cylinders |
| Drops (far) | Reduce density, simpler sprites |
| Puddles | Animated ground decals |
| Ripples | Decals or particle spawns on surfaces |
| Splashes | Small bursts on impact (limited) |
| Mist/fog | Separate fog system, not particles |
| Wet surfaces | Material parameter, not VFX |

### Rain Material

- Velocity-aligned stretch
- Additive or translucent
- Simple — no complex math per drop

### Rain LOD

- Near camera: Full density, splash effects
- Medium: Reduced density, no splashes
- Far: Represented by fog/haze only

---

## Snow

Similar to rain but slower, more visible particles.

### Differences from Rain

| Aspect | Rain | Snow |
|--------|------|------|
| Speed | Fast | Slow |
| Stretch | High (motion blur) | Low (visible flake) |
| Drift | Minimal | Wind-affected, swirling |
| Accumulation | Puddles | Ground cover material |

### Snow Particle Behavior

- Add noise to velocity (flutter, drift)
- Slower fall speed
- Optional: Curl noise for wind turbulence
- Larger size, lower count than rain

### Accumulation

Snow accumulation is **material-based**, not particle:
- World-aligned blend: Snow on upward-facing surfaces
- Vertex painting or runtime mask
- NOT particle deposition (too expensive)

---

## Fog

### Types

| Type | Technique | Use |
|------|-----------|-----|
| Global fog | Exponential Height Fog | Atmosphere, distance fade |
| Volumetric fog | UE Volumetric Fog | Light shafts, localized density |
| Local fog | Particle system | Ground fog, mist patches |
| Character fog | Attached particles | Breath, aura |

### Exponential Height Fog

Built-in, very cheap. Use for:
- Distance fade (aerial perspective)
- Base atmosphere
- Height-based density (valley fog)

### Volumetric Fog (UE)

Enables:
- Light scattering (god rays)
- Localized density variation
- Shadow interaction

Cost: Moderate at default settings, expensive at high quality.

### Local Fog Particles

When you need fog that:
- Moves (blowing through area)
- Has specific shape (cloud bank)
- Interacts with gameplay

Setup:
- Large, soft sprites
- Slow movement, long lifetime
- Low count (5-20 particles can fill area)
- Depth fade to avoid hard edges

---

## Dust / Pollen / Floating Particles

### Purpose

Add life to scene. Shows air movement, atmosphere.

### Technique

- Small sprites (2-10 units)
- Very low count (50-200 in visible area)
- Slow, drifting movement
- Lit by scene (catches light beams)

### Visibility Trick

Dust is most visible when:
- Backlit (between camera and light)
- In light shafts
- Against dark backgrounds

Place dust emitters in areas with good lighting contrast.

---

## Wind Effects

Wind itself is invisible. Show it through:

| Indicator | Technique |
|-----------|-----------|
| Leaves | Particle system with mesh leaves |
| Debris | Small sprites, directional velocity |
| Grass | Material animation (vertex offset) |
| Cloth | Cloth simulation or material |
| Particles | Add wind force to all particle systems |

### Wind Zone

Use Niagara Wind Force or custom vector field:
- Define wind direction and strength
- All outdoor particles sample this
- Consistency across systems

---

## God Rays / Light Shafts

### Techniques

| Technique | Look | Cost |
|-----------|------|------|
| Volumetric fog + light | Physical light scattering | Medium |
| Radial blur post-process | Screen-space approximation | Cheap |
| Mesh beams | Geometry with additive material | Cheap |
| Particle beams | Soft sprites in ray shapes | Medium |

### Volumetric Approach (Realistic)

Requires:
- Volumetric fog enabled
- Light with "Volumetric Scattering Intensity" > 0
- Something to cast shadows (creates ray definition)

### Mesh/Sprite Approach (Stylized)

- Cone or cylinder meshes from light source
- Additive material with noise
- Cheaper, more art-directed

---

## Day/Night Considerations

Environmental VFX often need to adapt:

| Time | Adjustments |
|------|-------------|
| Day | Visible dust, light shafts, less fog |
| Dusk/Dawn | Orange-tinted particles, long shadows |
| Night | Reduce particle visibility, add fireflies/stars |
| Storm | Increase density, add lightning |

Use material parameters driven by time-of-day system.

---

## Performance Budget

Environmental effects are ALWAYS ON. Budget strictly:

| Effect | Target Particle Count | Max Overdraw |
|--------|----------------------|--------------|
| Rain | 500-2000 | 2x |
| Snow | 200-1000 | 2x |
| Dust/pollen | 50-200 | 1x |
| Ground fog | 5-30 (large sprites) | 3x |
| Ambient debris | 20-100 | 1x |

### Scalability

All environmental VFX must have scalability:
- Epic: Full effect
- High: 75% density
- Medium: 50% density, simplified
- Low: 25% or disabled

Use `User.EnvironmentalVFXScale` parameter.

---

## Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| Rain covering entire map | Millions of particles | Local box around camera |
| Snow accumulation via particles | Impossible to do well | Material-based accumulation |
| Fog sprites with hard edges | Obvious billboards | Soft alpha, depth fade |
| God rays always on | Looks bad in wrong lighting | Tie to sun angle/intensity |
| Environmental VFX not scaled | Can't ship on low-end | Implement scalability |
| Same density everywhere | Unrealistic, performance waste | Vary density by area importance |

---

## Implementation Checklist

Before finalizing environmental effect:

- [ ] Works in local space around camera OR efficiently covers area
- [ ] Particle count within budget
- [ ] Scalability parameters exposed
- [ ] Tested at target framerate with other systems running
- [ ] Day/night adaptation if applicable
- [ ] LOD implemented (simplify at distance)
- [ ] Doesn't look obviously tiled/repeating
