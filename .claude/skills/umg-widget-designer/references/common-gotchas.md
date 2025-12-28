# UMG Common Gotchas

Pitfalls and solutions for UMG widget development.

---

## Text Issues

### Long text gets cut off
**Problem:** Dialogue or descriptions overflow container.
**Solution:** Enable `Auto Wrap Text` on TextBlock.
```
TextBlock > Auto Wrap Text: ✓
```
Also ensure parent has defined width or uses `Size To Content` properly.

### Text doesn't wrap even with Auto Wrap
**Problem:** Auto Wrap enabled but text still clips.
**Cause:** Parent widget has no width constraint.
**Solution:** Wrap in `SizeBox` with explicit width, or use `HorizontalBox` slot with `Fill` sizing.

### RichTextBlock ignores line breaks
**Problem:** `\n` doesn't create new lines.
**Solution:** Use `<br/>` tag in rich text, or ensure default style has proper line height.

### Text blurry at certain scales
**Problem:** DPI scaling causes fuzzy text.
**Solution:** Use fonts at appropriate sizes, avoid extreme scaling. Consider separate text sizes for different resolutions.

---

## Layout Issues

### Widget doesn't appear
**Problem:** Added widget but invisible.
**Causes:**
- Size is 0x0 (no content, no explicit size)
- Render Opacity is 0
- Visibility set to Hidden/Collapsed
- Behind another widget (Z-order)
- Anchored outside viewport

**Debug:** Set explicit size via `SizeBox`, check Visibility, check Render Opacity.

### Child fills entire parent unexpectedly
**Problem:** Small element stretches to fill container.
**Cause:** Slot set to `Fill` instead of `Auto`.
**Solution:** In parent Box, set child's slot Size to `Auto`.

### Content doesn't center
**Problem:** Widget stays top-left despite center anchor.
**Cause:** Alignment not set, or parent overriding position.
**Solution:** Set both Anchor AND Alignment to center (0.5, 0.5).

### ScrollBox content not scrolling
**Problem:** ScrollBox shows but doesn't scroll.
**Causes:**
- Content smaller than ScrollBox (nothing to scroll)
- Content not in a vertical/horizontal box
- Nested ScrollBoxes conflicting

**Solution:** Ensure content exceeds ScrollBox bounds. Wrap content in VerticalBox if needed.

### CanvasPanel children overlap wrong
**Problem:** Z-order not as expected.
**Solution:** Later children in hierarchy render on top. Reorder in widget tree, or use `ZOrder` slot property.

---

## Anchor & Positioning Issues

### Widget moves on different resolutions
**Problem:** UI shifts position on different screens.
**Cause:** Anchored to wrong point, or using absolute position.
**Solution:** 
- Center elements: Anchor (0.5, 0.5), Alignment (0.5, 0.5)
- Corners: Anchor to that corner
- Stretch: Use anchor range (min ≠ max)

### Stretched widget distorts
**Problem:** Image or panel looks squished/stretched.
**Solution:** Use `SizeBox` with fixed aspect ratio, or `ScaleBox` with appropriate scaling rule.

### Anchor range math confusion
**Remember:**
- Anchor Min/Max same point = fixed position
- Anchor Min/Max different = stretch between
- Position/Offset is relative to anchor point
- Alignment shifts the pivot (0.5 = centered on anchor)

---

## Button & Interaction Issues

### Button doesn't respond to clicks
**Problem:** Button exists but OnClicked never fires.
**Causes:**
- Another widget blocking input (check Hit Test Invisible)
- IsFocusable disabled when needed
- Button Visibility not `Visible` (must be exactly Visible, not just shown)

**Solution:** Set blocking widgets to `Hit Test Invisible`. Ensure button `Visibility = Visible`.

### Hover state stuck
**Problem:** Button stays hovered after mouse leaves.
**Cause:** Mouse capture issues, or quick mouse movement.
**Solution:** Reset style in OnUnhovered, or use IsHovered checks in Tick.

### Click passes through to widgets behind
**Problem:** Clicking UI also triggers game input.
**Solution:** Ensure widget has proper `Visibility` (not Hit Test Invisible). Set Input Mode to UI Only when menu open.

---

## Image Issues

### Image appears white/blank
**Problem:** Image widget shows but no texture.
**Causes:**
- Brush not set
- Texture not imported correctly
- Wrong texture format

**Solution:** Verify Brush > Image is set. Check texture import settings.

### 9-slice corners stretching
**Problem:** Box-mode image corners distort.
**Cause:** Margin values wrong.
**Solution:** Margins are 0-1 normalized. Corner of 32px on 128px texture = 32/128 = 0.25 margin.

### Image color tinting everything
**Problem:** Image has unwanted color overlay.
**Solution:** Set `Color and Opacity` to white (1,1,1,1) for no tint.

---

## Performance Gotchas

### UI causing frame drops
**Problem:** Complex UI tanks performance.
**Causes:**
- Too many widgets
- Invalidation every frame
- Complex materials on UI

**Solutions:**
- Cache complex widgets
- Avoid Tick-based updates, use event-driven
- Collapse/remove offscreen widgets
- Use `Invalidation Box` for static content

### Binding functions called constantly
**Problem:** Property bindings fire every frame.
**Solution:** Avoid bindings for values that rarely change. Use manual updates via events instead.

---

## Animation Issues

### Animation doesn't play
**Problem:** PlayAnimation called but nothing happens.
**Causes:**
- Animation not added to widget
- Playing on wrong widget instance
- Animation length is 0

**Solution:** Verify animation exists in Widget > Animations. Check you're calling on correct widget reference.

### Animation resets unexpectedly
**Problem:** Animation snaps back to start.
**Cause:** Playing from beginning when already playing, or PlayMode wrong.
**Solution:** Use `Play Animation` with proper settings, check if already playing before replay.

---

## Hierarchy Issues

### Can't access child widget in Blueprint
**Problem:** GetWidgetFromName returns None.
**Cause:** Widget created at runtime not in tree, or name mismatch.
**Solution:** Use `Is Variable` checkbox on widget to expose it. Match exact name.

### Widget Switcher shows wrong index
**Problem:** SetActiveWidgetIndex shows unexpected child.
**Solution:** Index is 0-based. First child = 0. Verify child order in hierarchy.

---

## Quick Debug Checklist

When widget misbehaves:

1. ☐ Visibility = Visible (not Hidden, not Collapsed, not Hit Test Invisible if needs interaction)
2. ☐ Size > 0 (wrap in SizeBox if needed)
3. ☐ Render Opacity = 1
4. ☐ Anchors/Alignment correct for intended position
5. ☐ Parent slot settings (Auto vs Fill)
6. ☐ Nothing blocking input (check Hit Test settings on overlapping widgets)
7. ☐ Is Variable checked if accessed in Blueprint
