# MetaSound Troubleshooting

Common issues and solutions when working with MetaSounds.

---

## No Sound Output

### Symptom
MetaSound plays in editor preview but no sound in-game or level.

### Checklist

1. **Audio Component setup**
   - Is Audio Component attached to actor?
   - Is `Auto Activate` enabled (for ambient) or `Play()` called?
   - Is the MetaSound asset assigned?

2. **Output connections**
   - Is final mix connected to `Out Left` / `Out Right` / `Out Mono`?
   - Are all nodes in signal path actually connected?

3. **Gain staging**
   - Any node outputting 0 or near-zero?
   - Check multipliers — a `0.0` anywhere kills signal

4. **Attenuation**
   - Is sound too far from listener? Check attenuation settings
   - Is `Override Attenuation` checked with valid settings?

5. **Triggers/Gates**
   - For one-shots: Is `Trigger` input actually receiving trigger?
   - For loops: Is gate/bool input set to true?

### Quick Test
Add a simple `Sine (440 Hz) → Out Mono` to verify basic playback works.

---

## Clicking/Popping Artifacts

### Symptom
Audible clicks, pops, or discontinuities in sound.

### Causes & Solutions

1. **Abrupt amplitude changes**
   - **Fix:** Add envelope with non-zero attack/release (even 5-10ms helps)
   - **Fix:** Use `Smooth` node before amplitude changes

2. **Oscillator phase discontinuity**
   - **Fix:** Reset oscillator phase on trigger if needed
   - **Fix:** Use `Sync` input properly

3. **Filter instability**
   - **Fix:** Reduce resonance/Q value
   - **Fix:** Ensure cutoff frequency stays in valid range (not 0 Hz or Nyquist)

4. **Sample loop points**
   - **Fix:** Ensure wave files have proper loop points
   - **Fix:** Use crossfade looping in Wave Player settings

5. **Parameter jumps**
   - **Fix:** Smooth parameter changes with `One Pole LP` on control signals
   - **Fix:** Use `Interp` node for gradual changes

### Debug Approach
Isolate layers one at a time to find the clicking source.

---

## CPU Spikes / High Usage

### Symptom
Audio thread spiking, stuttering, or dropped audio.

### Diagnosis

1. **Enable Audio Debug** (`stat Audio` in console)
2. **Check voice count** — Too many simultaneous sounds?
3. **Profile individual MetaSounds** in editor

### Common Causes & Fixes

| Cause | Fix |
|-------|-----|
| Too many voices | Add concurrency limits, distance culling |
| Granular synthesis overuse | Use only when necessary, reduce grain count |
| Multiple reverbs | Use send to single shared reverb |
| Complex filter chains | Simplify, use cheaper filter types |
| Unused processing | Add `Branch` nodes to skip when not needed |
| High sample rate internal | Match project sample rate |

### Optimization Steps

1. Reduce node count — combine where possible
2. Use `One Pole` instead of `Biquad` where quality allows
3. Add `Branch` to bypass expensive paths when parameter is 0
4. Implement LOD — simpler graphs at distance
5. Pool one-shot sounds with concurrency limits

---

## Parameter Not Responding

### Symptom
Changing input parameter has no effect on sound.

### Checklist

1. **Input node exists and is exposed**
   - Is the Input node actually in the graph?
   - Is it marked as "Exposed to Blueprint"?

2. **Input is connected**
   - Trace wire from Input to where it should affect sound
   - Any broken connections?

3. **Parameter name matches exactly**
   - Blueprint uses exact name (case-sensitive)
   - No typos in `Set Parameter` node

4. **Type matches**
   - Float input expects float parameter
   - Trigger input requires `Execute Trigger` (not Set Float)

5. **Range is sensible**
   - Is 0-1 input actually being mapped to useful range?
   - Add `Map Range` node if input range doesn't match expected

### Debug Approach
Add a `Print` node to verify input value is reaching the graph.

---

## Spatialization Issues

### Symptom
Sound doesn't move with source, wrong panning, no distance attenuation.

### 3D Sound Not Working

1. **Check Audio Component settings**
   - `bOverrideAttenuation` = true
   - Attenuation Settings assigned or overridden

2. **MetaSound output**
   - Use `Out Mono` for proper 3D spatialization
   - Stereo outputs may not spatialize correctly

3. **Listener position**
   - Is Audio Listener on correct actor (usually Camera)?
   - Multiple listeners can cause issues

### Distance Attenuation Not Working

1. **Attenuation settings**
   - Check inner/outer radius values
   - Verify falloff curve is not flat

2. **Sound too loud at source**
   - May be hitting 0dB before attenuation kicks in
   - Reduce source volume

### Stereo Ambient Issues

For ambient beds that should still have width:
- Use stereo output
- Set attenuation but with larger radius
- Consider disabling 3D entirely for true "background"

---

## Compilation Errors

### Symptom
MetaSound won't compile, red error nodes.

### Common Errors

1. **Unconnected required inputs**
   - Some nodes require all inputs connected
   - Add default value nodes if needed

2. **Type mismatch**
   - Connecting float to int (or vice versa)
   - Add explicit conversion nodes

3. **Circular dependencies**
   - Feedback loops without proper `Delay` node
   - Audio feedback requires at least 1-sample delay

4. **Missing references**
   - Wave Player pointing to deleted/moved asset
   - Reassign the asset reference

### After Fixing
Right-click → "Refresh Node" on problem nodes, then recompile.

---

## Sound Too Quiet / Too Loud

### Symptom
Output level doesn't match expectations.

### Gain Staging Guidelines

1. **Internal levels**: Keep between -12dB and 0dB
2. **Final output**: Should peak around -6dB to -3dB
3. **Headroom**: Leave room for mixing in game

### Common Issues

| Problem | Solution |
|---------|----------|
| Too quiet | Check for any 0.0 multipliers in chain |
| Too loud / clipping | Reduce gains, add limiter before output |
| Inconsistent levels | Normalize all source samples |
| Frequency masking | EQ to separate competing frequencies |

### Debug Approach
Add gain nodes at key points to isolate where level is wrong.

---

## Random/Variation Not Working

### Symptom
Sound plays identically every time despite randomization.

### Checklist

1. **Random node seeding**
   - Is seed changing? (should be automatic per-trigger)
   - Try different random node type

2. **Random range**
   - Is min/max range actually different?
   - Is random output being used (connected)?

3. **Trigger timing**
   - Random evaluated once at trigger
   - For continuous random: use noise or LFO instead

4. **Sample selection**
   - Wave Player random selection enabled?
   - Multiple samples in array?

### For Better Variation
- Layer multiple random sources
- Randomize multiple parameters (pitch, filter, timing)
- Use slightly different ranges on each layer
