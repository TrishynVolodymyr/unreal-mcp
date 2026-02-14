# Font Tools - Unreal MCP

This document provides comprehensive information about using Font tools through the Unreal MCP (Model Context Protocol). These tools allow you to create and manage font assets in Unreal Engine — including FontFace assets from SDF textures, offline bitmap fonts from texture atlases, TTF imports, and unified font creation — using natural language commands.

## Overview

Font tools enable you to:
- Create FontFace assets from SDF (Signed Distance Field) textures for scalable text rendering
- Configure FontFace properties (hinting, loading policy, ascender/descender)
- Create offline fonts from pre-rendered texture atlases with metrics JSON files
- Import TTF font files directly into Unreal Engine
- Use the unified `create_font` command supporting all source types (TTF, SDF texture, offline)
- Inspect font metadata (cache type, character count, scaling, kerning)

## Natural Language Usage Examples

```
"Create a FontFace called 'FF_DarkFantasy_Regular' from my SDF texture"

"Import the Cinzel-Torn TTF file as a new font asset"

"Create an offline bitmap font from the DarkFantasy atlas texture and metrics JSON"

"Get metadata about Font_DarkFantasy_Regular — how many characters does it have?"

"Set the hinting mode to None on FF_DarkFantasy_Regular"

"Create a font from my TTF file at E:/fonts/Cinzel-Torn.ttf"
```

## Tool Reference

---

### `create_font`

**Unified font creation command** supporting multiple source types. This is the recommended tool for creating font assets.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `font_name` | string | ✅ | Name of the font asset (e.g., `"Font_CinzelTorn"`) |
| `source_type` | string | ✅ | Source type: `"ttf"`, `"sdf_texture"`, or `"offline"` |
| `ttf_file_path` | string | | *(Required for `"ttf"`)* Absolute path to the TTF file |
| `sdf_texture` | string | | *(For `"sdf_texture"`)* Path to the SDF texture in UE |
| `atlas_texture` | string | | *(Required for `"offline"`)* Path to the texture atlas in UE |
| `metrics_file` | string | | *(Required for `"offline"`)* Absolute path to metrics JSON on disk |
| `path` | string | | Asset folder path (default: `"/Game/Fonts"`) |
| `use_sdf` | boolean | | *(For `"sdf_texture"`)* Whether to use SDF rendering (default: True) |
| `distance_field_spread` | number | | *(For `"sdf_texture"`)* SDF spread value (default: 32) |
| `font_metrics` | object | | Optional font metrics dict with `ascender`, `descender`, `line_height` |

**Examples:**
```
# Import a TTF file
create_font(
  font_name="Font_CinzelTorn",
  source_type="ttf",
  ttf_file_path="E:/code/unreal-mcp/Python/font_generation/Cinzel-Torn.ttf"
)

# Create from SDF texture
create_font(
  font_name="Font_DarkFantasy",
  source_type="sdf_texture",
  sdf_texture="/Game/Fonts/DarkFantasy_sdf"
)

# Create offline bitmap font
create_font(
  font_name="Font_DarkFantasy_Regular",
  source_type="offline",
  atlas_texture="/Game/Fonts/DarkFantasy_atlas",
  metrics_file="E:/fonts/DarkFantasy_metrics.json"
)
```

---

### `create_font_face`

Create a new FontFace asset from an SDF texture for high-quality text rendering at any scale.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `font_name` | string | ✅ | Name of the FontFace asset (e.g., `"FF_DarkFantasy_Regular"`) |
| `source_texture` | string | ✅ | Path to the source SDF texture |
| `path` | string | | Asset folder path (default: `"/Game/Fonts"`) |
| `use_sdf` | boolean | | Whether this font uses SDF rendering (default: True) |
| `distance_field_spread` | number | | SDF spread value in pixels (default: 4) |
| `font_metrics` | object | | Optional dict with `ascender`, `descender`, `line_height` |

---

### `set_font_face_properties`

Set properties on an existing FontFace asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `font_path` | string | ✅ | Path to the FontFace asset |
| `properties` | object | ✅ | Dictionary of properties to set |

**Supported properties:**
- `Hinting` — `"Default"`, `"Auto"`, `"AutoLight"`, `"Monochrome"`, `"None"`
- `LoadingPolicy` — `"LazyLoad"`, `"Stream"`, `"Inline"`
- `Ascender` — Font ascender value (float)
- `Descender` — Font descender value (float)
- `SubFaceIndex` — Sub-face index for multi-face fonts (int)

---

### `get_font_face_metadata`

Get metadata about an existing FontFace asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `font_path` | string | ✅ | Path to the FontFace asset |

Returns: `font_path`, `font_name`, `source_filename`, `hinting`, `loading_policy`, `ascender`, `descender`, `sub_face_index`

---

### `create_offline_font`

Create an offline (bitmap/SDF atlas) font from a texture and metrics JSON file.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `font_name` | string | ✅ | Name of the font asset (e.g., `"Font_DarkFantasy_Regular"`) |
| `texture_path` | string | ✅ | Path to the SDF texture atlas in UE |
| `metrics_file_path` | string | ✅ | Absolute file path to the metrics JSON on disk (NOT an Unreal asset path) |
| `path` | string | | Asset folder path (default: `"/Game/Fonts"`) |

The metrics JSON should contain:
- `atlasWidth`, `atlasHeight` — Atlas dimensions in pixels
- `lineHeight`, `baseline` — Font metrics
- `characters` — Object mapping character codes to glyph data (`u`, `v`, `width`, `height`, `xOffset`, `yOffset`, `xAdvance`)

---

### `get_font_metadata`

Get metadata about an existing UFont asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `font_path` | string | ✅ | Path to the font asset |

Returns: `font_path`, `font_name`, `cache_type`, `em_scale`, `ascent`, `descent`, `leading`, `kerning`, `scaling_factor`, `legacy_font_size`, `character_count`, `texture_count`, `is_remapped`

## Advanced Usage Patterns

### Creating a Complete SDF Font Pipeline

```
"Set up a full SDF font from texture:
1. Create a FontFace 'FF_DarkFantasy_Regular' from the SDF texture at /Game/Fonts/DarkFantasy_sdf
2. Set distance field spread to 32 for high-quality rendering
3. Set hinting to None (SDF fonts don't benefit from hinting)
4. Set font metrics: ascender 0.8, descender -0.2
5. Get the metadata to verify everything is configured correctly"
```

### Building a Font Family with Multiple Weights

```
"Create a complete font family:
1. Import the regular TTF file as Font_Cinzel_Regular using create_font with source_type 'ttf'
2. Import the bold TTF file as Font_Cinzel_Bold
3. Import the italic TTF file as Font_Cinzel_Italic
4. Get metadata for each font to verify character counts and metrics
5. Create an SDF version from the DarkFantasy atlas texture for scalable UI use"
```

### Setting Up an Offline Bitmap Font with Atlas

```
"Create a bitmap font from atlas:
1. Create an offline font 'Font_DarkFantasy_Regular' from the atlas texture at /Game/Fonts/DarkFantasy_atlas
2. Use the metrics JSON file at E:/fonts/DarkFantasy_metrics.json for glyph data
3. Get font metadata to verify character count and scaling factor
4. Check that the cache type is 'Offline' and texture count matches expectations"
```

## Best Practices for Natural Language Commands

### Use the Unified create_font Command
Instead of choosing between `create_font_face`, `create_offline_font`, or TTF import, use: *"Create a font called 'Font_CinzelTorn' from TTF source at E:/fonts/Cinzel-Torn.ttf"* — the unified `create_font` tool handles all source types.

### Specify Source Type Explicitly
Always state the source type: *"Create a font from SDF texture"*, *"Import TTF font"*, or *"Create offline bitmap font"* — this prevents ambiguity about which creation path to use.

### Verify Fonts After Creation
After creating any font asset, always: *"Get metadata for Font_DarkFantasy_Regular"* to verify character count, metrics, and cache type are correct before using it in UI widgets.

### Use Absolute Paths for Disk Files
TTF files and metrics JSON files need absolute disk paths: *"Import TTF from E:/fonts/Cinzel-Torn.ttf"* — not Unreal content paths. Texture and font asset references use Unreal paths (e.g., `/Game/Fonts/...`).

### Disable Hinting for SDF Fonts
SDF-rendered fonts bypass traditional hinting. Always: *"Set hinting to None on FF_DarkFantasy_Regular"* for SDF FontFace assets to avoid unnecessary processing.

## Common Use Cases

### Game UI Typography
Importing TTF or creating SDF fonts for main menu titles, HUD text, dialogue boxes, and inventory item names — SDF fonts scale cleanly across different screen resolutions.

### Stylized Game Fonts
Creating custom bitmap fonts from pre-rendered texture atlases for stylized, hand-drawn, or fantasy lettering that can't be achieved with standard TTF fonts.

### Localization-Ready Fonts
Setting up fonts with large character sets that support multiple languages, verifying character count with metadata inspection to ensure all required glyphs are present.

### Performance-Critical UI
Using offline bitmap fonts with pre-rasterized atlases for UI elements that need fast rendering without runtime glyph rasterization overhead.

### Distance Field Text Effects
Creating SDF FontFace assets with configurable spread values for text that supports outlines, glows, drop shadows, and other shader-based effects at any scale.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"File not found" during TTF import**: Ensure the path is absolute and uses the correct separators for your OS. The TTF file must exist on disk at the exact specified path — Unreal content paths (`/Game/...`) won't work for source files.
2. **Font displays blank or missing characters**: Use `get_font_metadata` to check `character_count`. If zero, the metrics file may be malformed or the atlas texture may not be assigned correctly. For offline fonts, verify the metrics JSON structure includes valid `characters` entries.
3. **SDF font looks blurry or has artifacts**: Adjust `distance_field_spread` — too low causes sharp cutoff artifacts, too high causes blurriness. Values between 16-64 work for most cases. Also verify the source SDF texture has adequate resolution.
4. **FontFace properties not applying**: Use `get_font_face_metadata` to verify the current property state. Property names are case-sensitive: `"Hinting"`, `"LoadingPolicy"`, `"Ascender"`, `"Descender"`.
5. **Offline font scaling incorrect**: Check `em_scale` and `scaling_factor` in the metadata. The metrics JSON's `lineHeight` and `baseline` values must match the actual atlas rendering parameters.

## Performance Considerations

- **Prefer SDF fonts for scalable text**: SDF (Signed Distance Field) fonts render at any size from a single texture, avoiding the need for multiple font sizes. This reduces texture memory compared to maintaining separate bitmap fonts for each size.
- **Use offline fonts for fixed-size text**: When text always renders at the same size (e.g., pixel-art UI), offline bitmap fonts skip runtime rasterization entirely and are the fastest rendering path.
- **Keep atlas textures reasonable**: Large font atlases with thousands of characters consume GPU memory. For languages with large character sets, consider using the `"Stream"` loading policy on FontFace assets.
- **Minimize font asset count**: Each unique font asset requires its own texture data in memory. Share font assets across widgets where possible rather than creating duplicates for different UI elements.
