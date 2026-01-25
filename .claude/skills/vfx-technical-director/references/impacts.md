# Impacts, Explosions, and Destruction

Fast, violent effects. Key challenge: delivering visual punch while managing performance spikes.

## Characteristics of Impact Effects

- **Burst nature**: Many particles spawned instantly
- **Short duration**: Most particles die within 0.5-2 seconds
- **Multi-element**: Fire + smoke + debris + shockwave + light
- **Performance spike**: Acceptable briefly, must settle quickly

---

## Anatomy of an Explosion

### Layer Stack (front to back)

| Layer | Purpose | Renderer | Timing |
|-------|---------|----------|--------|
| Flash | Initial brightness | Sprite or light | Frame 1-3 |
| Core fire | Hot center | Mesh or sprites | Frames 1-20 |
| Expanding fire | Outward flame | Velocity sprites | Frames 5-40 |
| Smoke | Obscures, adds mass | Billboard sprites | Frames 10-120 |
| Debris | Physical chunks | Mesh particles | Frames 1-60 |
| Shockwave | Ground distortion | Decal or ring sprite | Frames 1-30 |
| Dust | Settling aftermath | Billboard sprites | Frames 30-180 |
| Sparks/embers | Lingering detail | Small sprites | Frames 20-120 |

### Timing Principle

Stagger elements for readability:
1. Flash (immediate)
2. Fire expands (0-0.5s)
3. Smoke billows through fire (0.2-1s)
4. Debris arcs and lands (0.1-1s)
5. Dust settles (0.5-3s)

---

## Technique Selection by Scale

### Small Impact (bullet hit, footstep)

**Approach:** Simple sprite burst + optional decal

- 5-15 sprites, short lifetime
- Ground decal for persistent mark
- No mesh particles needed
- No light

**Budget:** Dozens simultaneous acceptable

---

### Medium Explosion (grenade, magic spell)

**Approach:** Multi-layer sprite system

- Flash sprite (1 frame, large)
- Fire sprites (velocity-aligned outward, 20-40 count)
- Smoke sprites (slower, rising, 10-20 count)
- Optional debris meshes (5-10 if needed)
- Optional point light (1, brief duration)

**Budget:** 5-10 simultaneous acceptable

---

### Large Explosion (vehicle, building)

**Approach:** Full layer stack + mesh debris

- All layers from anatomy section
- Mesh debris with physics (CPU sim or pre-baked trajectories)
- Point light (1, animated intensity)
- Screen shake / camera effect
- Ground decal (scorch mark)
- Possibly secondary explosions (delayed child systems)

**Budget:** 1-3 simultaneous, LOD for distant

---

### Massive/Hero Explosion (nuke, boss death)

**Approach:** Volumetrics + full production

- True volumetric mushroom cloud (HVOL)
- Multi-system composition
- Post-process effects (bloom spike, exposure)
- Sound design integration
- Possibly cutscene/camera takeover

**Budget:** 1 at a time, hero moment

---

## Debris Considerations

### When to Use Mesh Debris

- Medium+ explosions where chunks should be visible
- Destruction effects (breaking objects)
- When debris needs to interact with environment

### Debris Simulation Options

| Option | Fidelity | Cost | Use When |
|--------|----------|------|----------|
| GPU particle meshes | Low (no collision) | Cheap | Visual only, falls through floor |
| CPU particle meshes | Medium (collision events) | Medium | Needs to land on surfaces |
| Physics-driven meshes | High (full rigid body) | Expensive | Hero destruction, few pieces |
| Pre-baked trajectories | Medium (deterministic) | Cheap | Repeated effect, same every time |

### Debris Material

- Usually opaque, not translucent
- Can use instanced static mesh for better batching
- LOD: Replace with sprites at distance

---

## The Flash Problem

Initial explosion flash can either:
- **Sprite**: Large additive sprite, fades quickly
- **Point light**: Affects scene, more realistic, expensive

### Point Light Guidelines

- **Duration**: 0.1-0.3 seconds max
- **Intensity curve**: Spike then rapid falloff
- **Count**: 1 per explosion, disable for distant/LOD
- **Never**: Multiple persistent lights

---

## Shockwave / Distortion

### Techniques

| Technique | Look | Cost |
|-----------|------|------|
| Ring sprite (additive) | Visible energy ring | Cheap |
| Decal projection | Ground-only ring | Cheap |
| Screen-space distortion | Heat shimmer effect | Medium |
| Mesh ring (expanding) | 3D shockwave | Medium |

### Screen Distortion Setup

- Render distortion normal to scene capture or refraction
- Animate UV offset based on distance from center
- Use with caution — disorienting if overused

---

## LOD Strategy for Explosions

| Distance | Adjustment |
|----------|------------|
| Near | Full effect |
| Medium | Disable debris physics, reduce particle counts 50% |
| Far | Sprite-only, no mesh, no light |
| Very far | Single billboard, no simulation |

**Critical:** Explosions have performance spikes. LOD prevents distant explosions from contributing to frame drops.

---

## Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| All particles spawn frame 1 | Unreadable mess | Stagger timing per layer |
| Debris with no collision | Falls through floor, looks broken | CPU sim or fake landing |
| Long-lived explosion particles | Performance drag, visual noise | Kill particles, use decals for persistence |
| Same explosion at all distances | Wasted GPU on far effects | Implement LOD |
| No flash | Explosion lacks punch | Add brief flash sprite/light |
| Too much smoke | Obscures gameplay | Balance smoke density and fade |

---

## Impact Decals

Explosions should leave marks:

### Decal Types

- **Scorch**: Dark, radial, persistent
- **Crater**: Normal-mapped depth illusion
- **Debris scatter**: Small detail decals

### Decal Placement

- Spawn at impact location
- Project onto surfaces
- Fade over time or on limit reached
- Pool and recycle

---

## Performance Checklist

Before finalizing explosion effect:

- [ ] Peak particle count acceptable for target hardware
- [ ] Particles die quickly (not lingering)
- [ ] LOD implemented for distance
- [ ] Point lights limited (max 1, brief)
- [ ] Mesh debris count reasonable (or disabled at distance)
- [ ] Multiple simultaneous explosions tested
- [ ] Overdraw acceptable during peak
