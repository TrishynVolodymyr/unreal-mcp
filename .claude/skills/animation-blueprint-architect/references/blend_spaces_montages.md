# Blend Spaces & Montages Reference

## Blend Space Deep Dive

### Axis Configuration

#### 1D Blend Space

| Use Case | Axis | Range | Samples |
|----------|------|-------|---------|
| Walk/Run | Speed | 0-600 | 3-5 |
| Lean | Angle | -45 to 45 | 3 |
| Aim Pitch | Pitch | -90 to 90 | 3-5 |

#### 2D Blend Space

| Use Case | X Axis | Y Axis | Grid |
|----------|--------|--------|------|
| Locomotion | Direction (-180/180) | Speed (0/600) | 5x4 |
| Aim Offset | Yaw (-90/90) | Pitch (-90/90) | 5x5 |
| Strafe | Direction (-90/90) | Speed (0/600) | 3x3 |

### Sample Placement Rules

1. **Corners first** - Place extreme poses at grid corners
2. **Center neutral** - Idle/neutral at origin (0,0)
3. **Even spacing** - Consistent intervals for predictable blending
4. **Match timing** - All samples same length for clean loops

### Interpolation Settings

| Setting | Value | Use Case |
|---------|-------|----------|
| Interpolation Time | 0.0 | Instant response (aim) |
| Interpolation Time | 0.1 | Smooth (locomotion) |
| Target Weight Interpolation | Per Bone | Partial body blending |

### Axis Snapping

```
Snap to Grid: ON for locomotion (prevents diagonal drift)
Snap to Grid: OFF for aim offset (smooth targeting)
```

## Aim Offset Specifics

### Setup Requirements

1. **Base Pose** - Neutral aim (looking forward, arms natural)
2. **Additive Type** - Mesh Space (not Local Space)
3. **LOD** - Can reduce samples at distance

### Sample Grid (Typical)

```
        -90°    -45°     0°     45°    90°   ← Yaw
  90°   Up+L    Up+CL   Up     Up+CR   Up+R   ← Pitch
  45°   MU+L    MU+CL   MU     MU+CR   MU+R
   0°   Mid+L   Mid+CL  Center Mid+CR  Mid+R
 -45°   MD+L    MD+CL   MD     MD+CR   MD+R
 -90°   Dn+L    Dn+CL   Down   Dn+CR   Dn+R
```

### Bone Filtering

Only apply to upper body:
```
Bones: spine_01 and children
Exclude: pelvis, leg chains
```

## Montage Architecture

### Slot Configuration

```
Slot Groups:
├── DefaultGroup
│   └── DefaultSlot (full body)
├── UpperBody  
│   └── UpperBodySlot (spine_01+)
├── Arms
│   ├── LeftArm (clavicle_l+)
│   └── RightArm (clavicle_r+)
└── Additive
    └── AdditiveSlot (hit reacts)
```

### Section Workflow

#### Linear Montage (Simple Attack)

```
[Default] ───────────────────→ End
   0.0s                      0.8s
```

#### Branching Montage (Combo)

```
[Start] → [Attack1] → [Attack2] → [Attack3] → [End]
  0.0s      0.3s        0.6s        0.9s      1.2s
              │           │           │
              └→ [End]    └→ [End]    └→ [End]
              (no input)  (no input)  (no input)
```

#### Looping Section

```
[WindUp] → [Loop] ←─┐ → [Release] → [End]
              └─────┘
         (hold button)  (release)
```

### Section Timing

| Section | Duration | Purpose |
|---------|----------|---------|
| WindUp | 0.1-0.3s | Anticipation, can cancel |
| Active | 0.2-0.5s | Damage window |
| Recovery | 0.2-0.4s | Return to neutral |
| Combo Window | 0.1-0.2s | Input buffer |

### Blend Settings Per Montage

```
Blend In:  0.1s (quick entry)
Blend Out: 0.2s (smooth exit)
Blend Mode: Standard (replace base)

For additives:
Blend Mode: Additive
```

## Anim Notify Deep Dive

### Notify Types

| Type | Class | Timing |
|------|-------|--------|
| Single Frame | `AnimNotify` | Exact frame |
| Duration | `AnimNotifyState` | Start + End |
| Native | C++ class | Best performance |
| Blueprint | BP class | Easy iteration |

### Common Notify Implementations

#### Footstep

```
AN_Footstep
├── Triggers at: foot contact frame
├── Plays: Footstep sound
├── Spawns: Dust particle (optional)
└── Parameters: Foot (Left/Right), Surface type
```

#### Weapon Trail

```
ANS_WeaponTrail
├── Begin: Trail start frame
├── End: Trail end frame  
├── Effect: Niagara ribbon trail
└── Socket: weapon_trail_start, weapon_trail_end
```

#### Damage Window

```
ANS_DamageWindow
├── Begin: Attack becomes active
├── End: Attack deactivates
├── Notifies: Character to enable hitbox
└── Data: Damage amount, type, direction
```

#### Combo Input Window

```
ANS_ComboWindow
├── Begin: Start accepting input
├── End: Stop accepting input
├── On Input: Set NextSection
└── No Input: Continue to recovery
```

### Notify Curves

Drive values over time:

```
Curve: "DamageMultiplier"
├── Frame 10: 0.0 (wind up)
├── Frame 15: 1.0 (peak damage)
└── Frame 25: 0.5 (follow through)

Access in BP: GetCurveValue("DamageMultiplier")
```

## Montage Playback Control

### From Character Blueprint

```
Play Montage:
├── Montage: AM_Attack_Light
├── Play Rate: 1.0
├── Start Section: "Default"
└── Return: Montage Instance ID

Stop Montage:
├── Montage: (specific or all)
└── Blend Out: 0.2s

Jump to Section:
├── Section: "Attack2"
└── Montage: AM_Attack_Combo
```

### From Animation Blueprint

```
Montage Is Playing: Check if active
Get Current Section: For state logic
Montage Set Next Section: Queue combo
```

## Root Motion

### Montage Root Motion Settings

| Setting | Value | Use Case |
|---------|-------|----------|
| Root Motion | Enable | Dodge rolls |
| Root Motion | Disable | In-place attacks |
| Extract Root Motion | Distance | Movement-based |
| Extract Root Motion | Velocity | Physics-based |

### Handling in Character

```
On Montage Started:
├── Enable Root Motion Mode
└── Set Movement Mode: Flying (for full control)

On Montage Ended:
├── Disable Root Motion Mode
└── Restore Movement Mode: Walking
```
