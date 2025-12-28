---
name: umg-widget-designer
description: Design-driven UMG widget creation for Unreal Engine using umgMCP tools. ALWAYS use this skill when creating, modifying, or discussing UMG widgets. Enforces mandatory design document review before any widget work—requires UI philosophy document and visual design examples with pictures. Ensures visual consistency, proper component hierarchy, and adherence to established design language.
---

# UMG Widget Designer

## Core Principle

**No widget work without design context.** Every widget must trace back to documented design decisions.

## Mandatory Pre-Flight (Before ANY Widget Work)

### Step 1: Request Design Documents

Before using umgMCP tools, MUST have in context:

1. **UI Philosophy Document** — Design language, principles, aesthetic direction
2. **Visual Design Examples** — Screenshots, mockups, or reference images showing the target look

If not provided, ask:
> "Before I create any widgets, I need your UI design documentation:
> 1. UI philosophy/design language document
> 2. Visual examples or mockups for reference
> 
> Please share these so I can ensure the widgets match your established design."

### Step 2: Extract Design Tokens

From the design documents, identify and confirm:

| Token Type | Examples |
|------------|----------|
| Colors | Background, accent, text, borders, rarity tiers |
| Typography | Font families, sizes, weights |
| Spacing | Margins, padding, gaps |
| Corner Radius | Button rounding, panel corners |
| Effects | Shadows, glows, gradients |

### Step 3: Confirm Before Creating

Before first umgMCP call, state:
> "Based on your design docs, I'll use: [list key tokens]. Creating [widget name] now."

## Widget Creation Workflow

### For Each Widget

1. **Reference the design** — Which mockup/example does this match?
2. **List components** — Break down into UMG primitives
3. **Define states** — Normal, hovered, pressed, disabled, selected
4. **Create incrementally** — One component at a time, test between
5. **Validate visually** — Does it match the reference?

### Component Hierarchy Pattern

```
WBP_[WidgetName]
├── Root (Overlay or Canvas)
│   ├── Background (Image/Border)
│   ├── Content (Vertical/Horizontal Box)
│   │   ├── Header
│   │   ├── Body
│   │   └── Footer
│   └── Interactive Elements
```

### State Management

For interactive widgets, define all states upfront:

| State | Visual Changes |
|-------|----------------|
| Normal | Base appearance |
| Hovered | Highlight, glow, scale |
| Pressed | Darken, inset |
| Disabled | Desaturate, reduce opacity |
| Selected | Border, accent color |
| Focused | Outline for accessibility |

## Design Validation Prompts

Ask yourself during creation:

- Does this match the reference exactly?
- Are colors from the documented palette?
- Is typography consistent with the design system?
- Do interactive states follow established patterns?
- Is the hierarchy clean and maintainable?

## Reference Files

### Widget Patterns
See `references/widget-patterns.md` for:
- Inventory slots with rarity borders
- Tooltip structures
- Stat bars and indicators
- Action buttons with cooldowns
- Panel layouts and containers

### Mockup to UMG Mapping
See `references/mockup-mapper.md` for:
- Design elements → UMG widget translation
- Layout patterns (rows, stacks, grids)
- 9-slice image setup
- Spacing and color application

### Common Gotchas
See `references/common-gotchas.md` for:
- Text issues (auto-wrap for dialogue, RichText line breaks)
- Layout problems (widget not appearing, centering, ScrollBox)
- Anchor/positioning for different resolutions
- Button interaction issues
- Performance pitfalls
- Quick debug checklist

## Anti-Patterns

- Creating widgets without design reference
- Inventing colors/styles not in design docs
- Skipping state definitions for interactive elements
- Deep nesting without clear hierarchy
- Inconsistent spacing or typography

## Integration with unreal-mcp-architect

This skill works alongside the architect skill:
- Architect handles overall structure and Base/Feature organization
- This skill handles visual fidelity and design consistency
- Both enforce incremental, testable steps
