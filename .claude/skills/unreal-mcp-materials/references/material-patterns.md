# Material Patterns Reference

Concrete node graph recipes for common material types. Each pattern shows exact expressions and connections.

---

## Standard PBR Material

Basic physically-based material with packed textures.

### Expressions to Add

```
TextureSampleParameter2D  name="BaseColorTex"
TextureSampleParameter2D  name="NormalTex"
TextureSampleParameter2D  name="ORMTex"    // Occlusion, Roughness, Metallic packed
```

### Connections

```
BaseColorTex.RGB → BaseColor
NormalTex.RGB → Normal
ORMTex.R → AmbientOcclusion
ORMTex.G → Roughness  
ORMTex.B → Metallic
```

### Property Settings

```
NormalTex → SamplerType: Normal
```

---

## Parameterized PBR (Instance-Ready)

Master material with exposed parameters for variation.

### Expressions to Add

```
TextureSampleParameter2D  name="BaseColorTex"
TextureSampleParameter2D  name="NormalTex"
TextureSampleParameter2D  name="ORMTex"
VectorParameter           name="ColorTint"         default=(1,1,1,1)
ScalarParameter           name="RoughnessMultiplier" default=1.0
Multiply                  // For tint
Multiply                  // For roughness
```

### Connections

```
BaseColorTex.RGB → Multiply(A)
ColorTint.RGB → Multiply(B)
Multiply → BaseColor

ORMTex.G → Multiply(A)
RoughnessMultiplier → Multiply(B)
Multiply → Roughness

NormalTex.RGB → Normal
ORMTex.R → AmbientOcclusion
ORMTex.B → Metallic
```

---

## Masked Material (Vegetation/Decals)

For alpha-tested materials like leaves, grass, decals.

### Material Settings
- Blend Mode: `Masked`
- Two Sided: `true` (for foliage)

### Expressions to Add

```
TextureSampleParameter2D  name="BaseColorTex"
TextureSampleParameter2D  name="NormalTex"
ScalarParameter           name="OpacityMaskClip"  default=0.5
```

### Connections

```
BaseColorTex.RGB → BaseColor
BaseColorTex.A → OpacityMask
NormalTex.RGB → Normal
```

---

## Emissive Material

Glowing/self-illuminated surfaces.

### Expressions to Add

```
TextureSampleParameter2D  name="BaseColorTex"
TextureSampleParameter2D  name="EmissiveTex"
VectorParameter           name="EmissiveColor"     default=(1,0.5,0,1)
ScalarParameter           name="EmissiveIntensity" default=5.0
Multiply                  // EmissiveTex × Color
Multiply                  // Result × Intensity
```

### Connections

```
BaseColorTex.RGB → BaseColor

EmissiveTex.RGB → Multiply(A)
EmissiveColor.RGB → Multiply(B)
Multiply → Multiply(A)
EmissiveIntensity → Multiply(B)
Multiply → EmissiveColor
```

---

## Pulsing Emissive (Animated)

Glowing material with time-based animation.

### Expressions to Add

```
VectorParameter  name="EmissiveColor"     default=(1,0.2,0,1)
ScalarParameter  name="PulseSpeed"        default=2.0
ScalarParameter  name="EmissiveMin"       default=1.0
ScalarParameter  name="EmissiveMax"       default=10.0
Time
Multiply         // Time × Speed
Sine
Add              // Shift sine 0-1 range
Multiply         // Scale to 0.5
Lerp             // Min to Max
Multiply         // Color × Intensity
```

### Connections

```
Time → Multiply(A)
PulseSpeed → Multiply(B)
Multiply → Sine
Sine → Add(A)
Constant(1.0) → Add(B)
Add → Multiply(A)
Constant(0.5) → Multiply(B)
Multiply → Lerp(Alpha)
EmissiveMin → Lerp(A)
EmissiveMax → Lerp(B)
Lerp → Multiply(A)
EmissiveColor.RGB → Multiply(B)
Multiply → EmissiveColor
```

---

## Fresnel Rim Effect

Edge glow/highlight effect.

### Expressions to Add

```
Fresnel         ExponentIn=4.0
VectorParameter name="RimColor"      default=(0.5,0.8,1,1)
ScalarParameter name="RimIntensity"  default=2.0
Multiply        // Fresnel × Color
Multiply        // Result × Intensity
```

### Connections

```
Fresnel → Multiply(A)
RimColor.RGB → Multiply(B)
Multiply → Multiply(A)
RimIntensity → Multiply(B)
Multiply → EmissiveColor
```

---

## World-Aligned Texture

Tiling based on world position, not UVs. Good for terrain, cliffs.

### Expressions to Add

```
WorldPosition
TextureSampleParameter2D  name="DiffuseTex"
Divide                     // Scale world position
ComponentMask              // Get XY or XZ
```

### Connections

```
WorldPosition → Divide(A)
Constant(512.0) → Divide(B)       // Tile size in units
Divide → ComponentMask(XY)        // Top-down projection
ComponentMask → DiffuseTex.UVs
DiffuseTex.RGB → BaseColor
```

---

## Triplanar Mapping

Three-way projection for complex geometry.

### Expressions to Add

```
WorldPosition
WorldNormal (AbsoluteWorldPosition node)
TextureSampleParameter2D  name="DiffuseTex"     // Sample 3 times
Divide
ComponentMask             // XY, XZ, YZ projections
Abs
Power                     // Blend sharpness
Lerp                      // Blend projections
```

### High-Level Flow

1. Sample texture using XY (top/bottom)
2. Sample texture using XZ (front/back)
3. Sample texture using YZ (left/right)
4. Use world normal to blend between samples

---

## Vertex Color Masking

Blend materials using painted vertex colors.

### Expressions to Add

```
VertexColor
TextureSampleParameter2D  name="Material1_Base"
TextureSampleParameter2D  name="Material2_Base"
Lerp
```

### Connections

```
Material1_Base.RGB → Lerp(A)
Material2_Base.RGB → Lerp(B)
VertexColor.R → Lerp(Alpha)
Lerp → BaseColor
```

### Usage
Paint vertex colors in mesh editor: Red = Material2, Black = Material1

---

## Distance Blend (LOD Transition)

Blend between detail and simple versions based on camera distance.

### Expressions to Add

```
CameraPosition
WorldPosition
Distance
Divide           // Normalize distance
Saturate
Lerp
```

### Connections

```
Distance(CameraPosition, WorldPosition) → Divide(A)
Constant(2000.0) → Divide(B)        // Transition distance
Divide → Saturate
Saturate → Lerp(Alpha)
DetailMaterial → Lerp(A)
SimpleMaterial → Lerp(B)
Lerp → BaseColor
```

---

## Detail Normal Blending

Combine tiled detail normal with base normal.

### Expressions to Add

```
TextureSampleParameter2D  name="BaseNormal"
TextureSampleParameter2D  name="DetailNormal"
BlendAngleCorrectNormal   // Or use Reoriented Normal Blending
TextureCoordinate         // For detail tiling
Multiply                  // Scale detail UVs
```

### Connections

```
TextureCoordinate → Multiply(A)
Constant(4.0) → Multiply(B)           // Detail tile amount
Multiply → DetailNormal.UVs

BaseNormal.RGB → BlendAngleCorrectNormal(A)
DetailNormal.RGB → BlendAngleCorrectNormal(B)
BlendAngleCorrectNormal → Normal
```

---

## Subsurface Material

For skin, wax, leaves with light transmission.

### Material Settings
- Shading Model: `Subsurface`

### Expressions to Add

```
TextureSampleParameter2D  name="BaseColorTex"
VectorParameter           name="SubsurfaceColor"  default=(1,0.2,0.1,1)
ScalarParameter           name="SubsurfaceOpacity" default=0.5
```

### Connections

```
BaseColorTex.RGB → BaseColor
SubsurfaceColor.RGB → SubsurfaceColor
SubsurfaceOpacity → Opacity
```

---

## Translucent Material

For glass, water, effects.

### Material Settings
- Blend Mode: `Translucent`
- Lighting Mode: `Surface TranslucencyVolume` (for lit) or `Unlit`

### Expressions to Add

```
VectorParameter   name="Color"      default=(0.8,0.9,1,0.5)
ScalarParameter   name="Opacity"    default=0.3
ScalarParameter   name="Refraction" default=1.05
Fresnel
Lerp              // Fresnel-based opacity
```

### Connections

```
Color.RGB → BaseColor
Opacity → Lerp(A)
Constant(0.9) → Lerp(B)
Fresnel → Lerp(Alpha)
Lerp → Opacity
Refraction → Refraction
```

---

## Quick Recipes

### Flat Color (Unlit)
```
Shading Model: Unlit
VectorParameter "Color" → EmissiveColor
```

### Metal (Full Metallic)
```
Constant(1.0) → Metallic
ScalarParameter "Roughness" default=0.2 → Roughness
```

### Plastic (Non-Metal)
```
Constant(0.0) → Metallic
ScalarParameter "Roughness" default=0.4 → Roughness
```

### Mirror
```
Constant(1.0) → Metallic
Constant(0.0) → Roughness
```
