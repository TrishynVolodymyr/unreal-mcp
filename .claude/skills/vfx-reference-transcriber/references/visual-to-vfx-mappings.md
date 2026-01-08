# Visual-to-VFX Mappings

Translation tables from visual observations to Niagara parameters.

---

## Fire & Embers

| Visual Description | VFX Translation |
|-------------------|-----------------|
| "Embers floating up" | Spawn: continuous 20-50/s, Velocity: up 50-150, Gravity: -200 (negative = rise), Drag: 0.5, Lifetime: 2-4s |
| "Sparks shooting out" | Spawn: burst 10-30, Velocity: cone 300-800, Gravity: 400, Drag: 0.3, Lifetime: 0.5-1.5s |
| "Flickering flames" | Spawn: continuous 50-100/s, Curl noise: 200-400, Lifetime: 0.3-0.8s, Color: orange→yellow→black |
| "Gentle campfire" | Low spawn rate, gentle curl, slow rise, warm colors fading |
| "Roaring inferno" | High spawn rate, aggressive turbulence, bright emissive, secondary smoke |
| "Dying coals" | Very sparse, slow drift, deep red→black, long lifetime |
| "Sparks on impact" | Burst spawn, radial velocity, high drag, brief lifetime |

**Fire Color Curves**
- Hot core: white/yellow (1.0, 0.9, 0.5) → emissive 3-5
- Flame body: orange (1.0, 0.4, 0.0) → emissive 2-3
- Flame edge: red-orange (0.8, 0.2, 0.0) → emissive 1-2
- Ember: deep red (0.6, 0.1, 0.0) → emissive 0.5-1
- Fade to: black with alpha fadeout

---

## Smoke & Vapor

| Visual Description | VFX Translation |
|-------------------|-----------------|
| "Thick billowing smoke" | Large particles 50-200, high spawn, strong curl noise, slow velocity up |
| "Wispy smoke trails" | Small-medium size, low spawn, gentle turbulence, long lifetime |
| "Smoke dissipating" | Size grows over life, opacity fades, increasing drag |
| "Steam rising" | Fast initial up velocity, rapid size growth, quick fade, white/gray |
| "Heavy fog" | Volume spawn, minimal velocity, very long lifetime, no gravity |
| "Smoke puffs" | Burst spawn, radial expansion, medium lifetime |
| "Exhaust plume" | Cone spawn, directional velocity, size/opacity over life |

**Smoke Behavior Keys**
- Curl noise strength: 100-300 (gentle) | 300-600 (active) | 600+ (turbulent)
- Size over life: typically 1.5x-3x growth
- Drag: 0.3-0.8 (smoke slows down)
- Color: gray gradient with alpha fade

---

## Magic & Energy

| Visual Description | VFX Translation |
|-------------------|-----------------|
| "Glowing orbs" | Sphere spawn, slow drift, high emissive, soft falloff |
| "Magical particles swirling" | Orbit/vortex velocity, curl noise, color cycling |
| "Energy crackling" | Burst + continuous mix, erratic velocity, HDR emissive |
| "Aura/glow effect" | Surface spawn on character, low velocity, high emissive |
| "Spell charging" | Inward spiral velocity, increasing spawn rate, growing brightness |
| "Magic impact" | Burst radial, multiple colors, secondary particles |
| "Ethereal wisps" | Ribbon renderer, gentle curves, fade over length |
| "Lightning/electricity" | Beam renderer, jagged path, bright flash + fade |

**Magic Color Palettes**
- Arcane: purple (0.6, 0.2, 1.0) ↔ blue (0.2, 0.4, 1.0)
- Holy: gold (1.0, 0.85, 0.3) ↔ white
- Nature: green (0.3, 1.0, 0.3) ↔ yellow
- Fire magic: orange ↔ red with high emissive
- Ice: cyan (0.4, 0.9, 1.0) ↔ white
- Dark: purple-black with subtle emissive

---

## Debris & Physical

| Visual Description | VFX Translation |
|-------------------|-----------------|
| "Chunks flying" | Mesh renderer, burst spawn, high velocity, gravity 980, bounce collision |
| "Dust cloud on impact" | Burst spawn, radial expansion, quick fade, size growth |
| "Rubble falling" | Mesh renderer, spawn from surface, gravity, varied sizes |
| "Splinters/shards" | Small mesh particles, cone velocity, tumble rotation |
| "Dirt spray" | Cone spawn upward, gravity, size variation, earth colors |
| "Glass breaking" | Mesh shards, radial burst, light refraction material |

**Physical Motion Keys**
- Gravity: 980 (Earth normal)
- Bounce: restitution 0.3-0.7
- Rotation: random axis, speed based on velocity
- Drag: 0.1-0.3 for solid objects

---

## Weather & Environmental

| Visual Description | VFX Translation |
|-------------------|-----------------|
| "Rain falling" | Line/streak sprites, high velocity down, no turbulence |
| "Snow drifting" | Slow fall, gentle horizontal drift, curl noise |
| "Leaves in wind" | Mesh renderer, turbulence, varied rotation, slow fall |
| "Dust motes in light" | Very small, slow random velocity, long lifetime |
| "Ash falling" | Slow descent, gentle turbulence, dark particles |
| "Pollen/spores" | Tiny particles, drift movement, bright in light |
| "Underwater bubbles" | Rise with wobble, size variation, pop at surface |

**Environmental Densities**
- Light atmosphere: 5-20 particles visible
- Medium: 20-50 particles
- Dense: 50-200 particles
- Storm/heavy: 200+ particles

---

## Motion Descriptors

| Word | Translation |
|------|-------------|
| "Floating" | Low/negative gravity, high drag |
| "Drifting" | Low velocity, gentle turbulence |
| "Shooting" | High initial velocity, burst spawn |
| "Exploding" | Radial burst, high velocity, multiple sizes |
| "Swirling" | Curl noise or orbit velocity |
| "Rising" | Negative gravity or upward velocity |
| "Falling" | Normal gravity |
| "Spiraling" | Vortex/orbit velocity module |
| "Flickering" | Random size/opacity variation |
| "Pulsing" | Sine wave on spawn rate or size |
| "Fading" | Opacity curve to zero |
| "Growing" | Size over life increasing |
| "Shrinking" | Size over life decreasing |
| "Tumbling" | Mesh with random rotation velocity |
| "Streaming" | Ribbon renderer, continuous spawn |

---

## Intensity/Feel Mapping

| Feel | Spawn Rate | Velocity | Turbulence | Emissive |
|------|-----------|----------|------------|----------|
| Gentle/calm | Low (5-20/s) | Slow | Subtle | Low |
| Moderate | Medium (20-50/s) | Medium | Light | Medium |
| Active | High (50-100/s) | Fast | Medium | Medium-High |
| Intense | Very high (100+/s) | Very fast | Strong | High |
| Chaotic | Burst + continuous | Varied | Aggressive | HDR |
| Subtle/ambient | Very low (<10/s) | Very slow | Minimal | None-Low |

---

## Density/Thickness

| Description | Particle Count | Size | Opacity |
|-------------|----------------|------|---------|
| "Sparse/few" | <20 visible | Any | Any |
| "Scattered" | 20-50 visible | Small-medium | Low-medium |
| "Moderate" | 50-100 visible | Medium | Medium |
| "Dense/thick" | 100-300 visible | Medium-large | Medium-high |
| "Opaque/solid" | 300+ visible | Large | High, overlapping |
