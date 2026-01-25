---
name: niagara-vfx-architect
description: >
  Expert VFX architect for Unreal Engine Niagara particle systems using niagaraMCP tools.
  Optimization-first approach: GPU sim preferred, LOD systems, scalability parameters.
  ALWAYS trigger on: particle effects, VFX, Niagara systems, emitters, particle spawning,
  fire, smoke, explosions, magic effects, weather systems, ambient particles, trails,
  beams, ribbons, mesh particles, GPU particles, sprite particles, collision particles,
  or phrases like "create effect", "particle system", "VFX", "spawn particles",
  "emitter", "burst effect", "continuous effect", "environmental VFX".
  Uses create_niagara_system, create_niagara_emitter, add_module_to_emitter, and related tools.
---

# Niagara VFX Architect

Expert-level VFX creation using niagaraMCP tools. **Optimization is the top priority.**

## MCP Tool Access (DO THIS FIRST)

Before any implementation, load tools via ToolSearch:
```
ToolSearch("niagara")
```

This returns available niagaraMCP tools. Tool names in this skill are shorthand — actual MCP tools have full paths like `mcp__niagaraMCP__create_niagara_system`.

**DO NOT** search Python source files or read *_mcp_server.py to find tools.

---

## Core Philosophy

1. **GPU simulation by default** — CPU only when necessary (collision events, complex logic)
2. **Scalability built-in** — Every effect must scale down gracefully
3. **LOD from the start** — Not an afterthought
4. **Profile, don't guess** — Know your particle counts and overdraw

---

## Units & Measurements Reference (CRITICAL)

**All Niagara values use Unreal Units. 1 Unreal Unit (UU) = 1 centimeter (cm).**

### Base Units

| Measurement Type | Unit | Notes |
|------------------|------|-------|
| **Distance/Position** | cm | 100 UU = 1 meter |
| **Velocity** | cm/s | centimeters per second |
| **Acceleration/Force** | cm/s² | centimeters per second squared |
| **Size (SpriteSize)** | cm | width × height in centimeters |
| **Angles** | degrees | 0-360 for rotation |
| **Time** | seconds | Lifetime, delays |

### World Scale References

| Object | Size (UU/cm) | Notes |
|--------|--------------|-------|
| UE Mannequin | ~180 cm tall | Standard human reference |
| Default floor tile | 100 × 100 cm | 1 meter grid squares |
| Door height | ~200-220 cm | Standard doorway |
| Character capsule | ~88 cm radius, ~96 cm half-height | Default Third Person |
| Small prop | 10-50 cm | Bottles, books, tools |
| Medium prop | 50-150 cm | Chairs, crates, barrels |
| Large prop | 150-300 cm | Tables, vehicles |

### Physics Values

| Parameter | Real-World Value | UE Value | Notes |
|-----------|------------------|----------|-------|
| **Earth gravity** | 9.8 m/s² | **980 cm/s²** | Use (0, 0, -980) for realistic fall |
| **Light gravity** | ~1 m/s² | **100 cm/s²** | Floaty/slow fall |
| **Moon gravity** | 1.6 m/s² | **160 cm/s²** | Low-gravity feel |
| **Walking speed** | ~1.4 m/s | **140 cm/s** | Casual walk |
| **Running speed** | ~5-7 m/s | **500-700 cm/s** | Sprint |
| **Terminal velocity (human)** | ~55 m/s | **5500 cm/s** | Max fall speed |

### Sprite Size Guidelines

| Effect Type | Typical Size (cm) | Visual Reference |
|-------------|-------------------|------------------|
| **Tiny sparks/dust** | 0.5 - 2 cm | Barely visible specks |
| **Small embers** | 2 - 5 cm | Fingertip-sized glowing dots |
| **Medium sparks** | 5 - 15 cm | Fist-sized, clearly visible |
| **Fire embers** | 1 - 5 cm | Small glowing particles |
| **Smoke puffs** | 20 - 100 cm | Basketball to large beach ball |
| **Explosion debris** | 5 - 30 cm | Fist to head-sized chunks |
| **Magic orbs** | 10 - 50 cm | Tennis ball to basketball |
| **Rain drops** | 1 - 3 cm length | Elongated streaks |
| **Snow flakes** | 0.5 - 2 cm | Small, slow-falling |
| **Leaves** | 5 - 15 cm | Hand-sized |

### Velocity Guidelines

| Effect Type | Typical Velocity (cm/s) | Notes |
|-------------|-------------------------|-------|
| **Slow drift (dust, ash)** | 5 - 30 | Gentle floating |
| **Rising embers** | 50 - 200 | Lazy upward drift |
| **Smoke rise** | 100 - 300 | Thermal lift |
| **Fire sparks** | 200 - 500 | Energetic but short-lived |
| **Explosion debris** | 500 - 2000 | Initial burst, decays quickly |
| **Bullet trails** | 5000 - 30000 | Very fast |
| **Magic projectile** | 500 - 1500 | Visible travel |
| **Rain** | 500 - 1500 (downward) | Fast falling |
| **Snow** | 50 - 150 (downward) | Slow falling |

### Force Module Values

| Module | Light | Medium | Strong | Notes |
|--------|-------|--------|--------|-------|
| **Gravity Force** | (0,0,-100) | (0,0,-500) | (0,0,-980) | Z is up/down |
| **Drag** | 0.1 - 0.5 | 0.5 - 2.0 | 2.0 - 5.0 | Higher = more resistance |
| **Vortex Force** | 20 - 50 | 50 - 150 | 150 - 500 | Swirl intensity |
| **Curl Noise Strength** | 5 - 20 | 20 - 100 | 100 - 500 | Turbulence intensity |
| **Curl Noise Frequency** | 0.5 - 1.0 | 1.0 - 3.0 | 3.0 - 10.0 | Higher = finer detail |

### Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| Gravity = -9.8 | Way too weak (should be -980) | Use -980 for Earth gravity |
| Velocity = 10 | Barely moves (10 cm/s = 0.1 m/s) | Use 100-500 for visible motion |
| SpriteSize = 100 | May be too large (1 meter!) | Check against world references |
| Drag = 10 | Particles stop almost instantly | Use 0.5-2.0 for natural decel |

### Quick Conversion Reference

```
1 cm = 1 UU (Unreal Unit)
1 m = 100 UU
1 km = 100,000 UU

1 m/s = 100 cm/s = 100 UU/s
1 km/h ≈ 27.8 cm/s ≈ 28 UU/s

Earth gravity: 9.8 m/s² = 980 cm/s² = 980 UU/s²
```

---

## Core Workflow

### Step 1: Define Effect Requirements

Before creating, clarify:
- **Effect type**: Burst, continuous, triggered, environmental?
- **Simulation**: GPU (default) or CPU (collision events, complex logic)?
- **Renderer**: Sprite, Mesh, Ribbon, Light?
- **Performance tier**: Background ambient, gameplay-critical, hero moment?
- **Scalability needs**: What degrades first? Spawn count? Lifetime? Distance culling?

### Step 2: Create System

```
create_niagara_system(
  system_name="NS_EffectName",
  asset_path="/Game/Effects/"
)
```

**Naming conventions:**
- `NS_` — Niagara System
- `NE_` — Niagara Emitter (standalone/template)

### Step 3: Create Emitter(s)

```
create_niagara_emitter(
  emitter_name="NE_EmitterName",
  asset_path="/Game/Effects/Emitters/",
  sim_target="GPUComputeSim"  // or "CPUSim"
)
```

**Always specify sim_target explicitly.** Default to GPU.

### Step 4: Build Module Stack

Follow this order for clean, debuggable emitters:

1. **Emitter State** — Lifecycle, loop behavior
2. **Spawn** — Rate, burst, spawn conditions
3. **Particle Spawn** — Initial values (position, velocity, size, color)
4. **Particle Update** — Per-frame logic (forces, drag, color over life)
5. **Renderer** — Visual output configuration

### Step 5: Add to System & Compile

```
add_emitter_to_system(system_path, emitter_path)
compile_niagara_asset(asset_path)
```

Always compile after structural changes.

---

## MCP Tool Reference

### Asset Creation

| Tool | Purpose |
|------|---------|
| `create_niagara_system` | Create new system asset |
| `create_niagara_emitter` | Create new emitter asset |
| `add_emitter_to_system` | Attach emitter to system |
| `get_niagara_metadata` | Inspect existing asset structure |
| `compile_niagara_asset` | Compile after changes |

### Module Stack Building

| Tool | Purpose |
|------|---------|
| `search_niagara_modules` | Find available modules by name/category |
| `add_module_to_emitter` | Add module to specific stack group |
| `set_module_input` | Configure module input values |

### Parameters & Data

| Tool | Purpose |
|------|---------|
| `add_niagara_parameter` | Add user/system parameter |
| `set_niagara_parameter` | Set parameter value |
| `add_data_interface` | Add data interface (mesh, skeletal, collision) |
| `set_data_interface_property` | Configure data interface |

### Rendering

| Tool | Purpose |
|------|---------|
| `add_renderer` | Add renderer to emitter |
| `set_renderer_property` | Configure renderer settings |

### Level Integration

| Tool | Purpose |
|------|---------|
| `spawn_niagara_actor` | Spawn system in level for testing |

---

## Optimization Rules (PRIORITY)

### Performance Budgets

| Effect Type | Max Particles | Overdraw Target |
|-------------|---------------|-----------------|
| Ambient/Background | 100-500 | < 2x |
| Gameplay (frequent) | 500-2000 | < 4x |
| Hero/Cinematic | 2000-10000 | < 8x |
| One-shot burst | Brief spike OK | Settle quickly |

### GPU vs CPU Decision Tree

**Use GPU (default):**
- High particle counts (>500)
- Simple update logic
- No collision events needed
- No per-particle Blueprint communication

**Use CPU only when:**
- Collision events trigger gameplay
- Complex conditional logic per particle
- Need to read particle data in Blueprint
- Very low counts where GPU overhead not worth it

### Scalability Parameters

Every effect MUST expose:
```
User.SpawnRateMultiplier    // Scale spawn rate
User.MaxParticles           // Hard cap
User.EffectScale            // Overall size multiplier  
User.LODDistance            // Cull/simplify distance
```

### LOD Strategy

| Distance | Action |
|----------|--------|
| Near | Full quality |
| Medium | 50% spawn rate, simpler modules |
| Far | 25% spawn rate, disable sub-emitters |
| Cull | Kill system entirely |

Implement via `Emitter.LODDistanceFraction` and `Scalability` settings.

### Overdraw Reduction

1. **Smaller particles** — Reduce size, increase count if needed
2. **Shorter lifetimes** — Particles should die, not fade forever
3. **Depth fade** — Soft intersection with scene
4. **Cutout/masked sprites** — Avoid full-quad alpha
5. **Limit ribbons** — Expensive overdraw, use sparingly

### Anti-Patterns (AVOID)

- Spawning thousands of CPU particles
- Ribbons with high tessellation + long lifetime
- Particle lights (use sparingly, max 2-3 per effect)
- Collision every frame (use fixed intervals)
- Translucent mesh particles (heavy overdraw)
- No scalability parameters
- No LOD consideration

---

## Module Stack Groups

When using `add_module_to_emitter`, specify the correct group:

| Group | Purpose | Common Modules |
|-------|---------|----------------|
| `EmitterSpawn` | One-time emitter setup | — |
| `EmitterUpdate` | Per-frame emitter logic | Emitter State, Spawn Rate |
| `ParticleSpawn` | Initial particle values | Initialize Particle, Position, Velocity, Size, Color |
| `ParticleUpdate` | Per-frame particle logic | Forces, Drag, Scale/Color Over Life, Curl Noise |
| `EventHandler` | Response to events | Death events, collision events |

---

## Common Module Patterns

### Spawn Rate Control
```
Module: "Spawn Rate"
Group: EmitterUpdate
Inputs:
  SpawnRate = 50.0  // or bind to User.SpawnRateMultiplier
```

### Burst Spawn
```
Module: "Spawn Burst Instantaneous"
Group: EmitterUpdate
Inputs:
  SpawnCount = 100
  SpawnProbability = 1.0
```

### Initialize Particle
```
Module: "Initialize Particle"
Group: ParticleSpawn
Inputs:
  Lifetime = RandomRange(1.0, 2.0)
  Mass = 1.0
  SpriteSize = (10.0, 10.0)
  Color = (1.0, 0.5, 0.0, 1.0)
```

### Velocity Cone
```
Module: "Add Velocity in Cone"
Group: ParticleSpawn
Inputs:
  ConeAngle = 30.0
  VelocityStrength = 500.0
```

### Gravity Force
```
Module: "Gravity Force"
Group: ParticleUpdate
Inputs:
  Gravity = (0, 0, -980)
```

### Scale Color Over Life
```
Module: "Scale Color"
Group: ParticleUpdate
Inputs:
  ColorScale = Curve  // Alpha fade out
```

---

## Renderer Types

| Renderer | Use Case | Performance |
|----------|----------|-------------|
| Sprite | Default particles | Cheap |
| Mesh | Debris, complex shapes | Medium-Heavy |
| Ribbon | Trails, beams, magic streams | Heavy (overdraw) |
| Light | Illumination | Very Heavy (max 2-3) |

### Sprite Renderer Settings
```
Material = MI_ParticleSprite
Alignment = Velocity (for streaks) or Camera (for puffs)
SubImageIndex = For flipbooks
SortMode = ByDistance or Dynamic (if needed)
```

### Mesh Renderer Settings
```
ParticleMesh = SM_Debris
Material = MI_DebrisMaterial
FacingMode = Velocity (tumbling) or Default
```

---

## Data Interfaces

### Static Mesh DI
Sample positions/normals from mesh surface.
```
add_data_interface(
  data_interface_type="StaticMesh",
  data_interface_name="SourceMesh"
)
set_data_interface_property(
  property_name="DefaultMesh",
  property_value="/Game/Meshes/SM_Source"
)
```

### Skeletal Mesh DI
Attach particles to animated skeleton.
```
add_data_interface(
  data_interface_type="SkeletalMesh",
  data_interface_name="CharacterMesh"
)
```

### Collision Query DI
Enable particle-world collision (CPU only).
```
add_data_interface(
  data_interface_type="CollisionQuery",
  data_interface_name="SceneCollision"
)
```

---

## Incremental Workflow

Follow unreal-mcp-architect principles:

1. **Create system** — Verify it exists
2. **Create emitter** — Specify sim target
3. **Add modules one at a time** — Verify each addition
4. **Configure inputs** — Test values
5. **Add renderer** — Visual verification
6. **Add to system** — Compile
7. **Spawn in level** — Test performance

**After each step:** "Finished [step]. Ready for testing."

---

## Effect Patterns

See `references/effect-patterns.md` for complete recipes:
- Fire/flame effects
- Smoke plumes
- Explosions with debris
- Magic projectiles with trails
- Weather (rain, snow, dust)
- Ambient environmental particles
- Impact effects
- Beam/laser effects

## Troubleshooting

See `references/troubleshooting.md` for:
- Effect not visible
- Performance issues diagnosis
- Module not working
- Compilation errors
- Collision not detecting

---

## Pre-Flight Checklist

Before finalizing any effect:

- [ ] Sim target explicitly set (GPU preferred)
- [ ] Scalability parameters exposed
- [ ] LOD behavior defined
- [ ] Max particle count reasonable for effect type
- [ ] Lifetime not excessive (particles die)
- [ ] Overdraw considered (small particles, depth fade)
- [ ] No unnecessary CPU features on GPU emitter
- [ ] Compiled successfully
- [ ] Tested at target frame rate
