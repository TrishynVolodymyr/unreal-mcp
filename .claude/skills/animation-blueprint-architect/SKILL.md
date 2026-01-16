---
name: animation-blueprint-architect
description: >
  Architect Animation Blueprints, state machines, blend spaces, and montages in Unreal Engine
  using MCP tools. Covers locomotion systems, combat animations, layered blending, and
  transition logic. ALWAYS trigger on: animation blueprint, ABP, state machine, blend space,
  blend space 1D, blend space 2D, animation montage, montage, anim notify, animation state,
  transition rule, locomotion, animation layer, pose blending, additive animation, aim offset,
  or any request involving character animation systems.
---

# Animation Blueprint Architect

Design and implement Animation Blueprints using MCP tools.

## Core Concepts

| Component | Purpose |
|-----------|---------|
| Animation Blueprint (ABP) | Controls skeletal mesh animation logic |
| State Machine | Manages animation states and transitions |
| Blend Space | Blends animations based on parameters |
| Montage | One-shot/layered animations (attacks, emotes) |
| Anim Notify | Events triggered during animation |

## Naming Conventions

| Asset Type | Prefix | Example |
|------------|--------|---------|
| Animation Blueprint | `ABP_` | `ABP_PlayerCharacter` |
| Blend Space 1D | `BS1D_` | `BS1D_WalkRun` |
| Blend Space 2D | `BS2D_` | `BS2D_Locomotion` |
| Montage | `AM_` | `AM_Attack_Light` |
| Animation Sequence | `AS_` | `AS_Idle` |
| Aim Offset | `AO_` | `AO_AimPitch` |

## State Machine Architecture

### Basic Locomotion State Machine

```
[Entry] → [Idle] ←→ [Walk/Run] ←→ [Jump] → [Fall] → [Land] → [Idle]
              ↓           ↓
          [Crouch] ←→ [Crouch Walk]
```

### State Design Rules

1. **Entry state** - Always Idle or a neutral pose
2. **One responsibility** - Each state handles one animation concern
3. **Clean transitions** - Explicit conditions, no "remaining time < 0.1" hacks
4. **Blend durations** - 0.1-0.2s for snappy, 0.3-0.5s for smooth

### Transition Rules

| From | To | Condition |
|------|-----|-----------|
| Idle | Walk/Run | `Speed > 5.0` |
| Walk/Run | Idle | `Speed < 5.0` |
| Any | Jump | `bIsJumping AND bIsOnGround` |
| Jump | Fall | `Time in state > JumpApex` |
| Fall | Land | `bIsOnGround` |
| Land | Idle | `Animation complete` |

## Blend Space Setup

### 1D Blend Space (Speed-based)

For walk/run blending on single axis:

```
Parameter: Speed (0 → 600)
Samples:
  0   → AS_Idle
  150 → AS_Walk
  300 → AS_Jog  
  600 → AS_Sprint
```

### 2D Blend Space (Directional Locomotion)

For omnidirectional movement:

```
Horizontal: Direction (-180 → 180)
Vertical: Speed (0 → 600)

Grid Layout:
         -180    -90     0      90     180
  600    Sprint  Sprint  Sprint Sprint Sprint
         BWD_L   Left    FWD    Right  BWD_R
  300    Jog_BWD Jog_L   Jog    Jog_R  Jog_BWD
  150    Walk_BW Walk_L  Walk   Walk_R Walk_BW
    0    Idle    Idle    Idle   Idle   Idle
```

### Aim Offset (Upper Body)

For aiming without affecting legs:

```
Horizontal: Yaw (-90 → 90)
Vertical: Pitch (-90 → 90)

Additive type: Mesh Space
Base pose: Idle (arms neutral)
```

## Montage Architecture

### Slot Groups

| Slot | Usage |
|------|-------|
| `DefaultSlot` | Full body actions |
| `UpperBody` | Attacks, reloads (legs continue) |
| `Face` | Facial expressions |
| `Additive` | Hit reactions, flinches |

### Montage Sections

For combo attacks:

```
AM_Attack_Combo:
  [Start] → [Attack1] → [Attack2] → [Attack3] → [End]
                ↓           ↓           ↓
            (branch)    (branch)    (branch)
              to End      to End      to End
```

### Anim Notifies

| Notify Type | Use Case |
|-------------|----------|
| `ANS_` (State) | Weapon trail, invincibility frames |
| `AN_` (Single) | Sound, particle spawn, damage window |

Common notifies:
- `AN_FootstepLeft` / `AN_FootstepRight`
- `AN_WeaponSwing`
- `ANS_DamageWindow` (start/end)
- `AN_ComboInputWindow`

## Common Patterns

### Layered Animation (Upper/Lower Body Split)

```
[Locomotion State Machine]
         ↓
    [Layered Blend Per Bone]
         ├── Base: Legs (from locomotion)
         └── Blend: Upper body (from montage slot)
              Bone: spine_01
              Blend Depth: 1.0
```

### Additive Hit Reactions

```
[Base Pose]
     ↓
[Apply Additive]
     ├── Base: Current animation
     └── Additive: Hit reaction (mesh space additive)
          Alpha: Driven by curve/variable (0→1→0)
```

### IK Foot Placement

```
[Final Pose]
     ↓
[Two Bone IK]
     ├── IK Bone: foot_l
     ├── Effector: IK_foot_l (virtual bone)
     └── Alpha: Foot IK Alpha variable
```

## MCP Tool Workflow

### Create Animation Blueprint

```
1. create_animation_blueprint(
     name="ABP_Character",
     skeleton="/Game/Characters/Skeleton"
   )

2. create_state_machine(
     blueprint="ABP_Character",
     name="Locomotion"
   )

3. add_state(
     state_machine="Locomotion",
     name="Idle",
     animation="/Game/Anims/AS_Idle"
   )

4. add_transition(
     from_state="Idle",
     to_state="Moving",
     condition="Speed > 10"
   )
```

### Link Blend Space

```
add_blend_space_player(
  state="Moving",
  blend_space="/Game/Anims/BS2D_Locomotion",
  parameters=["Speed", "Direction"]
)
```

## Variables to Expose

Essential variables from Character to ABP:

| Variable | Type | Purpose |
|----------|------|---------|
| `Speed` | Float | Movement speed |
| `Direction` | Float | Movement direction (-180 to 180) |
| `bIsInAir` | Bool | Jumping/falling |
| `bIsCrouching` | Bool | Crouch state |
| `AimPitch` | Float | Vertical aim |
| `AimYaw` | Float | Horizontal aim |
| `bIsAttacking` | Bool | Combat state |

## Performance Guidelines

1. **Fast Path** - Keep Event Graph minimal, use Fast Path nodes
2. **Update Rate Optimization** - LOD-based update rates for distant characters
3. **Avoid Blueprint VM** - Use native getters over BP functions in ABP
4. **Bone Masks** - Only blend necessary bones, not full skeleton

## References

- `references/state_machine_patterns.md` - Hierarchical state machines, conduits, transitions
- `references/blend_spaces_montages.md` - Detailed blend space setup, montage sections, notifies
