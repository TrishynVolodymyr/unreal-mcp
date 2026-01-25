# Magic Effects: Auras, Shields, Channeling, Portals

Supernatural effects not bound by physical realism. More creative freedom, but clarity matters.

## Magic Effect Categories

| Category | Characteristics | Primary Challenge |
|----------|-----------------|-------------------|
| Auras | Surround character, persistent | Performance over time, attachment |
| Shields | Defensive barriers, reactive | Surface representation, impact feedback |
| Channeling | Build-up effects, anticipation | Timing, escalation |
| Portals | Gateways, dimensional | Depth illusion, transition |
| Projectiles | See `projectiles.md` | 3D presence |
| Buffs/Status | Visual indicators | Readability, not annoying |

---

## Auras

### Purpose
Communicate character state: powered up, protected, transformed, casting.

### Techniques

| Technique | Look | Use When |
|-----------|------|----------|
| Orbiting sprites | Particles circling character | Energy, elemental power |
| Mesh shell | Geometry hugging character | Shields, solid auras |
| Ribbon spirals | Flowing energy trails | Wind, holy, nature |
| Ground decal + particles | Aura emanating from feet | Grounded power, summoning |
| Skeletal mesh attachment | Effect follows bones | Form-fitting energy |

### Attachment Methods

**Socket attachment:**
- Attach emitter to character socket (spine, root, weapon)
- Effect moves with character
- Good for localized effects

**Skeletal mesh sampling:**
- Spawn particles on mesh surface
- Particles born at bone positions
- Good for full-body coverage

**Local space simulation:**
- Particles simulate in actor's local space
- Maintains formation during movement
- Required for orbiting effects

### Performance Considerations

Auras are **persistent** — budget accordingly:
- Lower particle counts than burst effects
- Efficient materials (avoid complex math per particle)
- LOD: Simplify or disable at distance
- Consider disabling for distant NPCs

---

## Shields / Barriers

### Visual Language

Shields typically need:
- **Surface definition**: Where is the boundary?
- **Transparency**: Can see through, but barrier visible
- **Impact reaction**: Feedback when hit

### Techniques

| Technique | Look | Best For |
|-----------|------|----------|
| Mesh shell + fresnel | Visible edges, transparent center | Force fields, energy shields |
| Mesh + refraction | Distortion effect | Invisible barriers, glass-like |
| Hemisphere particles | Particle-defined surface | Magical barriers |
| Projected pattern | Hexagons, runes on surface | Sci-fi, arcane shields |

### Shield Material Approach

**Fresnel-based:**
- Edge glow (facing away from camera = brighter)
- Center mostly transparent
- Color indicates type (blue = ice, red = fire)

**Pattern projection:**
- Hex grid, rune circles, tribal patterns
- Animated: pulse, rotate, react to damage
- Use mesh UV or world-space projection

### Impact Reaction

When shield is hit:
- Ripple effect from impact point
- Brief opacity spike
- Particle burst at impact
- Damage pattern / cracks if breaking

Implementation: Pass impact world position to material, animate ripple outward.

---

## Channeling / Charge-Up

### Purpose
Build anticipation before big attack. Communicate "something is coming."

### Structure (Temporal)

| Phase | Duration | Visual |
|-------|----------|--------|
| Start | 0-0.3s | Initial spark, attention grab |
| Build | 0.3-2s | Escalating intensity, gathering energy |
| Peak | 2-2.5s | Maximum intensity, ready to release |
| Release | 2.5s+ | Transition to attack effect |

### Escalation Techniques

- **Particle count**: Increase spawn rate over time
- **Size**: Particles grow larger
- **Speed**: Orbiting particles accelerate
- **Brightness**: Emissive intensifies
- **Sound**: Audio pitch/volume rises (coordinate with MetaSound)

### Visual Patterns

| Pattern | Look | Good For |
|---------|------|----------|
| Convergence | Particles drawn inward to focal point | Energy gathering |
| Spiral | Particles orbit with decreasing radius | Wind-up, magical |
| Growth | Central effect expands | Power building |
| Accumulation | Layers stack up | Combo counter, charging |

### Implementation

Use Niagara parameters driven by Blueprint:
- `User.ChargeProgress` (0-1): Controls all scaling
- Emitters read this to adjust spawn rate, size, speed
- Material reads this for emissive intensity

---

## Portals

### The Depth Challenge

Portals imply depth — "another place" visible through the opening.

### Techniques

| Technique | Depth Fidelity | Cost | Use When |
|-----------|----------------|------|----------|
| Flat sprite/texture | None (obvious) | Cheap | Stylized, 2D games |
| Animated flipbook | Minimal | Cheap | Abstract, energy portals |
| Render target | Full (shows other location) | Expensive | Sci-fi, literal view |
| Skybox/cubemap | Environment reflection | Medium | Dimensional rifts |
| Parallax material | Illusion of depth | Medium | Magical, doesn't need accuracy |

### Portal Frame

The boundary ring/edge:
- Mesh ring with energy material
- Orbiting particles around edge
- Ribbon renderer for flowing edges
- Ground decal for arcane circles

### Energy Portal (doesn't show destination)

- Central area: Swirling noise texture, animated
- Color: Indicates destination type or magic school
- Depth illusion: Dark center, lighter edges
- Particle emission: Wisps emanating from surface

### Literal Portal (shows other side)

Requires Scene Capture:
1. Place Scene Capture 2D at destination
2. Render to texture
3. Project texture onto portal mesh
4. Update capture per frame (expensive) or static

**Performance warning:** Real-time scene capture is expensive. Consider:
- Static capture (doesn't update)
- Low resolution
- Reduced capture frequency

---

## Buffs / Status Indicators

### Design Principles

- **Readable**: Player instantly knows what it means
- **Non-intrusive**: Doesn't block gameplay view
- **Consistent**: Same buff = same visual across game

### Techniques by Intrusiveness

| Level | Technique | Good For |
|-------|-----------|----------|
| Subtle | Color tint, subtle glow | Long-duration buffs |
| Moderate | Small orbiting icons, ground glow | Medium-duration, important buffs |
| Prominent | Full aura, particle trails | Short-duration, powerful buffs |
| Overwhelming | Screen effects, camera shake | Ultimate abilities, transformations |

### Attachment Best Practices

- Attach to predictable socket (not weapon that might be sheathed)
- Consider third-person vs first-person visibility
- Don't stack 10 buff effects visually — combine or prioritize

---

## Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| Aura too dense | Obscures character, performance | Reduce count, increase transparency |
| Shield with no edge definition | Can't see boundary | Add fresnel edge glow |
| Charge effect with no escalation | No anticipation built | Animate intensity over charge time |
| Portal with no depth | Looks like flat decal | Add parallax, swirl, or darken center |
| Buff stacking visual chaos | Unreadable mess | Combine buffs, priority system |
| Persistent effects not LOD'd | Kills perf with many characters | Disable/simplify at distance |

---

## Material Techniques Commonly Used

### Fresnel
Edge detection — brighter when surface faces away from camera. Essential for force fields.

### Panning Noise
Animated turbulence. Use for energy surfaces, swirling portals.

### Dissolve
Reveal/hide effect using noise threshold. Good for spawning, teleporting.

### Refraction
Light bending through surface. Use for glass, water, heat shimmer, invisibility.

### Emissive Pulse
Sine-wave brightness animation. Use for magical glow, heartbeat effects.
