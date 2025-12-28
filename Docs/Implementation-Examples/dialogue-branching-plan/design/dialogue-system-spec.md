# Dialogue System UI Kit - Design Specification

## Overview

This UI kit provides all components needed for a branching dialogue system in a dark fantasy RPG. All designs follow the established visual language from the inventory system.

---

## Components Included

### 1. WBP_InteractionIndicator (`02_interaction_indicator.png`)

**Purpose:** Floating prompt above NPCs when player is nearby

| Property | Value |
|----------|-------|
| Size | 180 × 50 px |
| Background | Semi-transparent dark (#120F0C, 90% opacity) |
| Border | Gold (#AA8740), 2px |
| Corner Radius | 6px |

**Variants:**
- **Default** — Standard appearance
- **Highlighted** — Brighter gold border (for important NPCs)
- **Minimal** — No decorative elements

**Key Badge:**
- Size: 24 × 24 px
- Background: Dark gold (#785A28)
- Border: Light gold (#D2AF5A)
- Text: White, Jura Medium 20px

---

### 2. WBP_AnswerButton (`03_answer_button_states.png`)

**Purpose:** Clickable player response options

| Property | Value |
|----------|-------|
| Size | 600 × 44 px (flexible width) |
| Corner Radius | 4px |
| Number Badge | 26 × 26 px |

**States:**

| State | Background | Border | Text Color | Number |
|-------|------------|--------|------------|--------|
| Normal | #1C1814 → #181411 | #2D3034 | #D7CDC0 | #AA8740 |
| Hover | #342620 → #26211C | #AA8740 | #F5F0E6 | #D2AF5A |
| Pressed | #16120F → #0C0A08 | #785A28 | #AAA091 | #AA8740 |
| Disabled | #201B17 → #16120F | #2D3034 | #786E5F | #786E5F |

**Hover Features:**
- Left accent bar (3px, gold)
- Right arrow indicator
- Brighter text

---

### 3. WBP_SpeakerName (`04_speaker_name_badge.png`)

**Purpose:** Display current speaker's name

| Property | Value |
|----------|-------|
| Font | Jura Medium, 22px |
| Underline | 2px, fades out |

**Variants:**

| Type | Text Color | Accent Color |
|------|------------|--------------|
| Default | White (#F5F0E6) | Gold (#AA8740) |
| Important | Gold (#D2AF5A) | Gold (#AA8740) |
| Hostile | Muted red (#C88278) | Dark red (#96504A) |

---

### 4. WBP_DialogueTextArea (`05_dialogue_text_area.png`)

**Purpose:** NPC dialogue text display with typewriter effect

| Property | Value |
|----------|-------|
| Size | 580 × 100 px (flexible) |
| Background | Subtle dark (#19151280, 50% opacity) |
| Corner Radius | 4px |
| Font | Jura Light, 18px |
| Text Color | #D7CDC0 |
| Line Height | 26px |

**Typewriter Effect:**
- Text appears character by character
- Blinking cursor (gold #D2AF5A) at insertion point
- Cursor visible during typing, hidden when complete

---

### 5. WBP_DialogueFrame (`06_dialogue_frame.png`)

**Purpose:** Main container for dialogue window

| Property | Value |
|----------|-------|
| Size | 960 × 280 px |
| Background | Semi-transparent (#14110E, 94% opacity) |
| Border | Iron gray (#464B52), 2px |
| Inner Border | Highlight (#413830), 1px |
| Corner Radius | 8px |
| Shadow | 8px blur, black 25% opacity |

**Decorative Elements:**
- Gold corner ornaments (L-shaped, 12px)
- Top decorative line (dark gold)
- Subtle gradient overlay (top lighter)

---

### 6. Continue Indicators (`07_continue_indicators.png`)

**Purpose:** Signal player can advance dialogue

| Style | Size | Description |
|-------|------|-------------|
| Arrow | 30 × 20 px | Downward triangle, gold |
| Dots | 40 × 20 px | Three dots, animated pulse |
| Text | 100 × 24 px | "Click to continue" label |

---

### 7. Dialogue Dividers (`08_dialogue_dividers.png`)

**Purpose:** Separate dialogue sections

| Style | Description |
|-------|-------------|
| Simple | Fading line, iron gray |
| Ornate | Gold lines with center diamond |

---

## Layout Specifications

### Full Dialogue Window (`01_dialogue_window_complete.png`)

```
┌─────────────────────────────────────────────────────────┐
│ [Corner]                                      [Corner]  │
│                                                         │
│   Speaker Name ___________                              │
│                                                         │
│   ┌─────────────────────────────────────────────────┐  │
│   │ Dialogue text area with NPC speech...           │  │
│   │ Multiple lines wrap automatically.              │  │
│   └─────────────────────────────────────────────────┘  │
│                                                         │
│   ─────────────────────────────────────────────────    │
│                                                         │
│   [1] Answer option one                           ▶    │
│   [2] Answer option two                                │
│   [3] Answer option three                              │
│                                                    ▼   │
│ [Corner]                                      [Corner]  │
└─────────────────────────────────────────────────────────┘
```

### Positioning (1920×1080)

| Element | Position |
|---------|----------|
| Dialogue Window | Bottom center, Y: 780 |
| Horizontal Margin | 160px from edges |
| Content Padding | 28px |
| Answer Button Gap | 6px vertical |

---

## Animation Recommendations

### Interaction Indicator
- **Appear:** Fade in 0.2s + slight scale up
- **Idle:** Subtle bob (2px, 2s loop)
- **Disappear:** Fade out 0.15s

### Dialogue Window
- **Open:** Slide up from bottom + fade in (0.25s)
- **Close:** Slide down + fade out (0.2s)

### Typewriter Effect
- **Speed:** ~40 characters/second (adjustable)
- **Cursor Blink:** 0.5s interval
- **Skip:** Instant complete on click

### Answer Buttons
- **Hover:** 0.1s transition
- **Press:** Slight scale down (0.98x)
- **Appear:** Stagger in, 0.05s delay each

---

## Typography

| Element | Font | Size | Weight |
|---------|------|------|--------|
| Speaker Name | Jura | 22px | Medium |
| Dialogue Text | Jura | 18px | Light |
| Answer Text | Jura | 16px | Light |
| Number Badge | Jura | 14px | Medium |
| Interaction Prompt | Jura | 15px | Medium |
| Key Badge | Jura | 20px | Medium |

---

## Color Quick Reference

### Backgrounds
- Darkest: `#0C0A08`
- Dark: `#16120F`
- Panel: `#1C1814`

### Borders
- Iron Dark: `#2D3034`
- Iron Mid: `#464B52`
- Gold: `#AA8740`

### Text
- White: `#F5F0E6`
- Light: `#D7CDC0`
- Mid: `#AAA091`
- Dark: `#786E5F`

### Accents
- Gold Dark: `#785A28`
- Gold Mid: `#AA8740`
- Gold Light: `#D2AF5A`
- Gold Bright: `#F0D28C`
