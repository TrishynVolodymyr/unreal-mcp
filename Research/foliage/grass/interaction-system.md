# Girardot Advanced Grass Interaction System - Technical Breakdown

**Source:** https://www.youtube.com/watch?v=BOEd3ZYilYM
**Author:** Girardot (YouTube / Patreon / Gumroad)
**Type:** UE plugin (Blueprint Actor Component + Material Functions)

---

## System Overview

A render-target-based grass interaction system that supports:
- Player trail interaction (springy bending with delay)
- Nearby actor interaction (rolling spheres, projectiles, etc.)
- Custom spawned effects (explosions, directional forces)
- Physics-driven (velocity-based bending direction)

Everything is baked into 2D render targets -- the system operates in 2D XY space only, with no Z-axis knowledge.

---

## Architecture

### Components

1. **Blueprint Actor Component** -- added to the player pawn
2. **Sphere Volume** on the pawn -- generates overlap events with nearby actors; must have a specific tag (configurable in component settings) so the system auto-picks it up
3. **Blueprint Interface** -- implemented on both pawn and interacting actors; provides a single function called frequently to send interaction context (velocity, radius, alpha)
4. **Two Render Targets** -- both 128x128 pixels, super low resolution for performance
5. **Material Functions** -- provided by the system for sampling render targets and computing grass rotation

### Data Flow

```
Actor/Pawn velocity + position
  -> Blueprint Interface function (called frequently per actor)
    -> Interaction context: XY location, XY velocity, radius, alpha
      -> Particle pool system (fixed pools per type)
        -> Material Instance updates (timestamp, radius, XY loc, XY vel)
          -> Draw onto Render Targets (material draws sphere mask + oscillating curve)
            -> Grass material samples render targets
              -> World Position Offset (rotation around pivot)
```

---

## Render Target Details

### Render Target 1: Base Interaction
- **Red channel:** X interaction velocity
- **Green channel:** Y interaction velocity
- **Blue channel:** Interaction mask (general purpose -- e.g., drive emissive on grass material)

### Render Target 2: Delayed Interaction
- **Red channel:** X delayed velocity
- **Green channel:** Y delayed velocity
- Purpose: Creates the springy bending effect by giving tips different motion than roots

### Resolution
- Both are **128x128 pixels**
- Increasing size costs more GPU but allows smaller interaction radii
- At 128px, very tiny interactions (e.g., individual foot prints) may be sub-pixel and break down

---

## Grass Material Integration

### With Pivot Points (Recommended)
- Grass pivot points must be **baked into UVs**
- System-provided material function computes rotation axis and angle
- **Roots** sample the base XY velocity render target (RT1)
- **Tips** sample the delayed velocity render target (RT2)
- The difference between root and tip sampling creates the spring/bend effect
- Plug output into **World Position Offset**

### Without Pivot Points (Simplified)
- A simplified material function is provided
- Only does **translation**, not rotation
- Doesn't look as good but works without UV-baked pivots

---

## Particle Pool System

### Player Trail Pool
- **20 particles** dedicated to the player
- Spawned via a **timer** that ticks based on pool size and particle lifetime
- Example: ticks every **0.2 seconds**
- Each tick: get owner -> call interface function -> get interaction context -> spawn trail effect
- Finds first available particle index, updates material instance with: timestamp, radius, XY location, XY velocity

### Actor Trail Pool
- **48 particles** dedicated to nearby actors
- Same material setup but more particles = more pixel instructions
- **This is the main limitation:** if all 48 particles are alive when a new one needs to spawn, actors stop interacting until a particle dies
- `actor_trail_rate` setting: rate at which trail particles are spawned per second per actor

### Effect Pool
- **10 particles per pool** (player and actors each have their own)
- Way more complex to render than trail particles (pixel instructions rise quickly)
- Used for custom effects (explosions, directional blasts)

---

## Material Drawing Technique

For each particle drawn onto the render target:
1. Draw a **sphere mask** around the XY location
2. Multiply the given XY velocity with an **oscillating curve**
3. Curve is sampled using: the particle's timestamp, current time, and expected lifetime -> compute **normalized time**
4. This oscillation creates the spring/bounce effect

Trail particles: ~300 pixel instructions per particle, drawn onto 128x128 RT -- actually fast.
Actor particles: ~700 pixel instructions per particle (48 particles) -- still fast on small RT.

---

## Blueprint Interface Function

Called frequently on both pawns and actors. Must be kept **simple** (no heavy logic -- blueprint performance risk).

### Parameters Sent
| Parameter | Description |
|-----------|-------------|
| XY Location | 2D position of the interaction |
| XY Velocity | 2D velocity driving bend direction |
| Radius | How large the interaction area is |
| Alpha | How strong the interaction is (0-1) |

### Example: Player Pawn Implementation
1. Check pawn velocity -- if moving enough, notify system
2. Send velocity as interaction direction
3. Use a **cheap line trace** downward to detect distance from ground
4. Drive alpha based on ground distance (so jumping = no grass interaction below)

### Example: Rolling Ball Actor
1. Just implement the interface
2. Return velocity, radius, alpha
3. Actor must be configured to generate overlap events with the pawn's sphere volume

---

## Custom Effects System

### Spawning Effects
- Call the component's effect function from Blueprint
- Example: left click -> call component function -> grass reacts in front of player
- Example: grenade -> on spawn, get player character -> get grass interaction component -> call effect function

### Effect Parameters
| Parameter | Description |
|-----------|-------------|
| Pool selection (bool) | Player pool or actor pool (10 particles each) |
| Location (XYZ) | Z only used for debug view; system only uses XY |
| Force | Strength of the effect |
| Radius | Area of effect |
| Direction (vector) | e.g., forward vector of the character |
| Directionality (0-1) | 0 = radial effect (grenade), 1 = fully directional (all grass bends toward direction) |
| Direction masking | Creates a cone -- grass behind the source is less affected |
| Directional falloff | Tightens the cone mask |

Direction serves dual purpose:
1. Drive the effect's bend direction
2. Mask the effect in the given direction (cone)

---

## Component Settings

| Setting | Description | Default/Notes |
|---------|-------------|---------------|
| Use Delay Pass (bool) | Whether to draw delayed velocities onto RT2 | If false, also set false in grass material function |
| Sphere Volume Tag | Tag for the overlap sphere on pawn | Must match between component and sphere volume |
| Capture Size (cm) | How far interaction effects are visible | Depends on view distance; limited by RT resolution |
| Actor Trail Rate | Trail particles spawned per second per actor | - |
| Particle Lifetime | How long each particle lives | Longer = fewer spawns possible before pool exhaustion |
| Debug View (bool) | Shows interaction radius, velocity lines, particle indices | - |

---

## Performance

### Test Hardware
- **GPU:** GTX 970
- **CPU:** First-gen Threadripper
- (Author notes: "nothing crazy for gaming")

### GPU Cost
| Component | Cost |
|-----------|------|
| Render Target Pass 1 (base velocities) | **~0.2 ms** |
| Render Target Pass 2 (delayed velocities) | **~0.1 ms** |
| Grass material sampling | **Tiny** (just sampling 1-2 render targets) |
| **Total GPU** | **~0.3 ms** |

### CPU Cost
- Blueprint tick: draws render targets + updates material parameter collection
- Two timers at low tick rate for trail particles
- Author considers CPU cost "not bad"

### Scaling Considerations
- You can add more particles to actor pool to support more actors (cost scales linearly on small RT)
- Disabling the delay pass saves ~0.1ms GPU
- Increasing RT resolution increases GPU cost proportionally
- The 128x128 RT is the key performance win -- all material drawing is cheap at this resolution

---

## Limitations

1. **2D only** -- no Z-axis awareness; must fake height with line traces driving alpha
2. **Limited actor count** -- 48-particle pool for actors; high actor counts exhaust the pool
3. **Render target resolution** -- 128x128 means very small interactions may be sub-pixel
4. **No per-foot precision** -- individual foot interaction would need higher RT resolution
5. **Overlap sphere required** -- only actors within the pawn's sphere volume are tracked
6. **Blueprint interface overhead** -- called frequently per actor; must stay lightweight

---

## Alternative Approaches Mentioned

- **Per-actor particle system + Scene Capture:** Each actor spawns its own Niagara system leaving trail particles, captured by a scene capture component. Removes the fixed particle pool limitation but scene captures are costly -- likely less performant overall.
- **Simpler system (author's previous release):** Limited to player character only, no actor interaction, no spring effect.

---

## Key Takeaways for Implementation

1. The spring effect comes from sampling RT1 at roots and RT2 (delayed) at tips -- the temporal difference creates the bend
2. The oscillating curve in the material is what gives the "weight" and spring-back feel
3. 128x128 render targets are surprisingly sufficient for most use cases
4. Fixed particle pools are a deliberate performance choice -- predictable cost
5. The system is essentially a 2D stamp system: stamp velocity circles onto RTs, sample in grass material
6. Pivot points baked into UVs are critical for proper rotation-based bending (vs. just translation)
