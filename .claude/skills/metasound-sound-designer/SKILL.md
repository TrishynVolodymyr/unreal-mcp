---
name: metasound-sound-designer
description: >
  Sound design architect for Unreal Engine MetaSounds — procedural audio graphs for games.
  Focused on: ambient soundscapes (rain, wind, fire, caves), impact/foley sounds (hits, footsteps),
  and magic/ability audio (charge-up, release, decay). NOT for music composition or dialogue.
  Optimization-first: CPU budgets, voice limits, distance culling.
  ALWAYS trigger on: MetaSound, procedural audio, ambient sound, soundscape, foley, impact sound,
  magic sound, ability sound, weather audio, environmental audio, fire crackling, rain ambience,
  wind sound, footsteps, sword impact, spell sound, charge-up sound, audio graph, sound design,
  or phrases like "create sound", "make audio", "design ambience", "impact SFX", "ability audio".
  Uses metasoundMCP tools when available, or provides manual graph construction guidance.
---

# MetaSound Sound Designer

Sound design architect for Unreal Engine MetaSounds. **Optimization and gameplay reactivity are priorities.**

## Scope

**IN SCOPE:**
- Ambient/environmental soundscapes (rain, fire, wind, caves, forests)
- Impact/foley sounds (weapons, footsteps, collisions)
- Magic/ability audio (charge, release, trails, impacts)
- Gameplay-reactive audio (health, intensity, proximity)

**OUT OF SCOPE:**
- Music composition (use Wwise/FMOD or dedicated tools)
- Dialogue/voiceover processing
- Complex realistic instrument synthesis

---

## Core Philosophy

1. **Layer simple elements** — Complex sounds = multiple simple generators
2. **Gameplay parameters first** — Design for runtime control from the start
3. **Sample + procedural hybrid** — Use samples for realism, procedural for variation
4. **CPU budget awareness** — MetaSounds run on audio thread, respect limits

---

## Core Workflow

### Step 1: Define Sound Requirements

Before creating, clarify:
- **Sound type**: Ambient (looping), One-shot (impact), Reactive (parameter-driven)?
- **Source approach**: Pure procedural, sample-based, or hybrid?
- **Gameplay hooks**: What parameters drive variation? (intensity, distance, state)
- **Performance tier**: Background ambient, gameplay feedback, hero moment?
- **Spatialization**: 2D (UI/music), 3D point source, 3D ambient bed?

### Step 2: Create MetaSound Source

```
Create MetaSound Source asset:
  Name: MS_SoundName
  Path: /Game/Audio/MetaSounds/
```

**Naming conventions:**
- `MS_` — MetaSound Source (playable asset)
- `MSP_` — MetaSound Patch (reusable sub-graph)
- `MSW_` — MetaSound for Waves (sample manipulation)

### Step 3: Design Input Interface

**Always expose these parameters:**
- `Trigger` (trigger) — For one-shots
- `Intensity` (float 0-1) — Master intensity control
- `Pitch` (float) — Pitch variation
- `Volume` (float) — Output level

**Context-specific parameters:**
- `WindSpeed`, `RainIntensity` — Weather
- `ImpactForce`, `SurfaceType` — Impacts
- `ChargeAmount`, `ElementType` — Magic

### Step 4: Build Audio Graph

Follow this signal flow:
1. **Source(s)** — Oscillators, noise, wave players
2. **Modulation** — LFOs, envelopes, random
3. **Processing** — Filters, distortion, delay
4. **Mixing** — Gain staging, panning
5. **Output** — Final stereo/mono out

### Step 5: Test & Iterate

1. Preview in MetaSound editor
2. Test with parameter sweeps
3. Place in level with Audio Component
4. Verify CPU usage in Audio Debug

---

## Node Categories

### Sources (Signal Generators)

| Node | Use Case | CPU Cost |
|------|----------|----------|
| `Sine` | Pure tones, sub-bass | Low |
| `Saw` | Harsh/buzzy tones | Low |
| `Square` | Hollow/digital tones | Low |
| `Noise` | White/pink/brown noise | Low |
| `Wave Player` | Sample playback | Medium |
| `Granular` | Texture/ambience | High |

### Modulation

| Node | Use Case |
|------|----------|
| `LFO` | Cyclic modulation (wobble, pulse) |
| `Envelope` | Attack-decay-sustain-release shapes |
| `Random` | Per-trigger variation |
| `Noise (control)` | Slow random drift |

### Filters

| Node | Use Case |
|------|----------|
| `One Pole Low Pass` | Simple smoothing, cheap |
| `Biquad Filter` | Quality LP/HP/BP |
| `State Variable Filter` | Resonant sweeps |

### Effects

| Node | Use Case | CPU Cost |
|------|----------|----------|
| `Delay` | Echoes, space | Medium |
| `Reverb` | Room simulation | High |
| `Distortion` | Saturation, grit | Low |
| `Chorus` | Width, movement | Medium |

---

## Optimization Rules (PRIORITY)

### CPU Budget Guidelines

| Sound Type | Max Voices | Target CPU/Voice |
|------------|------------|------------------|
| Ambient bed | 2-4 | < 0.5ms |
| Gameplay SFX | 8-16 | < 0.2ms |
| One-shots | 32+ (pooled) | < 0.1ms |

### Voice Management

Every MetaSound MUST consider:
```
- Max concurrent instances (Concurrency limit in Sound Cue/Class)
- Distance-based culling (Attenuation settings)
- Priority system (What yields when budget exceeded?)
```

### Optimization Patterns

**DO:**
- Use `One Pole Low Pass` over `Biquad` when possible
- Disable unused outputs
- Use `Branch` nodes to skip expensive paths
- Pool one-shot sounds
- Cull inaudible sounds aggressively

**AVOID:**
- Multiple reverbs per sound (use send to shared reverb)
- Granular synthesis for simple sounds
- Real-time convolution
- Unbounded delay feedback
- Many simultaneous filter sweeps

### Distance-Based Optimization

| Distance | Action |
|----------|--------|
| Near | Full quality, all layers |
| Medium | Reduce layers, simpler filtering |
| Far | Single layer, no modulation |
| Cull | Stop playback entirely |

---

## Sound Design Patterns

See `references/sound-recipes.md` for complete recipes:
- Fire/campfire ambience
- Rain (light to heavy)
- Wind (calm to storm)
- Cave/dungeon ambience
- Forest ambience
- Sword/weapon impacts
- Footsteps (multi-surface)
- Magic charge-up
- Magic release/impact
- Magic trails/loops

---

## Input Parameter Patterns

### Intensity-Driven Sound

Universal pattern for sounds that scale with gameplay intensity:

```
[Intensity 0.0 - 1.0]
    ├── Controls filter cutoff (darker → brighter)
    ├── Controls layer mix (fewer → more layers)
    ├── Controls modulation depth (subtle → aggressive)
    └── Controls output volume
```

### Trigger + Sustain Pattern

For sounds with attack and sustain phases:

```
[Trigger] ──► Attack envelope ──► Decay to sustain
                                       │
[Gate/Intensity] ──────────────────────┘
                                       │
[Release Trigger] ──► Release envelope ◄┘
```

### Surface/Material Variation

For impacts that vary by surface:

```
[SurfaceType int 0-N]
    ├── Select sample bank
    ├── Adjust filter settings
    └── Modify envelope times

[ImpactForce 0.0 - 1.0]
    ├── Layer selection
    └── Pitch/volume scaling
```

---

## Spatialization Setup

### 3D Point Source (Default)

For localized sounds (fire, impacts, footsteps):
- Enable spatialization on Audio Component
- Set Attenuation Settings
- Use mono MetaSound output

### 3D Ambient Bed

For environmental ambience (rain, wind):
- Stereo output for width
- Gentle attenuation (always audible)
- Consider multiple emitters for large areas

### 2D UI/Feedback

For non-spatial sounds:
- Disable spatialization
- Stereo or mono as needed
- No attenuation

---

## MetaSound + Blueprint Integration

### Exposing Parameters

In MetaSound:
1. Add Input node with appropriate type
2. Mark as "Expose to Blueprint"
3. Set default value

In Blueprint:
```
Set Parameter (Name, Value) on Audio Component
```

### Common Exposed Parameters

| Parameter | Type | Range | Use |
|-----------|------|-------|-----|
| `Intensity` | Float | 0-1 | Master control |
| `Pitch` | Float | 0.5-2.0 | Pitch variation |
| `Trigger` | Trigger | — | Fire one-shots |
| `Playing` | Bool | — | Start/stop loops |

---

## Incremental Workflow

Follow unreal-mcp-architect principles:

1. **Define parameters** — What inputs does sound need?
2. **Build source section** — Basic signal generation
3. **Add modulation** — Movement and variation
4. **Add processing** — Filtering, effects
5. **Connect output** — Gain staging
6. **Test in editor** — Parameter sweeps
7. **Test in level** — Real-world conditions

**After each step:** "Finished [step]. Ready for testing."

---

## Troubleshooting

See `references/troubleshooting.md` for:
- No sound output
- Clicking/popping artifacts
- CPU spikes
- Parameter not responding
- Spatialization issues

---

## Pre-Flight Checklist

Before finalizing any sound:

- [ ] Input parameters exposed and documented
- [ ] Default values set sensibly
- [ ] Intensity/volume control working
- [ ] Distance culling configured
- [ ] Voice limit set (concurrency)
- [ ] CPU usage acceptable
- [ ] Tested with parameter extremes
- [ ] No audio artifacts (clicks, pops, DC offset)
- [ ] Spatialization appropriate for use case
