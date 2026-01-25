# Skill Discovery Reference

## Frontmatter Extraction

Skills use YAML frontmatter format:

```yaml
---
name: skill-name
description: >
  Multi-line description with triggers and capabilities.
  This is what PM reads to match skills to requests.
---
```

## Discovery Command

To scan all available skills:

```bash
# Find all SKILL.md files
find /mnt/skills -name "SKILL.md" -type f 2>/dev/null

# Extract frontmatter from a single skill
sed -n '/^---$/,/^---$/p' /path/to/SKILL.md | head -50
```

## Skill Locations

| Path | Type | Description |
|------|------|-------------|
| `/mnt/skills/user/` | User skills | Custom skills added by user |
| `/mnt/skills/public/` | Public skills | Core document/file skills |
| `/mnt/skills/examples/` | Example skills | Reference implementations |

## Registry Building

When PM starts, build in-memory registry:

```
For each SKILL.md found:
  1. Read first 50 lines (frontmatter is always at top)
  2. Extract name and description
  3. Store in registry:
     {
       "skill-name": {
         "path": "/mnt/skills/user/skill-name/SKILL.md",
         "description": "..."
       }
     }
```

## Matching Algorithm

1. **Keyword extraction** from user request
2. **Description search** for matching terms
3. **Category inference** from skill description:
   - "architect", "designer", "create" → Implementer
   - "director", "design authority", "decisions" → Brain/Designer
   - "extractor", "metadata", "inspect" → Extractor
   - "debugger", "investigator", "linter" → Analyzer

## Known Skill Categories

Based on current skill ecosystem:

### Brains (Design Decisions)
- `vfx-technical-director` — VFX technique selection

### Implementers (Build Things)
- `niagara-vfx-architect` — Particle systems
- `unreal-mcp-materials` — Materials
- `umg-widget-designer` — UI widgets
- `animation-blueprint-architect` — Animation systems
- `datatable-schema-designer` — Data tables

### Extractors (Read State)
- `asset-state-extractor` — Get asset metadata

### Analyzers (Debug/Inspect)
- `blueprint-linter` — Quality analysis
- `unreal-mcp-debugger` — Debug investigation
- `crash-investigator` — Crash analysis
- `ue-log-analyst` — Log parsing

### Utilities
- `fluidninja-vfx` — FluidNinja workflow guide
- `vfx-texture-generator` — Procedural textures
- `metasound-sound-designer` — Audio design
- `font-generator` — Font creation

### Meta
- `skill-creator` — Create new skills
- `product-self-knowledge` — Anthropic product info

## Handling Missing Skills

If request needs capability not covered by any skill:

1. Check if partially covered (skill does 70% of what's needed)
2. Suggest skill creation if pattern will repeat
3. Offer manual guidance as fallback
