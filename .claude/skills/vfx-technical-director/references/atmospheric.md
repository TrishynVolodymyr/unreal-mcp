# Atmospheric Effects: Fire, Smoke, Fog, Clouds

Volumetric-looking effects that fill space. The challenge: achieving depth and realism.

## Viewing Angle Categories

Atmospheric effects fall into distinct categories based on camera behavior:

| Category | Camera Behavior | Viable Techniques |
|----------|-----------------|-------------------|
| **Fixed background** | Camera never orbits effect | POM, RM, flipbooks, all techniques |
| **Limited arc** | Camera moves ±45° around effect | Multi-plane, layered sprites |
| **Full 360°** | Camera can orbit freely | Mesh particles, true volumetrics |
| **Inside volume** | Camera enters the effect | True volumetrics only |

**Determine category first.** Technique selection depends entirely on this.

---

## Technique Breakdown

### 1. True Volumetrics (UE5 Heterogeneous Volumes)

**What it is:** Actual 3D density field rendered via raymarching.

**Capabilities:**
- Correct from any angle including inside
- Receives and casts shadows
- Responds to scene lighting
- True depth intersection with geometry

**Cost:**
- ~4ms+ per volume at quality settings
- `r.VolumetricFog.GridPixelSize 4` = high quality, expensive
- `r.VolumetricFog.GridPixelSize 8` = default, 4x cheaper

**When to use:**
- Hero moments (one or two instances)
- Cutscenes
- Large environmental fog where no other technique works
- Camera will enter the volume

**When NOT to use:**
- Gameplay effects with many instances
- Fast-moving effects (latency in volume updates)
- Mobile/low-end targets

---

### 2. POM + Ray Marching (FluidNinja approach)

**What it is:** 2D flipbook with depth/shadow illusion via material tricks.

**Capabilities:**
- Self-shadowing responds to light direction
- Parallax depth when camera moves slightly
- Extremely cheap compared to true volumetrics (200-280 IPP material)
- 1-4MB per effect vs 100MB+ VDB

**Critical limitation:**
> Works for **fixed-camera or background effects only**.
> Depth illusion breaks when camera orbits effect significantly.

**When to use:**
- Background fire/smoke (fireplace, distant volcano)
- 2.5D games or fixed camera angles
- Isometric/top-down views
- Effects behind player (not orbited)

**When NOT to use:**
- Anything the player can walk around
- Projectiles
- Effects in open 3D space with freelook

---

### 3. Multi-Plane / Layered Sprites

**What it is:** Multiple billboard sprites at different depths, creating parallax.

**Capabilities:**
- Depth parallax when camera moves
- Works for limited angle range (~±45°)
- Cheaper than volumetrics

**Setup:**
- 3-5 sprite planes at different Z offsets
- Each plane slightly different (rotation, scale, flipbook frame)
- Depth fade to blend intersections

**When to use:**
- Medium-distance effects
- Limited camera movement scenarios
- Smoke plumes viewed from one general direction

**Limitation:** Obvious layering when viewed perpendicular to planes.

---

### 4. Mesh Particle Clouds

**What it is:** Many small mesh particles (spheres/blobs) forming a cloud.

**Capabilities:**
- True 3D from any angle
- Can cast/receive shadows (expensive)
- Scales with LOD

**Cost:**
- Higher than sprites for same particle count
- 100-500 mesh particles typical

**When to use:**
- Stylized smoke/clouds
- Effects needing 360° viewing
- When translucent mesh overdraw is acceptable

**Material:** Usually unlit/emissive to avoid per-particle lighting cost.

---

### 5. Billboard Sprite Clouds

**What it is:** Classic particle clouds — many sprites, camera-facing.

**Capabilities:**
- Cheap
- Good for diffuse, chaotic volumes (smoke puffs)
- Individual sprite flatness hidden by volume

**When to use:**
- Smoke, dust, fog where individual particles aren't scrutinized
- Fast-dissipating puffs
- Large counts, soft edges

**Limitation:**
- Each sprite is flat — obvious if isolated
- No self-shadowing (unless baked into flipbook)

---

## Fire Specifically

Fire is challenging because it has:
- Defined shape (not diffuse like smoke)
- Self-illumination
- Internal structure visible
- Often viewed as hero element

### Fire Technique Ladder (low to high quality)

| Level | Technique | Tradeoff |
|-------|-----------|----------|
| 1 | Flipbook sprite, additive | Flat, no depth, stylized |
| 2 | Multi-sprite with velocity alignment | Better motion, still flat per sprite |
| 3 | POM + RM flipbook | Depth/shadow, but fixed angle only |
| 4 | Mesh particles + emissive | 3D correct, but chunky without many particles |
| 5 | Mesh core + sprite accents | Hybrid, good balance |
| 6 | True volumetrics | Perfect, but expensive |

**For gameplay fire (torch, campfire):** Level 2-3 if camera is constrained, Level 4-5 if freelook.

**For hero fire (dragon breath, spell):** Level 5-6.

---

## Smoke Specifically

Smoke is more forgiving than fire:
- Diffuse, soft edges
- Individual particles less scrutinized
- Often additive/translucent blend hides flatness

### Smoke Approaches

| Scenario | Technique |
|----------|-----------|
| Background/distant | Flipbook with POM/RM |
| Gameplay, dissipating | Billboard sprite cloud |
| Hero, cinematic | True volumetrics or mesh cloud |
| Rising plume | Layered planes with vertical motion |

---

## Fog/Clouds

### Localized Fog

- **Fixed area:** Exponential height fog + volumetric fog material
- **Moving with object:** Particle system with soft sprites
- **True volume:** HVOL (Heterogeneous Volume)

### Sky Clouds

- UE5 Volumetric Clouds system (built-in)
- Not particle-based — separate system entirely

---

## Common Mistakes

| Mistake | Problem | Fix |
|---------|---------|-----|
| POM fire on rotating object | Depth breaks as angle changes | Use mesh particles or accept limitation |
| Single flipbook for hero fire | Flat, not impressive | Layer techniques: mesh core + sprites |
| Volumetric fog for many small fires | Performance death | Reserve volumetrics for 1-2 hero instances |
| Fully lit mesh particles | Per-particle lighting cost | Use unlit/emissive materials |
| Hard-edge sprites in smoke | Obvious flat planes | Soft, gradient alpha, depth fade |

---

## Decision Flowchart

```
Is camera fixed relative to effect?
├─ Yes → POM + RM is viable (best quality/perf for fixed camera)
└─ No → Can camera fully orbit effect?
         ├─ Yes → Need mesh particles or true volumetrics
         └─ No (limited arc) → Multi-plane layers may work

Is this a hero moment (1-2 instances)?
├─ Yes → True volumetrics acceptable
└─ No → Must use particle-based approach

Is realistic self-shadowing required?
├─ Yes, fixed camera → POM + RM
├─ Yes, any angle → True volumetrics (expensive)
└─ No → Emissive/unlit particles acceptable
```
