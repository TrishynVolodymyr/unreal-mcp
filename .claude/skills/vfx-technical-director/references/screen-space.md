# Screen-Space Effects: Distortion, Post-Process, and Overlays

Effects rendered in screen space rather than world space. Different rules apply.

## What is Screen-Space?

Screen-space effects operate on the rendered image, not in 3D world:
- Applied after scene renders
- Resolution-dependent
- No depth/parallax (2D layer)
- Cheap relative to world particles

---

## Effect Types

| Type | Technique | Use |
|------|-----------|-----|
| Distortion | Refraction/UV offset | Heat shimmer, shockwaves, drunk/dazed |
| Color grading | Post-process volume | Mood, damage indication, environment |
| Vignette | Post-process | Damage, focus, cinematic |
| Blur | Post-process | Depth of field, damage, speed |
| Overlay | Screen sprite/material | Rain drops, blood splatter, frost |
| Chromatic aberration | Post-process | Damage, tech malfunction |
| Film grain | Post-process | Cinematic, horror |

---

## Distortion / Heat Shimmer

### What It Is
UV offset of the scene behind an object, creating refraction/ripple effect.

### Techniques

**Particle-based distortion:**
- Particles render to distortion buffer
- Each particle offsets UVs in its area
- Good for localized effects (near explosions, hot surfaces)

**Post-process distortion:**
- Full-screen effect
- Driven by mask or procedural
- Good for global effects (underwater, drunk vision)

### Particle Distortion Setup

1. Material Domain: Surface
2. Blend Mode: Translucent
3. Enable "Render After DOF" for proper ordering
4. Use refraction or scene color UV offset

**Material approach:**
```
Normal input: Distortion direction
Refraction: Scene UV offset amount
```

### Common Uses

| Effect | Implementation |
|--------|---------------|
| Heat shimmer | Particles with animated noise normals |
| Shockwave ring | Expanding ring sprite with radial distortion |
| Underwater | Post-process with sine wave UV offset |
| Dazed/drunk | Post-process with animated distortion |
| Portal edge | Mesh with refraction material |
| Impact ripple | Decal or sprite with radial distortion |

### Performance

Distortion is relatively cheap but:
- Multiple overlapping distortions compound
- Full-screen distortion affects everything
- Mobile: May need fallback or disable

---

## Damage Indicators

### Vignette

Darkened/colored edges of screen.

**Implementation:** Post-process material with radial gradient.

**Parameters:**
- Intensity: How dark/strong
- Color: Red for damage, blue for cold, etc.
- Radius: How far from edge

**Animation:** Pulse on damage, fade out over time.

### Screen Blood/Cracks

**Technique:** Screen-space overlay sprites or material.

**Options:**
1. **Sprite overlay**: UI widget with blood splatter texture
2. **Post-process**: Material samples blood mask texture
3. **Decal-like**: Multiple positioned elements

**Design consideration:** Don't obscure gameplay too much.

### Damage Flash

Brief screen tint:
- Red flash on hit
- White flash on crit
- Full screen for 1-3 frames

**Implementation:** Post-process with animated color multiply.

---

## Environmental Overlays

### Rain on Camera

Water droplets on the "lens."

**Technique:**
- Screen-space sprite particles
- Normal-mapped droplet material
- Distortion behind each droplet
- Droplets streak/fall based on "camera" movement

**Note:** This is a cinematic choice — implies a camera exists in the game world.

### Frost/Ice

Frost creeping from screen edges.

**Technique:**
- Animated mask (grows from edges)
- Post-process or UI overlay
- May include ice crystals and distortion

### Dirt/Blood Splatter

Persistent screen contamination.

**Technique:**
- UI overlay with alpha mask
- Additive layers for accumulation
- May require "wipe" mechanic to clear

---

## Speed/Motion Effects

### Radial Blur

Blur emanating from screen center. Conveys forward speed.

**Implementation:**
- Post-process radial blur
- Intensity based on speed
- Center point at vanishing point

### Motion Lines

Stylized speed lines (anime-style).

**Technique:**
- Screen-space particles or overlay
- Lines radiate from center
- Alpha varies with speed

### Tunnel Vision

Edges darken/blur at high speed.

**Combination of:**
- Vignette (darkening)
- Radial blur (motion)
- FOV change (if applicable)

---

## Post-Process Volumes

### Global vs Local

| Type | Use |
|------|-----|
| Global (Infinite Extent) | Default scene look, always applied |
| Local (Bounded) | Area-specific (underwater, cave, fire zone) |

### Blending

Multiple post-process volumes blend by priority and weight.

Use for:
- Transitioning between environments
- Temporary effects (damage pulse)
- Zone-based atmosphere

### Common Parameters

| Parameter | Effect |
|-----------|--------|
| Color grading | Overall color/contrast/saturation |
| Bloom | Glow on bright areas |
| Vignette | Edge darkening |
| Chromatic aberration | Color fringing |
| Film grain | Noise overlay |
| Depth of field | Focus blur |
| Motion blur | Movement smear |
| Exposure | Brightness adjustment |

---

## Performance Considerations

Screen-space effects are usually cheap BUT:

| Concern | Mitigation |
|---------|------------|
| Full-screen overdraw | Limit simultaneous full-screen effects |
| Post-process stacking | Combine into fewer passes when possible |
| Resolution scaling | Effects may look different at different resolutions |
| Mobile | Many post-process features expensive or unavailable |

### Scalability

Post-process effects should scale:
- High: Full effects
- Medium: Reduced intensity, simpler math
- Low: Disabled or basic fallback

---

## Implementation Notes

### Particle vs Post-Process Decision

| Use Particles When | Use Post-Process When |
|--------------------|----------------------|
| Localized effect | Global/full-screen effect |
| Multiple instances | Single effect |
| World-positioned | Screen-positioned |
| Depth interaction needed | 2D overlay acceptable |

### Render Order

Screen-space effects render in specific order:
1. Scene renders
2. Translucent particles (including distortion)
3. Post-process chain
4. UI overlays

Plan effect placement accordingly.

### Material Domain for Screen Effects

| Domain | Use For |
|--------|---------|
| Surface (Translucent) | Particle-based distortion |
| Post Process | Full-screen effects |
| User Interface | UI overlays |

---

## Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| Distortion on opaque material | Doesn't work | Use translucent |
| Too much screen blood | Gameplay obscured | Limit coverage, fade faster |
| Post-process always on | Wasted performance | Use when needed only |
| Ignoring mobile | Effects break on mobile | Test and provide fallbacks |
| Speed effects at low speed | Looks wrong | Threshold before activating |
| Competing screen effects | Visual mess | Priority system, limit simultaneous |

---

## Quick Reference: When to Use What

| Effect Goal | Recommended Technique |
|-------------|----------------------|
| Explosion distortion | Particle shockwave ring |
| Heat shimmer | Particle planes with noise |
| Damage feedback | Post-process vignette + flash |
| Underwater | Post-process color + distortion |
| Speed feeling | Post-process radial blur + vignette |
| Weather on lens | Screen-space particles |
| Environmental mood | Post-process volume (local) |
| Cinematic moment | Post-process color grading |
