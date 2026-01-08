# Questionnaire Deep Dive

Extended guidance for the discovery phase. Load this reference when handling complex or unusual font requests.

## Conversation Flow Examples

### Example 1: Demonic Inscriptions

**User:** "I need some demonic scribbles for ritual circles in my game"

**Questions to ask:**
1. "Do you have any visual references—concept art, screenshots from other games, or mood images?"
2. "What context will these appear in? Wall carvings, floating magical text, book pages, floor inscriptions?"
3. "Should they be readable (actual letters/words) or purely decorative symbols?"
4. "What mood—ancient evil, chaotic corruption, infernal/hellish, or something else?"

**Follow-ups based on answers:**
- If readable: "Any specific words or phrases you know will be displayed?"
- If decorative: "How many unique symbols do you need? 10-20 for variety, or fewer?"
- If for circles: "Will they animate/rotate? Need seamless tiling?"

### Example 2: UI Header Font

**User:** "I need a font for my dark fantasy RPG's UI"

**Questions to ask:**
1. "Is this for headers/titles only, or body text too?"
2. "Any reference fonts or games whose UI style you admire?"
3. "What's the overall UI aesthetic? We discussed your 'Modern Dark Fantasy' approach with clean gradients and tarnished gold—should the font complement that?"
4. "Any specific words that will be displayed frequently? (Item names, menu labels, etc.)"

**Follow-ups:**
- "Will this pair with another font for body text?"
- "Need bold/light variants, or single weight?"
- "Any specific characters beyond standard Latin? Currency symbols, special icons?"

### Example 3: Faction-Specific Alphabet

**User:** "I want a unique alphabet for an ancient civilization in my game"

**Questions to ask:**
1. "Should players be able to 'decode' it (consistent letter mapping) or is it purely visual?"
2. "Any real-world writing system influences? (Cuneiform, Mayan, Elvish/Tolkien, etc.)"
3. "Carved in stone, written with brush, or something else?"
4. "How often will players see it? Background flavor or important puzzle element?"

**Follow-ups:**
- "How many unique symbols? Full 26+ letter equivalent or smaller set?"
- "Need numerals in this system too?"
- "Should it feel geometric/precise or organic/handwritten?"

## Probing Questions by Font Purpose

### For Readability-Critical Fonts (UI, Dialogue)

- "What's the smallest size this will be displayed at?"
- "Light text on dark, or dark on light?"
- "Need to work at 1080p, 4K, or both?"
- "Long text passages or short labels?"
- "Any accessibility considerations?"

### For Decorative/Atmospheric Fonts (Inscriptions, Titles)

- "Is readability secondary to mood?"
- "Should it feel ancient, magical, corrupted, or something else?"
- "Will it have glow/particle effects applied in-engine?"
- "Static display or animated/revealed?"

### For Symbol Sets (Runes, Icons, Glyphs)

- "How many unique symbols needed?"
- "Any specific meanings to encode? (Elements, factions, stats)"
- "Should they share a consistent visual grammar?"
- "Used alone or combined into sequences?"

## Visual Reference Analysis Checklist

When user provides an image, systematically analyze:

### Structural Properties
- [ ] **Weight**: Thin / Light / Regular / Bold / Black
- [ ] **Width**: Condensed / Normal / Extended
- [ ] **Contrast**: High (thick/thin variation) / Low (monolinear)
- [ ] **x-height**: Tall / Normal / Short (if lowercase exists)

### Stylistic Properties
- [ ] **Serifs**: None / Bracketed / Slab / Wedge / Hairline
- [ ] **Terminals**: Rounded / Flat / Pointed / Ball / Tear-drop
- [ ] **Axis**: Vertical / Angled
- [ ] **Corners**: Sharp / Rounded / Mixed

### Texture & Treatment
- [ ] **Edge quality**: Clean / Rough / Eroded / Organic
- [ ] **Surface**: Solid / Textured / Distressed / Gradient
- [ ] **Effects**: None / Inline / Outline / Shadow / Glow

### Mood Descriptors
Choose 2-3 that apply:
Ancient, Modern, Elegant, Brutal, Corrupted, Sacred, Mysterious, Chaotic, Refined, Industrial, Organic, Geometric, Whimsical, Threatening, Noble, Primitive, Futuristic, Classical

## Red Flags & Clarifications

Watch for these and ask follow-up questions:

| User says | Potential issue | Clarify with |
|-----------|-----------------|--------------|
| "Something unique" | Too vague | "Unique how? Structurally unusual, or just distinctive style?" |
| "Like [famous font]" | May want exact copy | "Inspired by, or as close as possible? Any changes you'd want?" |
| "Easy to read but cool" | Contradictory priorities | "If you had to choose, which matters more for this use case?" |
| "All the characters" | Scope unclear | "Full Latin extended, or just A-Z, 0-9, basic punctuation?" |
| "It should look old" | Many interpretations | "Ancient carved stone, faded medieval manuscript, or weathered metal?" |

## Minimum Viable Questions

If user wants to move fast, these are the essential questions:

1. **What's it for?** (UI / inscriptions / titles / other)
2. **Any visual reference?** (image, font name, game example)
3. **What mood?** (one or two words)
4. **Which characters?** (offer presets: minimal/standard/extended/custom)

Everything else can be reasonable defaults or offered as refinement after initial generation.
