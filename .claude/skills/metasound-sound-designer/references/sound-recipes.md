# Sound Recipes

Detailed MetaSound graph patterns for common game audio needs.

---

## Table of Contents

1. [Ambient: Fire/Campfire](#ambient-firecampfire)
2. [Ambient: Rain](#ambient-rain)
3. [Ambient: Wind](#ambient-wind)
4. [Ambient: Cave/Dungeon](#ambient-cavedungeon)
5. [Ambient: Forest](#ambient-forest)
6. [Impact: Weapon Hit](#impact-weapon-hit)
7. [Impact: Footsteps](#impact-footsteps)
8. [Magic: Charge-Up](#magic-charge-up)
9. [Magic: Release/Cast](#magic-releasecast)
10. [Magic: Loop/Trail](#magic-looptrail)

---

## Ambient: Fire/Campfire

**Character:** Warm, crackling, organic randomness

### Inputs
- `Intensity` (float 0-1): Fire size/strength
- `WindInfluence` (float 0-1): External wind affecting flames

### Graph Structure

```
LAYER 1: Base Rumble
├── Noise (Brown) → One Pole LP (cutoff: 80-200 Hz via Intensity)
└── Gain: 0.3

LAYER 2: Crackle
├── Noise (White) → Biquad HP (2kHz) → Biquad LP (8kHz)
├── Modulate amplitude with: Random (0.0-1.0) → Envelope (fast attack, medium decay)
├── Trigger envelope randomly: Random Gate (rate: 5-15/sec via Intensity)
└── Gain: 0.4

LAYER 3: Pops (Optional sample layer)
├── Wave Player (pop samples array)
├── Random trigger: 0.5-3/sec
├── Random pitch: 0.9-1.1
└── Gain: 0.2

MIX → Output
```

### Key Parameters
- Crackle rate increases with Intensity
- LP cutoff on rumble opens with Intensity (fire "roars" louder)
- WindInfluence modulates overall amplitude with slow LFO

---

## Ambient: Rain

**Character:** Layered, intensity-scalable, natural variation

### Inputs
- `RainIntensity` (float 0-1): Light drizzle to downpour
- `SurfaceType` (int): 0=outdoor, 1=under cover, 2=on metal

### Graph Structure

```
LAYER 1: Constant Bed (outdoor rain texture)
├── Noise (Pink) → Biquad LP (cutoff: 2000-8000 Hz via RainIntensity)
├── Stereo spread via slight L/R delay difference
└── Gain: RainIntensity * 0.5

LAYER 2: Drop Detail (mid intensity+)
├── Noise (White) → Biquad BP (center: 4kHz, Q: 2)
├── Amplitude: Random pulses
├── Gate: Enable when RainIntensity > 0.3
└── Gain: (RainIntensity - 0.3) * 0.4

LAYER 3: Heavy Rumble (high intensity)
├── Noise (Brown) → One Pole LP (150 Hz)
├── Gate: Enable when RainIntensity > 0.6
└── Gain: (RainIntensity - 0.6) * 0.6

SURFACE MODIFIER:
├── SurfaceType 1 (under cover): HP filter at 500 Hz, reduce volume 50%
├── SurfaceType 2 (metal): Add resonant BP at 3kHz, increase detail layer
└── Mix based on SurfaceType selection

MIX → Output
```

### Key Parameters
- Filter cutoff and layer volume scale with RainIntensity
- Surface type dramatically changes character
- Consider occasional thunder samples triggered externally

---

## Ambient: Wind

**Character:** Flowing, dynamic, never static

### Inputs
- `WindSpeed` (float 0-1): Calm breeze to howling storm
- `Gusts` (bool): Enable random gust bursts

### Graph Structure

```
BASE:
├── Noise (Pink) → State Variable Filter (LP mode)
├── Cutoff: LFO (0.1-0.3 Hz) * WindSpeed range 200-2000 Hz
├── Resonance: 0.3 (slight whistle)
└── Base gain: WindSpeed * 0.6

WHISTLE (high wind):
├── Sine oscillator bank (3 sines)
├── Frequencies: 800, 1200, 1800 Hz (randomized ±50 Hz)
├── Amplitude: LFO modulated, gated when WindSpeed > 0.5
└── Gain: (WindSpeed - 0.5) * 0.3

GUSTS (if enabled):
├── Envelope: Attack 0.5s, Sustain 1s, Release 1s
├── Triggers randomly: 0.1-0.5/sec when Gusts=true
├── Modulates overall amplitude +20-40%
└── Also modulates filter cutoff +500 Hz

MIX → Output
```

### Key Parameters
- LFO on filter cutoff creates natural movement
- Multiple sine whistles prevent static tones
- Gusts add drama without constant intensity

---

## Ambient: Cave/Dungeon

**Character:** Dark, echoey, occasional drips, subtle unease

### Inputs
- `Depth` (float 0-1): Surface entrance to deep underground
- `Wetness` (float 0-1): Dry cave to dripping wet

### Graph Structure

```
LAYER 1: Dark Drone
├── Noise (Brown) → Biquad LP (60-150 Hz via Depth)
├── Very slow LFO (0.02 Hz) on amplitude
└── Gain: 0.2 + Depth * 0.2

LAYER 2: Room Tone
├── Noise (Pink) → Biquad BP (center: 300 Hz, Q: 0.5)
├── Very low level, creates "air" of space
└── Gain: 0.1

LAYER 3: Drips (sample-based)
├── Wave Player (drip sample array, 4-6 variations)
├── Random trigger: 0.1-2/sec * Wetness
├── Random pitch: 0.85-1.15
├── Delay send (300-800ms, feedback 0.3) for echo
└── Gain: 0.3 * Wetness

LAYER 4: Unease Tones (optional, for tension)
├── Sine (very low freq: 18-25 Hz, barely audible)
├── Slow fade in/out over 10-20 seconds
└── Gain: 0.1 * Depth

MIX → Subtle reverb send → Output
```

### Key Parameters
- Depth controls darkness (lower frequencies dominate)
- Wetness controls drip frequency and delay wetness
- Infrasound creates subtle unease without obvious source

---

## Ambient: Forest

**Character:** Layered natural sounds, time-of-day variation

### Inputs
- `Density` (float 0-1): Sparse to dense forest
- `TimeOfDay` (float 0-1): 0=night, 0.5=noon, 1=night
- `WindLevel` (float 0-1): Affects rustling

### Graph Structure

```
LAYER 1: Wind Through Leaves
├── Noise (Pink) → Biquad BP (center: 2-4 kHz)
├── Amplitude: LFO (0.2 Hz) * WindLevel
├── Stereo spread
└── Gain: 0.3 * Density * WindLevel

LAYER 2: Bird Calls (sample-based, daytime)
├── Wave Player (bird call samples, 8+ variations)
├── Random trigger: 0.05-0.3/sec
├── Trigger gated by: TimeOfDay > 0.2 AND TimeOfDay < 0.8
├── Random pitch: 0.9-1.1
├── Random pan L/R
└── Gain: 0.25 * Density

LAYER 3: Insects (sample or synthesis, dusk/night)
├── Option A: High-freq oscillators with random chirp patterns
├── Option B: Cricket/cicada samples
├── Gated by: TimeOfDay < 0.3 OR TimeOfDay > 0.7
└── Gain: 0.2 * Density

LAYER 4: Base Ambience
├── Noise (Pink) → Biquad LP (1 kHz)
├── Very low, constant presence
└── Gain: 0.1

MIX → Output
```

### Key Parameters
- TimeOfDay crossfades between day/night creature layers
- Density affects how "full" the soundscape feels
- Wind creates movement in vegetation layers

---

## Impact: Weapon Hit

**Character:** Punchy, layered, surface-reactive

### Inputs
- `Trigger` (trigger): Fire sound
- `ImpactForce` (float 0-1): Glancing blow to full hit
- `SurfaceType` (int): 0=flesh, 1=metal, 2=wood, 3=stone
- `WeaponType` (int): 0=sword, 1=blunt, 2=arrow

### Graph Structure

```
LAYER 1: Transient/Attack
├── Noise (White) → Envelope (attack: 1ms, decay: 20-50ms)
├── Biquad HP (500 Hz) — removes mud
├── Pitch/decay scales with ImpactForce
└── Gain: 0.5

LAYER 2: Body (surface-dependent sample)
├── Wave Player (impact samples organized by surface)
├── Select sample bank via SurfaceType
├── Pitch: 0.8-1.0 based on ImpactForce
└── Gain: 0.6 * ImpactForce

LAYER 3: Tail/Ring (metal only)
├── Sine bank (2-3 frequencies: 800, 1600, 2400 Hz)
├── Envelope: instant attack, long decay (500ms-1s)
├── Gated: only when SurfaceType = metal
└── Gain: 0.2

LAYER 4: Low Thump (heavy hits)
├── Sine (60-80 Hz) → Envelope (attack: 5ms, decay: 100ms)
├── Gated: only when ImpactForce > 0.6
└── Gain: 0.3 * (ImpactForce - 0.6)

Trigger fires all envelopes simultaneously
MIX → Output
```

### Key Parameters
- ImpactForce affects envelope lengths, pitch, layer activation
- SurfaceType selects sample bank and enables/disables ring layer
- WeaponType can further modify transient character

---

## Impact: Footsteps

**Character:** Subtle variation, surface-aware, movement-speed reactive

### Inputs
- `Trigger` (trigger): Foot contact
- `SurfaceType` (int): 0=stone, 1=wood, 2=dirt, 3=grass, 4=water
- `MovementSpeed` (float 0-1): Walk to sprint
- `FootIndex` (int 0-1): Alternate L/R for subtle variation

### Graph Structure

```
CORE: Sample playback with variation
├── Wave Player (footstep samples per surface, 4+ variations each)
├── Select bank via SurfaceType
├── Cycle through variations (avoid repetition)
├── Pitch: 0.95-1.05 random + MovementSpeed influence
├── Volume: 0.7-1.0 random + MovementSpeed scaling
└── Slight pan based on FootIndex

LAYER: Movement additions
├── Sprint only: Add subtle cloth/gear rustle sample
├── Water: Add splash layer with longer tail
└── Grass: Add subtle brush/foliage layer

Trigger fires primary + conditional layers
→ Output
```

### Key Parameters
- 4+ samples per surface minimum to avoid machine-gun effect
- Random pitch/volume variation essential
- Sprint footsteps are louder, slightly faster decay

---

## Magic: Charge-Up

**Character:** Building tension, ascending pitch/intensity, anticipation

### Inputs
- `ChargeAmount` (float 0-1): Continuously updated during charge
- `ElementType` (int): 0=fire, 1=ice, 2=lightning, 3=arcane

### Graph Structure

```
LAYER 1: Rising Tone
├── Saw oscillator (or sine for softer)
├── Frequency: 100 Hz + ChargeAmount * 400 Hz (rises with charge)
├── Biquad LP (cutoff rises with ChargeAmount)
└── Gain: ChargeAmount * 0.4

LAYER 2: Noise Texture
├── Noise (White) → Biquad BP
├── Center frequency rises with ChargeAmount (500 → 3000 Hz)
├── Resonance increases with ChargeAmount
└── Gain: ChargeAmount * 0.3

LAYER 3: Element Signature
├── Fire: Add crackle layer (see fire recipe)
├── Ice: Add crystalline shimmer (high sine harmonics)
├── Lightning: Add buzz (saw + distortion)
├── Arcane: Add ethereal pad (detuned sines with chorus)
└── Gain: ChargeAmount * 0.3

LAYER 4: Pulse (builds urgency)
├── LFO rate: 2 Hz + ChargeAmount * 8 Hz (speeds up)
├── Modulates overall amplitude ±10-30%
└── More pronounced as ChargeAmount increases

MIX → Output
```

### Key Parameters
- Everything rises with ChargeAmount: pitch, brightness, pulse rate
- Element type adds unique character
- Consider particle SFX sync for visual feedback

---

## Magic: Release/Cast

**Character:** Explosive release, element-specific, satisfying impact

### Inputs
- `Trigger` (trigger): Cast moment
- `CastPower` (float 0-1): Weak to full-power cast
- `ElementType` (int): 0=fire, 1=ice, 2=lightning, 3=arcane

### Graph Structure

```
LAYER 1: Transient Burst
├── Noise (White) → Envelope (attack: 1ms, decay: 50-150ms * CastPower)
├── Biquad BP (element-dependent center frequency)
└── Gain: 0.5 + CastPower * 0.3

LAYER 2: Tonal Punch
├── Sine/Saw → Envelope (attack: 5ms, decay: 100-300ms)
├── Frequency: Element-dependent base (fire=low, lightning=mid, ice=high)
├── Pitch drops slightly during decay (satisfying "whump")
└── Gain: 0.4 * CastPower

LAYER 3: Element Tail
├── Fire: Woosh + crackle tail (200-500ms)
├── Ice: Crystalline shatter + shimmer (300-600ms)
├── Lightning: Electrical zap + thunder rumble (100-400ms)
├── Arcane: Ethereal whoosh + reverb tail (400-800ms)
└── Gain: 0.4

LAYER 4: Sub Impact (power casts)
├── Sine (40-60 Hz) → Envelope (attack: 10ms, decay: 150ms)
├── Gate: only when CastPower > 0.5
└── Gain: 0.3 * (CastPower - 0.5)

Trigger fires all layers
MIX → Output
```

### Key Parameters
- CastPower affects duration, sub activation, overall punch
- Element tail is the signature differentiator
- Sync with visual VFX burst

---

## Magic: Loop/Trail

**Character:** Sustained magical presence, movement-reactive, element-flavored

### Inputs
- `Playing` (bool): Loop on/off
- `Intensity` (float 0-1): Idle to active
- `ElementType` (int): 0=fire, 1=ice, 2=lightning, 3=arcane
- `MovementSpeed` (float 0-1): Affects whoosh intensity

### Graph Structure

```
LAYER 1: Element Base Loop
├── Fire: Filtered noise (see fire recipe, scaled down)
├── Ice: High crystalline drone (detuned sines, slow LFO)
├── Lightning: Electrical hum (50/60 Hz + harmonics, filtered)
├── Arcane: Ethereal pad (soft sines, chorus, slow movement)
└── Gain: 0.3 * Intensity

LAYER 2: Movement Whoosh
├── Noise (Pink) → Biquad BP (1-3 kHz)
├── Amplitude follows MovementSpeed
├── Filter cutoff follows MovementSpeed
└── Gain: 0.25 * MovementSpeed

LAYER 3: Sparkle/Detail
├── Random high-frequency pings (element-colored)
├── Fire: Crackle pops
├── Ice: Crystalline tinks
├── Lightning: Small zaps
├── Arcane: Shimmer tones
├── Rate: 2-8/sec based on Intensity
└── Gain: 0.2 * Intensity

ENVELOPE:
├── Attack: 100-200ms (smooth fade in)
├── Release: 200-400ms (smooth fade out)
└── Controlled by Playing bool

MIX → Output
```

### Key Parameters
- Playing gates entire sound with smooth envelope
- Intensity affects richness without changing character
- MovementSpeed adds dynamic whoosh layer
- Element type defines sonic palette
