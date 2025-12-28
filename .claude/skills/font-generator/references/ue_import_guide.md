# Unreal Engine Font Import Reference

Quick reference for importing generated fonts in UE5. **SDF is the recommended approach.**

## SDF Fonts (RECOMMENDED)

SDF (Signed Distance Field) fonts scale perfectly at any size—the industry standard for game UI.

### Why SDF?
- **Perfect scaling** — crisp at any size, no blur
- **Easy effects** — outlines, glows, shadows via material
- **GPU efficient** — single texture, shader does the work
- **Small footprint** — one atlas for all sizes

### Import Workflow

1. **Import atlas texture**
   - Drag `FontName_sdf.png` into Content Browser
   - Compression: None or BC7 (preserve quality)
   - Filter: Bilinear
   - sRGB: Off (it's distance data, not color)

2. **Create Font Face asset**
   - Right-click → User Interface → Font Face
   - Set Font Filename to your atlas
   - Enable **"Use SDF"** checkbox
   - Set appropriate SDF spread (match your generation setting, typically 32)

3. **Create Font asset**
   - Right-click → User Interface → Font
   - Add Font Face to the font
   - Configure fallback fonts if needed

### SDF Material (for custom effects)

Basic SDF shader logic:
```hlsl
// Sample distance from atlas
float dist = Texture2DSample(FontAtlas, UV).r;

// Anti-aliased edge
float smoothing = fwidth(dist) * 0.5;
float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);

// Output
return float4(FontColor.rgb, alpha * FontColor.a);
```

**Outline effect:**
```hlsl
float outline = smoothstep(0.4 - smoothing, 0.4 + smoothing, dist);
float inner = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
float3 color = lerp(OutlineColor, FontColor, inner);
return float4(color, outline);
```

**Glow effect:**
```hlsl
float glow = smoothstep(0.3, 0.5, dist);
float core = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
// Blend glow behind core
```

## Bitmap Fonts (Non-SDF)

For pixel art or fixed-size scenarios only.

### Import Workflow
1. Import atlas texture (Point filtering for pixel art)
2. Create Slate Bitmap Font asset
3. Configure character regions from metrics JSON

### Settings
- Filtering: **Point** for pixel fonts, Bilinear otherwise
- Compression: None (preserve sharp edges)
- No mipmaps for fixed-size display

## Common Issues & Solutions

| Problem | Cause | Solution |
|---------|-------|----------|
| Blurry text | Wrong filtering or scaling | Use SDF, or Point filter for pixel fonts |
| Jagged edges | SDF spread too low | Regenerate with higher spread (48-64) |
| Thick/thin text | Wrong threshold | Adjust 0.5 threshold in material |
| Missing chars | Character not in atlas | Check metrics JSON, regenerate atlas |
| Wrong spacing | Metrics mismatch | Verify xAdvance values in metrics |

## Recommended Settings by Use Case

### UI Text (Headers, Labels, Body)
- **Format:** SDF atlas
- **Resolution:** 2048x2048
- **Spread:** 32
- Use UMG's built-in SDF rendering

### 3D World Text
- **Format:** SDF atlas
- **Material:** Custom SDF material
- **Extras:** Distance-based fade, billboard optional

### Decorative/Inscriptions
- **Format:** SDF for scalable, Bitmap for textured
- Consider normal maps for carved effect
- May use individual textures for very unique glyphs

### Pixel Art Style
- **Format:** Bitmap atlas (NOT SDF)
- **Filtering:** Point
- **Scaling:** Integer only
- No anti-aliasing

## File Organization

Recommended structure:
```
/Content/UI/Fonts/
├── Atlases/
│   ├── T_MainUI_SDF.uasset
│   ├── T_Decorative_SDF.uasset
│   └── T_Runes_SDF.uasset
├── FontFaces/
│   ├── FF_MainUI.uasset
│   └── FF_Decorative.uasset
├── Fonts/
│   ├── F_MainUI.uasset
│   └── F_Decorative.uasset
└── Materials/
    ├── M_SDFFont_Base.uasset
    ├── M_SDFFont_Outline.uasset
    └── M_SDFFont_Glow.uasset
```
