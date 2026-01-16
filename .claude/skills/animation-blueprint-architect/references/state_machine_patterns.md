# State Machine Patterns

## Hierarchical State Machines

Nested state machines for complex characters.

### Structure

```
[Root State Machine]
├── [Locomotion] (sub-state machine)
│   ├── Idle
│   ├── Walking
│   ├── Running
│   └── Crouching (sub-state machine)
│       ├── Crouch Idle
│       └── Crouch Walk
├── [Combat] (sub-state machine)
│   ├── Ready
│   ├── Attacking
│   └── Blocking
└── [Traversal] (sub-state machine)
    ├── Climbing
    ├── Vaulting
    └── Swimming
```

### When to Nest

- Group related states (all crouch states together)
- Isolate complex subsystems (combat separate from locomotion)
- Share transitions (any combat state → locomotion)

## Conduit States

Virtual states for complex transition logic.

### Use Case: Multi-condition Transitions

Instead of:
```
Idle → Jump (if bJump AND bGrounded AND Stamina > 10)
Walk → Jump (if bJump AND bGrounded AND Stamina > 10)
Run  → Jump (if bJump AND bGrounded AND Stamina > 10)
```

Use conduit:
```
Idle → [CanJump?] → Jump
Walk → [CanJump?] → Jump  
Run  → [CanJump?] → Jump

Conduit "CanJump?" evaluates: bJump AND bGrounded AND Stamina > 10
```

## Transition Interrupt Patterns

### Mutual Exclusion

Prevent animation fighting:

```
[Attacking] ←--X--→ [Dodging]
     ↓                  ↓
  (cannot interrupt)  (cannot interrupt)
```

Implementation: Check `!bIsAttacking` before allowing dodge transition.

### Priority Interrupts

High-priority actions interrupt lower:

```
Priority 1 (highest): Death
Priority 2: Stagger/Knockback  
Priority 3: Dodge
Priority 4: Attack
Priority 5: Locomotion
```

Transition rule: `NewStatePriority > CurrentStatePriority`

### Queued Transitions

Buffer input during uninterruptible states:

```
[Attacking]
  → On "Attack" input: QueuedAction = Attack
  → On state exit: Check QueuedAction
      → If Attack: Transition to ComboAttack
      → Else: Return to Locomotion
```

## Blend Profiles

### Standard Transitions

| Transition Type | Duration | Blend Mode |
|-----------------|----------|------------|
| Idle ↔ Move | 0.2s | Smooth |
| Walk ↔ Run | 0.15s | Smooth |
| Any → Jump | 0.1s | Smooth |
| Fall → Land | 0.05s | Snap |
| Combat entry | 0.2s | Smooth |
| Hit reaction | 0.05s | Smooth |
| Death | 0.1s | Smooth |

### Custom Blend Curves

For non-linear transitions:

```
Attack wind-up: Ease-in (slow start, fast end)
Attack recovery: Ease-out (fast start, slow end)
Dodge: Linear (consistent speed)
```

## Linked Anim Graphs

Share animation logic across multiple ABPs.

### Template ABP Pattern

```
ABP_CharacterBase (template)
├── Locomotion State Machine
├── Combat State Machine
└── IK Setup

ABP_Player (child)
└── Inherits all, adds:
    └── Camera-driven aim offset

ABP_Enemy_Melee (child)  
└── Inherits all, adds:
    └── AI-specific attack patterns
```

### Linked Layers

```
ABP_Player
├── Base Layer: ABP_Locomotion_Linked
├── Combat Layer: ABP_Combat_Linked
└── Additive Layer: ABP_Reactions_Linked
```

## State Aliases

Reference multiple states with one name for cleaner transitions.

### Setup

```
Alias "AnyLocomotion" = [Idle, Walk, Run, Crouch]
Alias "AnyAirborne" = [Jump, Fall, DoubleJump]
```

### Usage

```
Transition: "AnyLocomotion" → Dodge
(Instead of 4 separate transitions)
```

## Animation Sharing Between States

### Pose Snapshot

Capture pose for smooth custom transitions:

```
1. [Current State] → Snapshot current pose
2. → [New State]
3. Blend from snapshot to new state animation
```

### Inertialization

Built-in UE system for instant transitions:

```
[Any State] ---(Inertialization)-→ [New State]

Settings:
- Blend Time: 0.3s
- No blend asset needed
- Preserves momentum
```

## Debug Patterns

### State Logging

Add Print String nodes to track:
- State entry/exit
- Transition triggers  
- Variable values during transitions

### Visualization

Enable in ABP:
- Show Current State (viewport text)
- Show State Weights (blend percentages)
- Show Transition Info

### Common Issues

| Symptom | Likely Cause |
|---------|--------------|
| Stuck in state | Missing/false transition condition |
| Instant snap | Blend time = 0 |
| T-pose flash | Missing animation reference |
| Jittery blend | Conflicting state machines |
| No transition | Condition never true, check variable binding |
