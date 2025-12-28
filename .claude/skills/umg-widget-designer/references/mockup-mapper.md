# Mockup to UMG Component Mapper

How design mockup elements translate to UMG primitives.

## Visual Elements

| Mockup Element | UMG Widget | Notes |
|----------------|------------|-------|
| Solid rectangle | `Border` | Use for backgrounds, frames |
| Rounded rectangle | `Border` | Set Corner Radius in Brush |
| Image/icon | `Image` | Set Brush from texture |
| Gradient fill | `Image` | Use gradient texture or material |
| Drop shadow | `Image` (separate) | Layer behind, or use material |
| Glow/outer glow | `Image` (separate) | Additive blend, layer behind |
| Decorative frame | `Image` | 9-slice via Draw As: Box |

## Text Elements

| Mockup Element | UMG Widget | Notes |
|----------------|------------|-------|
| Simple label | `TextBlock` | Static text, single style |
| Styled text (mixed) | `RichTextBlock` | Bold, colors, inline icons |
| Long text / dialogue | `TextBlock` | **Enable Auto Wrap Text** |
| Input field | `EditableText` / `EditableTextBox` | Box version has border |
| Multi-line input | `MultiLineEditableText` | For notes, descriptions |

## Layout Elements

| Mockup Element | UMG Widget | Notes |
|----------------|------------|-------|
| Horizontal row | `HorizontalBox` | Children align in row |
| Vertical stack | `VerticalBox` | Children stack vertically |
| Grid layout | `UniformGridPanel` | Equal-size cells |
| Flexible grid | `GridPanel` | Variable row/column sizes |
| Overlapping layers | `Overlay` | Children stack on Z-axis |
| Absolute positioning | `CanvasPanel` | Position by coordinates |
| Scrollable area | `ScrollBox` | Wrap content that overflows |
| Fixed aspect ratio | `SizeBox` + `ScaleBox` | Maintain proportions |

## Interactive Elements

| Mockup Element | UMG Widget | Notes |
|----------------|------------|-------|
| Clickable button | `Button` | Wrap content in Button |
| Checkbox | `CheckBox` | Built-in checked states |
| Slider | `Slider` | Value 0-1, style thumb/track |
| Progress bar | `ProgressBar` | Percent fill |
| Dropdown | `ComboBox` | Requires string options |
| Tab bar | `WidgetSwitcher` + buttons | Manual tab logic |
| Radio buttons | Multiple `CheckBox` | Manual exclusivity logic |

## Container Patterns

| Mockup Pattern | UMG Approach |
|----------------|--------------|
| Card with padding | `Border` > `VerticalBox` with Padding |
| Header + content | `VerticalBox` > Header widget + content |
| Sidebar layout | `HorizontalBox` > sidebar + `Spacer` + main |
| Centered content | `Overlay` or anchor to center |
| Footer pinned bottom | `VerticalBox` > content + `Spacer` + footer |

## 9-Slice (Box) Images

For frames, panels, buttons with rounded corners:

1. Set Image `Draw As`: **Box**
2. Set `Margin` values (0-1) for slice boundaries
3. Corners stay fixed, edges stretch

**Margin guide:**
- If corner is 16px on 64px texture: margin = 16/64 = 0.25
- Set all four margins (Left, Top, Right, Bottom)

## Spacing Translation

| Design Term | UMG Property |
|-------------|--------------|
| Margin (outside) | `Padding` on Slot |
| Padding (inside) | `Padding` on Border/Box |
| Gap between items | `Padding` on child slots |
| Fixed width/height | `SizeBox` wrapper |
| Min/max size | `SizeBox` with overrides |

## Color Application

| Where | UMG Property |
|-------|--------------|
| Text color | `TextBlock` > Color and Opacity |
| Background | `Border` > Brush Color |
| Image tint | `Image` > Color and Opacity |
| Button states | `Button` > Style > Normal/Hovered/Pressed |
| Opacity | Alpha channel or Render Opacity |
