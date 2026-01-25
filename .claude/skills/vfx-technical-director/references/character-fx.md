# Character FX: Trails, Footsteps, and Attached Effects

Effects tied to character movement and actions. Must sync with animation.

## Categories

| Category | Trigger | Attachment |
|----------|---------|------------|
| Motion trails | Movement/attack | Weapon/limb socket |
| Footsteps | Foot contact | World position |
| Blood/damage | Hit event | Hit location |
| Status effects | Gameplay event | Character socket |
| Death/spawn | State change | Character root |

---

## Motion Trails (Weapon Swings, Movement)

### Purpose
Convey speed, power, arc of motion. Essential for combat readability.

### Techniques

| Technique | Look | Best For |
|-----------|------|----------|
| Ribbon renderer | Smooth, continuous trail | Sword slashes, fast movement |
| Sprite trail | Discrete afterimages | Stylized, anime-style |
| Mesh trail | Geometric, solid | Heavy weapons, energy effects |
| Distortion trail | Heat shimmer behind object | Subtle, realistic speed |

### Ribbon Trail Setup

**Attachment:**
- Socket on weapon or limb
- Trail follows socket position each frame

**Parameters:**
- Width: Match weapon visual or exaggerate for effect
- Lifetime: 0.1-0.5s typical
- Tessellation: Higher = smoother curve
- Fade: Alpha over lifetime, taper width

**Material:**
- Usually additive or translucent
- UV along length for gradient/pattern
- Color can indicate damage type

### Timing with Animation

Trail should:
- Start slightly before swing begins (anticipation)
- Peak during fastest motion
- Fade as swing ends

Use Animation Notifies to control:
- `TrailStart`: Begin spawning
- `TrailEnd`: Stop spawning, let existing fade

### Common Problems

| Problem | Cause | Fix |
|---------|-------|-----|
| Trail lags behind | Update order | Ensure Niagara ticks after animation |
| Trail too choppy | Low tessellation or frame rate | Increase ribbon segments |
| Trail visible during idle | Spawning when shouldn't | Use anim notify control |
| Trail detaches during fast motion | Large position delta | Interpolate between frames |

---

## Footsteps

### Components

| Component | Trigger | Effect |
|-----------|---------|--------|
| Dust puff | Foot contact | Sprite burst |
| Decal | Foot contact | Footprint mark |
| Splash | Water contact | Water particles |
| Sound | Foot contact | Audio (see MetaSound) |

### Surface-Aware Footsteps

Different surfaces need different effects:

| Surface | Visual | Sound |
|---------|--------|-------|
| Dirt | Brown dust puff, footprint decal | Soft thud |
| Stone | Minimal dust, spark potential | Hard click |
| Grass | Green particles, bent grass | Soft rustle |
| Water | Splash, ripples | Splash |
| Sand | Sand spray, deep footprint | Soft crunch |
| Snow | Snow puff, deep print | Crunch |

### Implementation

1. **Physical Material**: Assign to ground surfaces
2. **Line Trace**: From foot socket down
3. **Detect Material**: Get Physical Material at hit
4. **Spawn Effect**: Select appropriate Niagara system
5. **Spawn Decal**: Select appropriate footprint

### Animation Notify Integration

- `FootstepNotify_Left` and `FootstepNotify_Right`
- Notify triggers Blueprint/Niagara
- Pass foot transform to effect

### Performance

Footsteps are frequent. Keep cheap:
- 5-15 particles per step
- Short lifetime (<0.5s)
- No lights
- Decals pooled and recycled

---

## Blood and Damage Effects

### Hit Effect Components

| Component | Purpose |
|-----------|---------|
| Blood spray | Immediate impact feedback |
| Blood decal | Persistent evidence |
| Hit flash | Brief highlight at impact |
| Damage numbers | UI feedback (if applicable) |

### Blood Spray

- Direction: Opposite of hit direction
- Color: Red (realistic) or stylized
- Amount: Scale with damage
- Particles: 10-30, velocity away from hit

### Blood Decals

- Project onto nearby surfaces
- Pool and recycle
- Fade over time or persist based on game design

### Hit Location

Effect should spawn at:
- Exact hit location (from trace result)
- OR bone position (for animation-based combat)

Pass hit normal for spray direction.

### Style Considerations

| Style | Blood Approach |
|-------|---------------|
| Realistic | Red, volumetric, disturbing |
| Stylized | Exaggerated, colorful, anime-style |
| Low violence | Impact flash only, no blood |
| Souls-like | Spray + hit pause for impact |

---

## Status Effect Visuals

### Attachment Points

| Effect | Socket | Reason |
|--------|--------|--------|
| Poison/DOT | Root or Pelvis | Full body coverage |
| Burning | Multiple sockets | Flames need distribution |
| Frozen | Mesh surface | Ice should coat character |
| Electrocuted | Skeletal mesh sample | Arcs between bones |
| Buffed | Root | Aura around character |

### Performance Warning

Characters may have multiple status effects. Plan for stacking:
- Priority system (show most important)
- Combined effects (poison + burn = different look)
- LOD: Disable status VFX on distant NPCs

---

## Death and Spawn Effects

### Death

Options based on game style:

| Style | Technique |
|-------|-----------|
| Ragdoll | Physics death, minimal VFX |
| Dissolve | Material effect, particles as dissolve happens |
| Explosion | Burst into particles/gibs |
| Fade | Alpha fade out with soul/spirit rising |
| Dramatic | Time slow, particle burst, camera focus |

### Dissolve Material

Not a particle effect — material-based:
1. Noise texture threshold animation
2. Edge emission at dissolve boundary
3. Particles emit at edge for extra detail

### Spawn / Teleport

| Technique | Look |
|-----------|------|
| Fade in | Simple alpha blend |
| Materialize | Dissolve in reverse |
| Portal | Ring effect, character emerges |
| Particles | Particles converge to form character |
| Flash | Bright flash, character appears |

---

## Socket Reference (UE Mannequin)

Common sockets for effect attachment:

| Socket | Location | Use For |
|--------|----------|---------|
| root | Ground level | Ground effects, auras |
| pelvis | Hip | Full-body centered effects |
| spine_03 | Chest | Torso effects |
| head | Head | Head-attached effects |
| hand_r / hand_l | Hands | Held item effects |
| foot_r / foot_l | Feet | Footsteps |
| weapon_r | Right hand | Weapon trails |

For custom skeletons, verify socket names.

---

## Performance Guidelines

Character FX are per-character. With many characters:

| Scenario | Max Per-Character Budget |
|----------|-------------------------|
| Player character | Full effects |
| Nearby NPC | 75% effects |
| Mid-distance NPC | 50% effects, simplified |
| Distant NPC | Status indicators only |
| Crowd/background | No VFX |

### LOD Implementation

- Use distance from camera
- Niagara Scalability settings
- Blueprint check before spawning

---

## Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| Trail not attached to socket | Spawns at origin | Verify socket binding |
| Footsteps in wrong location | Effect at origin, not foot | Pass foot transform |
| Status effects on all NPCs | Performance death | LOD system |
| Blood spray wrong direction | Always sprays same way | Use hit normal |
| Dissolve without particles | Looks cheap | Add edge particles |
| Effects persist after death | Orphaned effects | Clean up on death event |
