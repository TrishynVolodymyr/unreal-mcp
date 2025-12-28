# Widget Patterns Reference

Common UMG widget structures for RPG and dark fantasy UI.

## Inventory Slot

```
WBP_InventorySlot
├── Overlay (Root)
│   ├── Border_Rarity          # Color based on item tier
│   │   └── Image_Background   # Slot background
│   ├── Image_Icon             # Item icon
│   ├── Text_Quantity          # Stack count (bottom-right)
│   └── Image_Highlight        # Hover/selection state
```

**States:**
- Empty: Dim background, no icon
- Filled: Show icon, quantity if >1
- Hovered: Highlight overlay
- Selected: Accent border
- Locked: Overlay + lock icon

## Tooltip

```
WBP_Tooltip
├── Border_Frame
│   └── VerticalBox_Content
│       ├── HorizontalBox_Header
│       │   ├── Image_Icon
│       │   └── VerticalBox_Title
│       │       ├── Text_Name
│       │       └── Text_Type
│       ├── Separator
│       ├── RichTextBlock_Description
│       └── VerticalBox_Stats (optional)
```

**Key behaviors:**
- Follow cursor with offset
- Clamp to screen bounds
- Fade in/out animation
- Dynamic height based on content

## Stat Bar

```
WBP_StatBar
├── Overlay
│   ├── Image_Background       # Dark base
│   ├── ProgressBar_Fill       # Colored fill
│   ├── Image_Decoration       # Texture overlay
│   └── HorizontalBox_Label
│       ├── Text_Current
│       ├── Text_Separator     # "/"
│       └── Text_Max
```

**Variants:**
- Health: Red gradient
- Mana: Blue gradient
- Stamina: Green/yellow
- Experience: Gold/purple

## Action Button

```
WBP_ActionButton
├── Button
│   └── Overlay
│       ├── Image_Background
│       ├── Image_Icon
│       ├── Image_Cooldown     # Radial wipe
│       ├── Text_Keybind       # Corner label
│       └── Border_Highlight   # Hover state
```

**States:**
- Available: Full color
- On Cooldown: Desaturated + radial fill
- Disabled: Grayed out
- Pressed: Scale down + darken

## Panel Container

```
WBP_Panel
├── Border_Outer
│   └── VerticalBox
│       ├── HorizontalBox_Header
│       │   ├── Text_Title
│       │   ├── Spacer
│       │   └── Button_Close
│       ├── Image_Separator
│       └── ScrollBox_Content
│           └── [Dynamic Content]
```

**Features:**
- Draggable header
- Close button
- Scrollable content area
- Consistent padding

## List Item

```
WBP_ListItem
├── Button_Row
│   └── HorizontalBox
│       ├── Image_Icon
│       ├── VerticalBox_Info
│       │   ├── Text_Primary
│       │   └── Text_Secondary
│       ├── Spacer
│       └── [Action Buttons]
```

**States:**
- Normal: Base colors
- Hovered: Subtle highlight
- Selected: Accent background
- Alternating: Slight shade difference

## Rarity Color Tiers

Standard tier colors (adjust to match design docs):

| Tier | Name | Typical Color |
|------|------|---------------|
| 0 | Common | Gray |
| 1 | Uncommon | Green |
| 2 | Rare | Blue |
| 3 | Epic | Purple |
| 4 | Legendary | Orange/Gold |
| 5 | Mythic | Red/Crimson |

## Animation Guidelines

**Hover transitions:**
- Duration: 0.1-0.15s
- Easing: EaseOut
- Properties: Color, scale, opacity

**Panel open/close:**
- Duration: 0.2-0.3s
- Easing: EaseOutQuad
- Properties: Scale + opacity

**Feedback pulses:**
- Duration: 0.3-0.5s
- Easing: EaseOutElastic (subtle)
- Use for: Item pickup, level up, notification
