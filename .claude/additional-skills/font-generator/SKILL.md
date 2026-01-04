---
name: font-generator
description: Generate custom fonts and typography for Unreal Engine projects. ALWAYS use this skill when the user asks to create fonts, typography, lettering, glyphs, runes, inscriptions, or custom text styles for games. Triggers on requests for game fonts, demonic/magical writing, ancient scripts, stylized UI text, decorative lettering, bitmap fonts, or any custom typeface creation. Emphasizes thorough discovery through questionnaire before generation.
---

# Font Generator for Unreal Engine

Generate custom fonts through comprehensive discovery and iterative refinement. The questionnaire-driven approach ensures fonts match the user's exact vision before generation begins.

## Core Workflow

1. **Discovery Phase** - Gather requirements through structured questionnaire
2. **Reference Analysis** - Analyze any visual examples provided
3. **Concept Proposal** - Present font concept for approval
4. **Generation** - Create font assets
5. **Delivery** - Export in UE-compatible format

## Discovery Phase: Questionnaire

Before generating ANY font, collect information across these categories. Ask questions conversationally (2-3 at a time, not all at once). Adapt based on the use case complexity.

### Visual Reference (PRIORITY - Ask First)

Always ask if the user has visual references:

- "Do you have any reference images—fonts you like, screenshots, concept art, or mood boards?"
- "Can you share an image showing the style you're aiming for?"
- "Are there any existing fonts (even partial matches) I should look at?"

If they upload an image: analyze it for weight, contrast, serifs, decorative elements, texture, and mood. Describe back what you observe and confirm.

### Use Case & Scope

Determine what the font is for:

| Question | Why It Matters |
|----------|----------------|
| What will this font be used for? (UI headers, body text, inscriptions, signage, item names, magical effects, etc.) | Determines readability requirements |
| Is this for a specific element or broader game-wide use? | Scope affects completeness of character set |
| Will it be displayed at large sizes, small sizes, or both? | Affects detail level and stroke weight |
| Does it need to animate or have effects applied? | May need cleaner outlines or SDF-ready design |

### Style & Aesthetic

Establish the visual direction:

| Question | Options/Examples |
|----------|------------------|
| What's the game's genre/setting? | Dark fantasy, sci-fi, horror, medieval, steampunk, cosmic horror, etc. |
| What mood should the font convey? | Ominous, elegant, chaotic, ancient, corrupted, sacred, brutal, refined |
| Serif, sans-serif, or decorative? | Or hybrid, or doesn't matter |
| Any specific cultural/historical influences? | Norse runes, Gothic blackletter, Art Deco, Japanese calligraphy, Lovecraftian, etc. |
| Thick/bold or thin/delicate strokes? | Or variable weight |
| Should it look handwritten, carved, printed, or magical? | Affects texture and edge treatment |

### Character Set Requirements

Determine what characters are needed:

| Question | Impact |
|----------|--------|
| Latin alphabet only, or extended characters? | A-Z vs accented characters, symbols |
| Numbers needed? | 0-9 |
| Special symbols or punctuation? | !@#$%^&*(),.?":;' etc. |
| Any custom glyphs? | Faction symbols, runes, icons |
| Case requirements? | Uppercase only, both cases, small caps |

**Quick presets to offer:**
- **Minimal**: A-Z uppercase only (26 glyphs)
- **Standard**: A-Z, a-z, 0-9, basic punctuation (~70 glyphs)
- **Extended**: Standard + accented characters (~150 glyphs)
- **Custom glyphs only**: User-specified symbols/runes (variable)

### Technical Requirements

For UE implementation:

| Question | Default |
|----------|---------|
| Output format preference? | TTF (most compatible) |
| Need SDF/MSDF for distance field rendering? | Usually yes for 3D/effects |
| Target texture resolution? | 1024x1024 or 2048x2048 |
| Specific UE font face asset requirements? | Standard import workflow |

### Pairing & Context

If part of a larger system:

- "Will this font be used alongside other fonts? Which ones?"
- "Should it contrast with or complement existing UI fonts?"
- "Is there a primary font this should work with as secondary/accent?"

## Reference Analysis

When user provides visual references:

1. **Identify key characteristics:**
   - Stroke weight and contrast
   - Terminal style (rounded, sharp, serif, etc.)
   - x-height and proportions
   - Decorative elements
   - Texture/distress level
   - Overall mood

2. **Describe observations back to user:**
   > "I see a heavy blackletter style with sharp angular terminals, high stroke contrast, and subtle weathering. The mood feels ancient and ominous. Is this accurate?"

3. **Clarify any ambiguities:**
   > "The reference shows both clean and distressed versions—which direction do you prefer?"

## Generation Approaches

Select approach based on requirements. **SDF Bitmap is the recommended default** — it's reliable, scalable, and industry-standard.

### Approach A: SDF Bitmap Font (RECOMMENDED)

**Best for:** Almost everything — UI text, headers, 3D text, scalable typography

**Why SDF:**
| Regular Bitmap | SDF Bitmap |
|----------------|------------|
| Stores pixel colors | Stores distance to edge |
| Blurry when scaled | Crisp at ANY size |
| Simple rendering | GPU reconstructs sharp edges |

Process:
1. Generate high-res glyph images (256px+ height recommended)
2. Run through msdfgen to create SDF/MSDF atlas
3. Import to UE5 with SDF enabled
4. Automatic crisp scaling + easy outline/glow effects

See `scripts/generate_sdf_atlas.py` for the full pipeline.

**UE5 Integration:**
- Enable "Use SDF" on Font Face asset, OR
- Use Runtime Font (SDF) for more control
- SDF also enables easy outline/glow/shadow effects via material

### Approach B: Standard Bitmap Atlas

**Best for:** Pixel art fonts, fixed-size UI, when SDF is overkill

Process:
1. Generate glyph images at target display size
2. Pack into texture atlas
3. Create font metrics JSON
4. Import to UE as Slate Bitmap Font

See `scripts/bitmap_font_atlas.py` for atlas generation.

**Note:** Will blur/pixelate if scaled. Only use for truly fixed-size scenarios.

### Approach C: Image-Based Glyphs

**Best for:** Unique symbols, runes, magical inscriptions (not full alphabet)

Process:
1. Generate individual glyph images
2. Export as transparent PNGs
3. User imports as textures/sprites in UE

### Approach D: Vector Font (TTF/OTF) — Direct Generation

**Best for:** When you need a system-installable font file or true vector scalability

**Method:** Use `fonttools` Python library to generate TTF directly from path data. **Do NOT use SVG as intermediate format** — SVG-to-TTF conversion is error-prone.

See **"TTF Generation: Technical Reference"** section below for:
- Direct TTF creation with fonttools
- Critical winding order requirements
- Why SVG conversion fails
- Code examples

**When to use TTF vs SDF:**
| TTF (Vector) | SDF (Bitmap) |
|--------------|--------------|
| System font installation needed | In-game UI only |
| External tool compatibility | UE-native rendering |
| True vector editing later | Easy outline/glow effects |
| Complex path work | More forgiving workflow |

## Delivery Checklist

Provide to user:

- [ ] Font file(s) in requested format (TTF, atlas, etc.)
- [ ] Preview image showing full character set
- [ ] Any source files (SVG, PSD) if requested
- [ ] Brief usage notes for UE import
- [ ] Variants if created (regular, bold, distressed)

## Style Reference Quick Cards

Use these as starting points based on common requests:

**Dark Fantasy / Souls-like:**
- Heavy serifs or blackletter influence
- Weathered, ancient feel
- Muted gold, bone, or blood accents
- Slightly uneven baseline

**Demonic / Occult:**
- Sharp angular forms
- Asymmetric, unsettling shapes
- Dripping, corrupted, or burning effects
- Custom sigil-like alternate glyphs

**Ancient / Runic:**
- Geometric, carved appearance
- Consistent stroke width
- Angular rather than curved
- Based on real runic systems or invented

**Eldritch / Cosmic Horror:**
- Organic, tentacle-like curves
- Unsettling proportions
- Letters that feel "wrong"
- May be intentionally hard to read

**Medieval / Historical:**
- Blackletter (Fraktur, Textura)
- Uncial or Carolingian influences
- Illuminated capital potential
- Period-appropriate ornamentation

**Magical / Arcane:**
- Flowing, calligraphic strokes
- Integrated symbols (stars, circles)
- Glowing/ethereal weight
- Often paired with more readable font

## TTF Generation: Technical Reference

### Direct TTF Creation with fonttools (Python)

For programmatic TTF generation, use `fonttools` library directly instead of SVG intermediates. This avoids common SVG-to-font conversion failures.

**Location:** `Python/font_generation/ttf_from_scratch.py`

```python
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

fb = FontBuilder(1000, isTTF=True)  # 1000 units per em
fb.setupGlyphOrder([".notdef", "space", "A", "B"])
fb.setupCharacterMap({0x0041: "A", 0x0042: "B"})

# Draw glyph with pen
pen = TTGlyphPen(None)
pen.moveTo((100, 0))
pen.lineTo((300, 700))
# ... more path commands
pen.closePath()
glyphs["A"] = pen.glyph()

fb.setupGlyf(glyphs)
fb.save("MyFont.ttf")
```

### Critical Technical Requirements

#### 1. Winding Order (Most Common Failure Point)

TrueType uses **non-zero winding rule** for fill:

| Contour Type | Winding Direction | Result |
|--------------|-------------------|--------|
| Outer shape | Clockwise | Filled (solid) |
| Inner hole | Counter-clockwise | Empty (hole) |

**If holes disappear:** The inner contour has wrong winding direction.

```python
# OUTER (clockwise in font coords - Y up)
pen.moveTo((50, 0))
pen.lineTo((50, 700))      # Up
pen.lineTo((500, 700))     # Right
pen.lineTo((500, 0))       # Down
pen.closePath()

# HOLE (counter-clockwise - opposite direction)
pen.moveTo((150, 100))
pen.lineTo((400, 100))     # Right first (opposite of outer)
pen.lineTo((400, 600))     # Up
pen.lineTo((150, 600))     # Left
pen.closePath()
```

#### 2. Coordinate System

| System | Y-Axis | Origin |
|--------|--------|--------|
| Fonts (TrueType) | Y grows **UP** | Baseline at Y=0 |
| SVG | Y grows **DOWN** | Top-left at (0,0) |
| Screen/Images | Y grows **DOWN** | Top-left at (0,0) |

When converting from SVG or image coordinates, **flip Y axis**:
```python
font_y = glyph_height - svg_y
```

#### 3. Path Requirements

| Requirement | What Happens If Violated |
|-------------|--------------------------|
| Paths must be **closed** (`closePath()`) | Glyph doesn't render |
| No self-intersecting paths | Incorrect fills |
| Holes must be **separate contours** in same glyph | Holes become solid |
| All coordinates as integers or floats | Rendering errors |

#### 4. Required Font Tables

FontBuilder handles these, but for reference:
- `glyf` - Glyph outlines
- `cmap` - Character to glyph mapping
- `head` - Font header (units per em, bounds)
- `hhea` - Horizontal metrics header
- `hmtx` - Horizontal metrics per glyph
- `maxp` - Maximum profile
- `name` - Font naming
- `OS/2` - OS/2 and Windows metrics
- `post` - PostScript info

### Why SVG-to-TTF Often Fails

Common SVG conversion problems:

1. **Mirrored paths** reverse winding order → holes disappear
2. **Strokes not converted to fills** → strokes disappear
3. **Nested transforms** not flattened → wrong positions
4. **`<use>` and `<symbol>` references** → elements missing
5. **Wrong viewBox mapping** → glyphs off-baseline
6. **Open paths** → glyphs don't render

**Recommendation:** For reliable TTF output, draw directly to fonttools pens instead of SVG intermediate.

### Approaches for Different Font Types

| Font Type | Recommended Approach |
|-----------|---------------------|
| Unique artistic font | Manual glyph definition or AI-assisted |
| Variations of a style | Parametric system with base paths |
| Quick prototype | Modify existing open-source font (OFL) |
| Icon/symbol set | Direct pen drawing (fewer glyphs) |
| Full alphabet (A-Z, a-z, 0-9) | Significant effort - 70+ glyph definitions |

### Testing Generated Fonts

```python
from PIL import ImageFont, Image, ImageDraw

font = ImageFont.truetype("MyFont.ttf", 48)
img = Image.new('RGB', (400, 100), 'white')
draw = ImageDraw.Draw(img)
draw.text((10, 10), "ABC", font=font, fill='black')
img.save("test_render.png")
```

## Important Notes

- Always confirm style direction BEFORE generating
- Show concept/preview and get approval before full character set
- Provide iteration opportunities—fonts often need refinement
- For very custom needs, may generate fewer initial glyphs as proof-of-concept
- Recommend testing in UE early with placeholder if font is complex
- **For TTF output:** Use fonttools direct generation, not SVG conversion
