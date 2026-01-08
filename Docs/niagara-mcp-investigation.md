# Niagara MCP Investigation - Module Settings Not Applying

## Problem Statement
MCP tools report success when setting Niagara module inputs, but the visual result shows the settings have no effect. The fire VFX looks like floating orbs instead of flames.

## Observed Issues

### Issue 1: Curl Noise Force Not Applying
**What was set via MCP:**
- Noise Strength = 300
- Pan Noise Field = (80, 50, 150)
- Mask Curl Noise = true
- Curl Noise Cone Mask Axis = (0, 0, 1)
- Curl Noise Cone Mask Angle = 60

**Expected behavior:** Particles should have chaotic, flickering motion biased upward

**Actual behavior:** Particles move in perfectly straight lines upward - zero turbulence visible

**Hypothesis:**
- Module might be in wrong stage (Spawn vs Update)?
- Values might not be persisting after compile?
- Something overriding the force?

---

### Issue 2: ScaleSpriteSize Not Working
**What was set via MCP:**
- Scale Sprite Size Mode = "Uniform"
- Uniform Scale Factor = 0.3

**Expected behavior:** Particles should shrink to 30% of original size over lifetime

**Actual behavior:** Particles remain same size from spawn to death

**Hypothesis:**
- Mode might not actually be changing?
- Scale factor might need curve mode instead?
- Module might not be in Update stage?

---

### Issue 3: Size Variation Not Working
**What was set via MCP:**
- Sprite Size Min = (30, 30)
- Sprite Size Max = (80, 80)

**Expected behavior:** Particles should spawn with random sizes between 30-80

**Actual behavior:** All particles appear identical size

**Hypothesis:**
- Randomness mode might not be enabled?
- Min/Max might be ignored without proper mode?

---

### Issue 4: Spawn Location Too Spread
**Current state:** Particles spawn spread across a wide horizontal area (3 separate fire actors visible, each with particles spread out)

**Expected:** Concentrated spawn at a small point/disk at base

**Hypothesis:**
- SphereLocation radius might be too large?
- Multiple actors spawned during testing?

---

## Investigation Plan

1. Query each module's current state via get_niagara_metadata
2. Compare queried values vs what we set
3. Check if modules are in correct stages (Spawn vs Update)
4. Check for any modules that might override our settings
5. Verify the MCP set_module_input is actually writing to RapidIterationParameters

## Investigation Results

### Finding 1: Values ARE Being Set Correctly
All the numeric values we set via MCP are persisting correctly:
- CurlNoiseForce: Noise Strength=300, Pan Noise Field=(80,50,150), Cone Mask settings all correct
- ScaleSpriteSize: Mode="Uniform", Uniform Scale Factor=0.3
- SphereLocation: Radius=25, Non Uniform Scale=(1,1,0.2)
- InitializeParticle: Sprite Size Min/Max=(30,30)/(80,80), Lifetime Min/Max=0.3/0.8

### Finding 2: MODE Settings Are Preventing Values From Being Used

**Issue 3 Root Cause - Size Variation:**
- Sprite Size Min = (30, 30) ✓
- Sprite Size Max = (80, 80) ✓
- **Sprite Size Mode = "Unset"** ← PROBLEM! Uses fixed Sprite Size instead
- **Sprite Size Randomness Mode = "Simulation Defaults"** ← Not "Random Range"

**Lifetime Variation (related):**
- Lifetime Min = 0.3 ✓
- Lifetime Max = 0.8 ✓
- **Lifetime Mode = "Direct Set"** ← PROBLEM! Uses fixed Lifetime instead

### Finding 3: ScaleSpriteSize "Uniform" Mode
- "Uniform Scale Factor = 0.3" applies a CONSTANT scale multiplier
- It does NOT scale over lifetime
- To shrink particles over time, need "Uniform Curve" mode with a curve from 1.0→0.0

### Finding 4: Curl Noise - Unknown
- All settings correct
- Noise Frequency = 0.05 (might be too low?)
- Need to verify SolveForcesAndVelocity is processing the force

### Root Cause Summary
**The MCP set_module_input tool sets VALUES correctly, but the MODULE MODES that control HOW those values are used are not being changed.**

Setting `Sprite Size Min = (30,30)` does nothing if `Sprite Size Mode` remains "Unset".

### Required Fixes
1. Need to change `Sprite Size Mode` to enable random sizing
2. Need to change `Lifetime Mode` to enable random lifetime
3. Need to change `Scale Sprite Size Mode` to "Uniform Curve" for over-lifetime scaling
4. Need to verify/fix curl noise application

---

## RESOLUTION (Session 2)

### Key Discovery: TWO Settings Required for Randomization!

Setting a Mode (e.g., `Lifetime Mode = "Random"`) was NOT sufficient!
You ALSO need to set the corresponding **Randomness Mode** to `"Non-Deterministic"`.

### Niagara Randomness Mode Enum Values
Discovered via improved MCP error messages:
- `"Simulation Defaults"` - Uses emitter-level defaults (may not enable randomization)
- `"Determinisitic"` (note: typo in Niagara!) - Reproducible random values
- `"Non-Deterministic"` - True random values per particle

### Final Configuration Applied

**InitializeParticle Module:**
| Setting | Value | Purpose |
|---------|-------|---------|
| Sprite Size Mode | Non-Uniform | Enable X/Y size separately |
| Sprite Size Randomness Mode | Non-Deterministic | Enable actual randomization |
| Sprite Size Min | (30, 30) | Minimum size |
| Sprite Size Max | (80, 80) | Maximum size |
| Lifetime Mode | Random | Enable random lifetime |
| Lifetime Randomness Mode | Non-Deterministic | Enable actual randomization |
| Lifetime Min | 0.3 sec | Minimum lifetime |
| Lifetime Max | 0.8 sec | Maximum lifetime |

**CurlNoiseForce Module (Update stage):**
| Setting | Value | Purpose |
|---------|-------|---------|
| Noise Strength | 300 | Force magnitude |
| Noise Frequency | 0.3 | Turbulence detail (increased from 0.05) |
| Mask Curl Noise | true | Enable directional bias |
| Curl Noise Cone Mask Axis | (0, 0, 1) | Bias toward +Z (up) |
| Curl Noise Cone Mask Angle | 60 | Cone width |

### MCP Tool Improvement Made
Enhanced error message for enum values now lists all valid options with both display names and internal names:
```
Enum value 'test' not found in enum 'ENiagaraRandomnessMode'.
Valid values: 'Simulation Defaults' (internal: NewEnumerator0),
'Determinisitic' (internal: NewEnumerator1),
'Non-Deterministic' (internal: NewEnumerator2)
```

### Lessons Learned
1. Niagara modules often require BOTH a Mode setting AND a Randomness Mode setting
2. "Simulation Defaults" for Randomness Mode may not enable randomization
3. Always query module_inputs metadata to verify all enum states after setting values
4. Improved error messages are critical for debugging enum value issues
