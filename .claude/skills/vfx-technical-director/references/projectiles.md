# Projectiles, Beams, and Lasers

Effects that travel through 3D space and are viewed from arbitrary angles.

## The Fundamental Problem

Projectiles are viewed from **any angle** as the camera and projectile move independently. This eliminates:
- Billboard sprites (face camera = flat from side)
- POM/Ray Marching (camera-angle dependent)
- Single-plane flipbooks (no depth from perpendicular view)

## Industry Standard Approaches

### 1. Mesh Core + Sprite Accents (Most Common)

**Structure:**
- **Core**: Mesh particle (sphere, elongated capsule, or custom shape)
- **Surface detail**: Small velocity-aligned sprites orbiting the core
- **Trail**: Ribbon renderer for motion trail
- **Glow**: Point light (optional, expensive) or emissive bloom

**Why it works:** Mesh provides true 3D silhouette. Sprites add detail but aren't the hero element.

**Material approach:**
- Core mesh: Emissive + fresnel for energy look, or detailed fire/magic texture with panning
- Sprites: Additive blend, velocity-stretched
- Ribbon: Additive, alpha fadeout over length

**Performance:** 1 mesh + 10-30 sprites + 1 ribbon = very manageable

---

### 2. Velocity-Aligned Sprites Only (Budget/Stylized)

**When acceptable:**
- Fast-moving projectiles (not scrutinized)
- Stylized art direction
- Distant view distances
- High projectile count scenarios

**Setup:**
- Sprite alignment: Velocity
- Stretch: Along velocity vector (1.5-3x based on speed)
- Multiple sprites with slight offset/rotation variance

**Limitation:** Side view still reveals flatness, but motion blur and speed hide it.

---

### 3. Multi-Sprite Shell (Quality Stylized)

**Structure:**
- 4-6 sprites arranged in sphere pattern around center
- Each sprite offset from center, all face camera
- Creates pseudo-volumetric appearance

**Why it works:** No single sprite carries the effect; overlapping creates depth illusion.

**Material:** Soft additive, depth fade enabled

---

### 4. Impostor / 6DOF Capture (High-End)

**Concept:** Pre-render volumetric effect from multiple angles, blend between captures at runtime.

**When to use:**
- AAA production with art pipeline support
- Repeated hero projectile (worth the capture setup)
- Need true volumetric look without volumetric cost

**Limitation:** Requires offline capture pipeline, not procedural.

---

## Technique Selection Matrix

| Requirement | Recommended Approach |
|-------------|---------------------|
| Realistic fire projectile, 360° view | Mesh core (sphere) + velocity-aligned flame sprites + ribbon trail |
| Magic orb, glowing | Mesh core + fresnel material + orbiting sprite particles |
| Laser/beam | Ribbon renderer or stretched mesh, NOT sprites |
| Arrow/physical projectile | Mesh only, no particles needed |
| Plasma ball, stylized | Multi-sprite shell or mesh + additive sprites |
| Budget, many simultaneous | Velocity-aligned sprites only, accept flatness |

---

## Beams and Lasers

**Primary technique:** Ribbon Renderer or Beam Emitter

**Structure:**
- Start point to end point
- Tessellation along length
- UV scrolling for energy movement

**Alternative:** Stretched mesh (cylinder) with scrolling material

**DO NOT USE:** Billboard sprites for beams — they cannot form a continuous line in 3D.

---

## Common Mistakes

| Mistake | Why It Fails | Fix |
|---------|--------------|-----|
| Billboard flipbook for fireball | Flat from side angles | Mesh core + sprite accents |
| POM/RM on moving projectile | View-dependent, breaks when angle changes | Mesh-based approach |
| Single large sprite | Obvious when camera orbits | Multi-sprite or mesh |
| No trail on fast projectile | Looks like teleporting dot | Add ribbon trail |
| Point light on every projectile | GPU destruction with many instances | Emissive + bloom only, light on hero only |

---

## Implementation Spec Template

**This is the level of detail required. Abstract specs are useless.**
```
EFFECT: Fire Projectile Trail
TECHNIQUE: Ribbon Core + Ember Sprites + Optional Smoke

COMPONENT 1: Trail Ribbon (NE_FireballTrail_Ribbon)
  Renderer: Ribbon
  Visual Target:
    - Color gradient over V (0=head, 1=tail):
      V=0.0: White-hot (1.0, 0.95, 0.85), Emissive=10
      V=0.2: Bright orange (1.0, 0.55, 0.1), Emissive=6
      V=0.5: Dark orange-red (0.8, 0.25, 0.05), Emissive=2
      V=0.8: Dark red-brown (0.4, 0.1, 0.02), Emissive=0.5
      V=1.0: Grey smoke (0.15, 0.12, 0.1), Alpha=0, Emissive=0
    - Width curve (ease-out): 35cm → 25cm → 12cm → 5cm → 2cm
    - Soft edges: Fresnel or gradient alpha on U edges
  Material:
    - Blend: Translucent
    - Base: Panning Perlin noise (WorldPosition, scale 0.2, speed 3.0)
    - Distortion: Subtle UV offset from noise (±0.02)
    - No texture needed if procedural matches existing M_FireballCore
  Behavior:
    - Sim space: WORLD (critical - trail stays in world as projectile moves)
    - Spawn: Continuous from emitter origin
    - Max trail length: 3.0 meters (~0.5s at 600cm/s)
    - Tessellation: 30 segments for smooth curves
    - Facing: Camera or Velocity (test both)

COMPONENT 2: Ember Sprites (NE_FireballTrail_Embers)
  Renderer: Sprite
  Visual Target:
    - Shape: Soft circular with radial gradient falloff
    - NO HARD EDGES - must use soft texture or material
    - Size at spawn: 8-20cm (random uniform)
    - Size curve: 100% → 60% → 20% over lifetime
    - Color at spawn: Bright yellow-white (1.0, 0.9, 0.5)
    - Color over life: Yellow → Orange (1,0.5,0.1) → Red (0.8,0.2,0) → Fade
    - Emissive: 6.0 at spawn → 0 at death
  Material:
    - Blend: Additive
    - Texture: Radial gradient (white center → transparent edge) OR
    - Procedural: RadialGradientExponential in material
  Behavior:
    - Spawn rate: 20/sec from trail position
    - Spawn offset: Random sphere radius 15cm (not from center)
    - Initial velocity: 50-150 cm/s random direction + 30% inherited
    - Drag: 2.5 (ember slows quickly, lingers)
    - Gravity: -150 (slight downward drift)
    - Lifetime: 0.25-0.5s (random)
    - Rotation: Random initial (0-360°), slow spin ±30°/s
    - Alignment: None (camera-facing) or Velocity (stretched)

COMPONENT 3: Smoke Wisps (NE_FireballTrail_Smoke) - OPTIONAL
  Renderer: Sprite
  Visual Target:
    - Shape: Soft, blobby, NOT circular
    - Size: 30-60cm, grows 150% over lifetime
    - Color: Dark grey (0.1, 0.1, 0.12), never black
    - Opacity: 0.3 at spawn → 0 (very subtle)
  Material:
    - Blend: Translucent (not additive)
    - Texture: Soft cloud/smoke texture or noise-based
  Behavior:
    - Spawn rate: 5/sec (sparse)
    - Spawn position: Behind ribbon tail (V=0.8+ region)
    - Velocity: Slow rise 20-50 cm/s upward + random spread
    - Drag: 1.0
    - Lifetime: 0.8-1.5s
    - Rotation: Slow random spin

PARAMETERS TO EXPOSE:
  - User.TrailIntensity: Emissive multiplier (default 1.0)
  - User.TrailLength: Ribbon lifetime multiplier (default 1.0)
  - User.EmberDensity: Spawn rate multiplier (default 1.0)
  - User.IncludeSmoke: Boolean (default false)

PERFORMANCE BUDGET:
  - Ribbon: 1 per projectile, 30 verts
  - Embers: 15-25 alive at once
  - Smoke: 5-10 alive (if enabled)
  - Total particles: 20-35 per projectile
  - 10 simultaneous projectiles: ~300 particles = very safe

VISUAL QUALITY CHECKLIST:
  - [ ] Ribbon color fades from hot to cool to smoke
  - [ ] NO white tube - must have color variation
  - [ ] Embers are SOFT circles, not squares
  - [ ] Embers drift and fall, don't follow rigidly
  - [ ] Smoke (if used) is subtle, not obscuring

WHAT BAD LOOKS LIKE (avoid):
  - Solid white/orange tube with no gradient = wrong
  - Square ember particles = missing soft texture
  - Embers that follow projectile exactly = wrong spawn space
  - Trail that moves with projectile = wrong sim space (must be World)
  - All same-size embers = no random variation

VERIFY:
  - Orbit camera around stationary projectile — should look solid from all angles
  - Fast motion should show continuous trail, no gaps
  - Slow motion — embers should drift naturally, not rigidly
```