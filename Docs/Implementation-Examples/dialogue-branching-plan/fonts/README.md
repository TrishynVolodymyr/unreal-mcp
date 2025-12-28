# Dark Fantasy SDF Font

Scalable SDF (Signed Distance Field) font based on Cinzel, optimized for Unreal Engine 5.

## Generated Files

| File | Description |
|------|-------------|
| `DarkFantasy_Regular_sdf.png` | Regular weight SDF atlas (4096x4096) |
| `DarkFantasy_Regular_metrics.json` | Regular weight character metrics |
| `DarkFantasy_Bold_sdf.png` | Bold weight SDF atlas (4096x4096) |
| `DarkFantasy_Bold_metrics.json` | Bold weight character metrics |

## Character Set

- **Uppercase:** A-Z (26 characters)
- **Lowercase:** a-z (26 characters)
- **Numbers:** 0-9 (10 characters)
- **Punctuation:** ` .,!?:;'"-_` (11 characters)
- **Total:** 73 glyphs per weight

## UE5 Import Instructions

### Option 1: UMG Runtime Font (Recommended)

1. **Import atlas texture:**
   - Drag `DarkFantasy_Regular_sdf.png` into Content Browser
   - In texture settings:
     - Compression: `UserInterface2D` or `BC7`
     - Mip Gen Settings: `NoMipmaps`

2. **Create Font Face:**
   - Right-click > User Interface > Font
   - Enable "Use SDF"
   - Set Distance Field Spread = 32

3. **Configure metrics:**
   - Use values from `*_metrics.json` for character UVs and advances

### Option 2: Custom SDF Material

For more control over rendering (outlines, glow, shadows):

1. **Import atlas texture** (same as above)

2. **Create SDF Material:**
```hlsl
// Sample the SDF atlas
float Distance = Texture2DSample(FontAtlas, UV).r;

// Basic rendering (0.5 = edge)
float Smoothing = fwidth(Distance);
float Alpha = smoothstep(0.5 - Smoothing, 0.5 + Smoothing, Distance);

// Output
return float4(FontColor.rgb, Alpha * FontColor.a);
```

3. **For outline effect:**
```hlsl
float OutlineWidth = 0.1;
float OutlineAlpha = smoothstep(0.5 - OutlineWidth - Smoothing,
                                 0.5 - OutlineWidth + Smoothing, Distance);
// Blend outline behind main text
```

4. **For glow effect:**
```hlsl
float GlowWidth = 0.2;
float GlowAlpha = smoothstep(0.5 - GlowWidth, 0.5, Distance);
// Apply glow color with GlowAlpha
```

## SDF Benefits

- **Perfect scaling** - Crisp at any size (unlike regular bitmap fonts)
- **Easy effects** - Outline, glow, shadow via material
- **Small file size** - One atlas works for all sizes
- **GPU accelerated** - Rendering done in shader

## Metrics JSON Format

```json
{
  "name": "DarkFantasy_Regular",
  "type": "sdf",
  "spread": 32,
  "atlasWidth": 4096,
  "atlasHeight": 4096,
  "lineHeight": 345,
  "baseline": 288,
  "characters": {
    "65": {  // Character code for 'A'
      "u": 0.0,      // UV left
      "v": 0.0,      // UV top
      "u2": 0.046,   // UV right
      "v2": 0.070,   // UV bottom
      "width": 191,  // Pixel width
      "height": 288, // Pixel height
      "xOffset": 0,
      "yOffset": 0,
      "xAdvance": 193
    }
  }
}
```

## Source Font

Based on [Cinzel](https://fonts.google.com/specimen/Cinzel) by Natanael Gama.
Licensed under SIL Open Font License 1.1.
