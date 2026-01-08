# Effect Patterns Reference

Concrete recipes for common VFX effects. Each pattern shows exact modules, groups, and input configurations.

---

## Fire / Flame Effect

Continuous burning fire with heat distortion optional.

### System Structure
```
NS_Fire
├── NE_FlameCore      (GPU, main flames)
├── NE_Embers         (GPU, rising sparks)
└── NE_Smoke          (GPU, optional smoke layer)
```

### NE_FlameCore Modules

**EmitterUpdate:**
```
Module: "Emitter State"
  LoopBehavior = Infinite

Module: "Spawn Rate"
  SpawnRate = 30
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(0.3, 0.6)
  SpriteSize = RandomRange((20, 30), (40, 60))
  Color = (1.0, 0.6, 0.1, 1.0)

Module: "Sphere Location"
  SphereRadius = 20
  
Module: "Add Velocity"
  Velocity = (0, 0, RandomRange(100, 200))
```

**ParticleUpdate:**
```
Module: "Gravity Force"
  Gravity = (0, 0, 50)  // Slight upward (fire rises)

Module: "Drag"
  DragCoefficient = 2.0

Module: "Scale Sprite Size"
  ScaleCurve = 1.0 → 1.5 → 0.5  // Grow then shrink

Module: "Scale Color"
  ColorScale = Curve
    0.0: (1.0, 0.8, 0.3, 1.0)   // Bright yellow
    0.5: (1.0, 0.3, 0.0, 0.8)   // Orange
    1.0: (0.3, 0.0, 0.0, 0.0)   // Fade to dark red
```

**Renderer:**
```
Type: Sprite
Material: MI_Fire_Additive
Alignment: Velocity
SubUVAnimation: 4x4 flipbook
BlendMode: Additive
```

### NE_Embers Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 15
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(1.0, 2.0)
  SpriteSize = RandomRange(2, 5)
  Color = (1.0, 0.5, 0.0, 1.0)

Module: "Add Velocity in Cone"
  ConeAngle = 20
  VelocityStrength = RandomRange(50, 150)
```

**ParticleUpdate:**
```
Module: "Curl Noise Force"
  NoiseStrength = 30
  
Module: "Gravity Force"
  Gravity = (0, 0, 50)

Module: "Scale Color"
  AlphaFadeOut at 80%
```

**Renderer:**
```
Type: Sprite
Material: MI_Ember_Additive
SortMode: None (cheap)
```

### Optimization Notes
- Fire is spawn-rate sensitive — scale SpawnRate first for LOD
- Use flipbook animation, not spawn count for visual complexity
- Embers can be culled at medium distance

---

## Smoke Plume

Rising smoke with turbulence.

### System Structure
```
NS_Smoke
└── NE_SmokePuff      (GPU)
```

### NE_SmokePuff Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 10

Module: "Emitter State"
  LoopBehavior = Infinite
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(3.0, 5.0)
  SpriteSize = RandomRange((50, 50), (80, 80))
  Color = (0.3, 0.3, 0.3, 0.5)

Module: "Cylinder Location"
  CylinderRadius = 30
  CylinderHeight = 10

Module: "Add Velocity"
  Velocity = (0, 0, RandomRange(30, 60))
```

**ParticleUpdate:**
```
Module: "Curl Noise Force"
  NoiseStrength = 50
  NoiseFrequency = 0.5

Module: "Drag"
  DragCoefficient = 0.5

Module: "Scale Sprite Size"
  ScaleCurve = 1.0 → 3.0  // Expand as rises

Module: "Scale Color"
  AlphaCurve = 0.5 → 0.3 → 0.0  // Gentle fadeout
```

**Renderer:**
```
Type: Sprite
Material: MI_Smoke_Lit (use lit for shadows)
Alignment: Camera
SortMode: ByDistance
```

### Optimization Notes
- Smoke overdraw is expensive — keep particles large but few
- Lit smoke costs more but looks better
- LOD: reduce lifetime at distance

---

## Explosion (Burst)

One-shot explosion with flash, debris, and smoke.

### System Structure
```
NS_Explosion
├── NE_Flash          (GPU, instant bright flash)
├── NE_Fireball       (GPU, expanding fire)
├── NE_Debris         (CPU if collision, else GPU)
├── NE_Shockwave      (GPU, ground ring)
└── NE_SmokeTrail     (GPU, lingering smoke)
```

### NE_Flash Modules

**EmitterUpdate:**
```
Module: "Spawn Burst Instantaneous"
  SpawnCount = 1
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = 0.1
  SpriteSize = (500, 500)
  Color = (1.0, 0.9, 0.7, 1.0)
```

**ParticleUpdate:**
```
Module: "Scale Sprite Size"
  ScaleCurve = 1.0 → 2.0

Module: "Scale Color"
  AlphaCurve = 1.0 → 0.0
```

**Renderer:**
```
Type: Sprite
Material: MI_Flash_Additive
Alignment: Camera
```

### NE_Fireball Modules

**EmitterUpdate:**
```
Module: "Spawn Burst Instantaneous"
  SpawnCount = 50
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(0.3, 0.6)
  SpriteSize = RandomRange((30, 30), (60, 60))

Module: "Sphere Location"
  SphereRadius = 20

Module: "Add Velocity"
  Velocity = RandomUnitVector * RandomRange(200, 500)
```

**ParticleUpdate:**
```
Module: "Drag"
  DragCoefficient = 5.0

Module: "Scale Color"
  Curve: Bright orange → Dark red → Black with alpha fade
```

### NE_Debris Modules (CPU for collision)

**EmitterUpdate:**
```
Module: "Spawn Burst Instantaneous"
  SpawnCount = 20
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(1.0, 2.0)
  Mass = 1.0

Module: "Add Velocity in Cone"
  ConeAngle = 60
  VelocityStrength = RandomRange(500, 1000)
```

**ParticleUpdate:**
```
Module: "Gravity Force"
  Gravity = (0, 0, -980)

Module: "Collision" (requires CPU + CollisionQuery DI)
  CollisionMode = Kill or Bounce
  Restitution = 0.3
```

**Renderer:**
```
Type: Mesh
ParticleMesh: SM_DebrisChunk
FacingMode: Velocity (tumble)
```

### Optimization Notes
- Explosion is one-shot — spike OK but settle fast
- Debris collision is expensive — limit count, use GPU if no collision needed
- Flash is single particle — cheap impact for visual punch

---

## Magic Projectile with Trail

Glowing orb with ribbon trail.

### System Structure
```
NS_MagicProjectile
├── NE_Core           (GPU, central glow)
├── NE_Trail          (GPU, ribbon trail)
└── NE_Sparkles       (GPU, orbiting particles)
```

### NE_Core Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 1  // Single particle, respawns
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = 0.1  // Short, constantly respawning
  SpriteSize = (30, 30)
  Color = (0.3, 0.5, 1.0, 1.0)  // Blue glow
```

**Renderer:**
```
Type: Sprite
Material: MI_Glow_Additive
```

### NE_Trail Modules (Ribbon)

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 60  // Smooth ribbon
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = 0.5
  RibbonWidth = 20

Module: "Inherit Source Movement"
  // Particles follow system movement
```

**ParticleUpdate:**
```
Module: "Scale Ribbon Width"
  WidthCurve = 1.0 → 0.0  // Taper

Module: "Scale Color"
  AlphaCurve = 1.0 → 0.0
```

**Renderer:**
```
Type: Ribbon
Material: MI_Trail_Additive
FacingMode: Screen
TessellationMode: Custom (keep low: 2-4)
```

### NE_Sparkles Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 20
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(0.2, 0.4)
  SpriteSize = RandomRange(2, 5)

Module: "Torus Location"
  TorusRadius = 15
```

**ParticleUpdate:**
```
Module: "Orbit"
  OrbitRadius = 15
  OrbitSpeed = 360 degrees/sec

Module: "Scale Color"
  Sparkle flicker via noise
```

### Optimization Notes
- Ribbon tessellation is key — keep low
- For fast projectiles, increase spawn rate for smooth trail
- Sparkles can be entirely culled at distance

---

## Rain (Weather)

Large-area rain with splash impacts.

### System Structure
```
NS_Rain
├── NE_Raindrops      (GPU, falling drops)
└── NE_Splashes       (GPU, ground impacts)
```

### NE_Raindrops Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 500  // Scalable via User param

Module: "Emitter State"
  LoopBehavior = Infinite
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = 1.0
  SpriteSize = (1, 20)  // Elongated streak
  Color = (0.7, 0.8, 0.9, 0.3)

Module: "Box Location"
  BoxExtents = (2000, 2000, 0)  // Large spawn area
  Offset = (0, 0, 500)  // Above ground

Module: "Add Velocity"
  Velocity = (WindX, WindY, -1500)  // Fast fall + wind
```

**ParticleUpdate:**
```
Module: "Gravity Force"
  Gravity = (0, 0, -980)
```

**Renderer:**
```
Type: Sprite
Material: MI_Rain_Translucent
Alignment: Velocity (streaks)
SortMode: None
```

### NE_Splashes Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 100
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = 0.15
  SpriteSize = (5, 5)

Module: "Box Location"
  BoxExtents = (2000, 2000, 0)
  // Spawn at ground level

Module: "Add Velocity in Cone"
  ConeAngle = 80
  VelocityStrength = RandomRange(20, 50)
```

**ParticleUpdate:**
```
Module: "Scale Sprite Size"
  Quick expand then shrink
```

### Optimization Notes
- Rain is spawn-count heavy — primary LOD lever
- No sorting on rain (too many particles)
- Splashes optional at distance
- Use fixed-size spawn box, not infinite

---

## Impact Effect (Hit/Footstep)

One-shot impact burst.

### System Structure
```
NS_Impact
├── NE_Dust           (GPU, dust puff)
└── NE_Debris         (GPU, small chunks)
```

### NE_Dust Modules

**EmitterUpdate:**
```
Module: "Spawn Burst Instantaneous"
  SpawnCount = 10
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(0.5, 1.0)
  SpriteSize = RandomRange((10, 10), (30, 30))
  Color = (0.6, 0.5, 0.4, 0.5)  // Dust color

Module: "Hemisphere Location"
  Radius = 10
  HemisphereDirection = Up

Module: "Add Velocity in Cone"
  ConeAngle = 60
  VelocityStrength = RandomRange(50, 150)
```

**ParticleUpdate:**
```
Module: "Drag"
  DragCoefficient = 3.0

Module: "Scale Sprite Size"
  Expand over life

Module: "Scale Color"
  AlphaFadeOut
```

### NE_Debris Modules

**EmitterUpdate:**
```
Module: "Spawn Burst Instantaneous"
  SpawnCount = 5
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(0.3, 0.6)
  SpriteSize = RandomRange(2, 5)

Module: "Add Velocity in Cone"
  ConeAngle = 45
  VelocityStrength = RandomRange(100, 300)
```

**ParticleUpdate:**
```
Module: "Gravity Force"
  Gravity = (0, 0, -980)
```

### Optimization Notes
- Impact effects fire frequently — keep particle counts very low
- Auto-destroy system after all particles dead
- Pool these effects for rapid reuse

---

## Beam / Laser

Continuous beam between two points.

### System Structure
```
NS_Beam
├── NE_BeamCore       (GPU, main beam)
└── NE_BeamGlow       (GPU, soft glow around beam)
```

### NE_BeamCore Modules (Ribbon-based beam)

**Parameters:**
```
User.BeamStart = Vector3
User.BeamEnd = Vector3
User.BeamWidth = Float (default 10)
```

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 2  // Just need start and end points
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = 0.1
  RibbonWidth = User.BeamWidth
  Position = Lerp(User.BeamStart, User.BeamEnd, NormalizedParticleIndex)
```

**Renderer:**
```
Type: Ribbon
Material: MI_Beam_Additive
TessellationMode: Low
```

### Alternative: Mesh Beam

For more control, use a stretched cylinder mesh:

```
Renderer: Mesh
ParticleMesh: SM_Cylinder
Scale: (BeamWidth, BeamWidth, BeamLength)
Orientation: Point at target
```

### Optimization Notes
- Beams are cheap if ribbon tessellation is low
- For pulsing effects, animate material, not spawn rate
- End hit points should be separate emitter (impact sparks)

---

## Ambient Particles (Dust Motes, Fireflies)

Environmental atmosphere particles.

### System Structure
```
NS_AmbientDust
└── NE_DustMotes      (GPU)
```

### NE_DustMotes Modules

**EmitterUpdate:**
```
Module: "Spawn Rate"
  SpawnRate = 20

Module: "Emitter State"
  LoopBehavior = Infinite
```

**ParticleSpawn:**
```
Module: "Initialize Particle"
  Lifetime = RandomRange(5.0, 10.0)
  SpriteSize = RandomRange(1, 3)
  Color = (1.0, 1.0, 1.0, 0.3)

Module: "Box Location"
  BoxExtents = (500, 500, 200)  // Around player
```

**ParticleUpdate:**
```
Module: "Curl Noise Force"
  NoiseStrength = 10
  NoiseFrequency = 0.3

Module: "Scale Color"
  Gentle alpha variation
```

**Renderer:**
```
Type: Sprite
Material: MI_DustMote
SortMode: None (too many)
```

### Optimization Notes
- Ambient effects must be VERY cheap
- Long lifetimes = fewer spawns needed
- Cull entirely at far distance
- Consider camera-relative spawn box

---

## Quick Reference: Spawn Patterns

| Pattern | Module | Use Case |
|---------|--------|----------|
| Continuous | Spawn Rate | Fire, smoke, trails |
| One-shot | Spawn Burst Instantaneous | Explosions, impacts |
| Timed burst | Spawn Burst Instantaneous + Duration | Muzzle flash |
| Per-distance | Spawn Per Unit | Tire tracks, footprints |

## Quick Reference: Location Modules

| Module | Shape | Use Case |
|--------|-------|----------|
| Sphere Location | Sphere | Explosions, magic |
| Box Location | Box | Rain, ambient |
| Cylinder Location | Cylinder | Fire, jets |
| Torus Location | Ring | Magic circles |
| Mesh Surface Location | Mesh | Dissolve effects |
| Skeletal Mesh Location | Bones | Character effects |
