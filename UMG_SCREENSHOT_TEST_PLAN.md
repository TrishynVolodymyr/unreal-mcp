# UMG Widget Screenshot Feature - Test Plan

## Overview
Test plan for verifying the widget screenshot capture functionality that allows AI to visually inspect UMG Widget Blueprint layouts.

## Prerequisites

### Setup Steps:
1. ✅ Build completed successfully
2. ✅ Python MCP tools updated with screenshot function
3. ✅ Launch Unreal Editor (`.\LaunchProject.bat`)
4. ✅ Python MCP server running (umg_mcp_server)
5. ✅ Test widget blueprint created

---

## Test Cases

### Test 1: Basic Screenshot Capture - Simple Widget

**Objective**: Verify basic screenshot functionality works with a simple widget

**Steps**:
1. Open Unreal Editor
2. Create a new Widget Blueprint: `WBP_ScreenshotTest1`
3. Add components:
   - Canvas Panel (root)
   - Button widget at position [100, 100], size [200, 50]
   - Text widget inside button with text "Test Button"
4. Save and compile the widget
5. Call MCP tool via Python:
   ```python
   result = capture_widget_screenshot(
       widget_name="WBP_ScreenshotTest1",
       width=800,
       height=600,
       format="png"
   )
   ```

**Expected Result**:
```json
{
  "success": true,
  "widget_name": "WBP_ScreenshotTest1",
  "image_base64": "iVBORw0KGgoAAAANSUhE...",
  "width": 800,
  "height": 600,
  "format": "png",
  "image_size_bytes": 50000-150000,
  "message": "Successfully captured screenshot..."
}
```

**Verification**:
- ✅ `success` is `true`
- ✅ `image_base64` contains data (long string starting with valid PNG header)
- ✅ Width and height match requested dimensions
- ✅ Image size is reasonable (50-150KB for 800x600)
- ✅ AI can view the image and see the button with text

---

### Test 2: High Resolution Screenshot

**Objective**: Test higher resolution captures

**Steps**:
1. Use the same widget from Test 1
2. Call MCP tool with higher resolution:
   ```python
   result = capture_widget_screenshot(
       widget_name="WBP_ScreenshotTest1",
       width=1920,
       height=1080,
       format="png"
   )
   ```

**Expected Result**:
- ✅ `success` is `true`
- ✅ `width` is 1920, `height` is 1080
- ✅ Image size is larger (~200-500KB)
- ✅ Screenshot shows higher detail

---

### Test 3: JPEG Format

**Objective**: Verify JPEG compression works

**Steps**:
1. Use the same widget
2. Call with JPEG format:
   ```python
   result = capture_widget_screenshot(
       widget_name="WBP_ScreenshotTest1",
       width=800,
       height=600,
       format="jpg"
   )
   ```

**Expected Result**:
- ✅ `success` is `true`
- ✅ `format` is "jpg" or "jpeg"
- ✅ Image size is smaller than PNG (~30-50% smaller)
- ✅ AI can view the image

---

### Test 4: Complex Widget Layout

**Objective**: Test with a more complex widget hierarchy

**Steps**:
1. Create widget: `WBP_ComplexTest`
2. Add components:
   - Canvas Panel (root)
   - Vertical Box at [50, 50], size [700, 500]
     - Text Block: "Header Text"
     - Horizontal Box
       - Button: "Button 1"
       - Button: "Button 2"
       - Button: "Button 3"
     - Image widget (any test image)
     - Progress Bar
3. Save and compile
4. Capture screenshot:
   ```python
   result = capture_widget_screenshot(
       widget_name="WBP_ComplexTest",
       width=1024,
       height=768
   )
   ```

**Expected Result**:
- ✅ Screenshot shows all components
- ✅ Hierarchy is visible (vertical/horizontal layout)
- ✅ All widgets rendered correctly

---

### Test 5: Widget With Styling

**Objective**: Verify colors, fonts, and styling are captured

**Steps**:
1. Create widget: `WBP_StyledTest`
2. Add components with specific styling:
   - Button with blue background
   - Text with red color, size 24 font
   - Border with yellow background
3. Capture screenshot
4. Have AI describe what it sees

**Expected Result**:
- ✅ AI can identify blue button
- ✅ AI can see red text
- ✅ AI can see yellow border
- ✅ Colors are accurate

---

### Test 6: Error Handling - Non-existent Widget

**Objective**: Verify error handling for invalid widget

**Steps**:
```python
result = capture_widget_screenshot(
    widget_name="WBP_DoesNotExist",
    width=800,
    height=600
)
```

**Expected Result**:
```json
{
  "success": false,
  "error": "Widget blueprint 'WBP_DoesNotExist' not found",
  "message": "Failed to capture widget screenshot: ..."
}
```

**Verification**:
- ✅ `success` is `false`
- ✅ Error message is clear and helpful
- ✅ No crash or exception

---

### Test 7: Error Handling - Invalid Dimensions

**Objective**: Verify parameter validation

**Steps**:
```python
# Test 1: Width too large
result = capture_widget_screenshot(
    widget_name="WBP_ScreenshotTest1",
    width=10000,
    height=600
)

# Test 2: Negative dimensions
result = capture_widget_screenshot(
    widget_name="WBP_ScreenshotTest1",
    width=-100,
    height=600
)

# Test 3: Zero dimensions
result = capture_widget_screenshot(
    widget_name="WBP_ScreenshotTest1",
    width=0,
    height=0
)
```

**Expected Result**:
- ✅ All return `success: false`
- ✅ Error messages explain the problem:
  - "Invalid width: 10000. Must be between 1 and 8192"
  - "Invalid width: -100. Must be between 1 and 8192"
  - "Invalid width: 0. Must be between 1 and 8192"

---

### Test 8: Error Handling - Invalid Format

**Objective**: Verify format validation

**Steps**:
```python
result = capture_widget_screenshot(
    widget_name="WBP_ScreenshotTest1",
    width=800,
    height=600,
    format="bmp"  # Invalid format
)
```

**Expected Result**:
```json
{
  "success": false,
  "error": "Invalid format: bmp. Must be one of: png, jpg, jpeg",
  "message": "Failed to capture screenshot: unsupported format 'bmp'"
}
```

---

### Test 9: Real-World Use Case - AI Layout Verification

**Objective**: Test complete AI workflow for layout creation and verification

**Scenario**: AI creates a login screen and verifies layout

**Steps**:
1. AI creates widget blueprint `WBP_LoginScreen`
2. AI adds components:
   - Title text at top
   - Username field
   - Password field
   - Login button
3. AI captures screenshot
4. AI analyzes screenshot and identifies issues (if any)
5. AI adjusts positioning based on visual feedback
6. AI captures again to verify

**Expected Result**:
- ✅ AI successfully creates all components
- ✅ Screenshot captures initial layout
- ✅ AI can identify misalignments or spacing issues
- ✅ AI adjusts components
- ✅ Final screenshot shows correct layout

---

### Test 10: Performance Test

**Objective**: Measure screenshot capture performance

**Steps**:
1. Capture screenshots of various resolutions
2. Measure time taken for each

**Resolutions to test**:
- 800x600 (small)
- 1280x720 (medium)
- 1920x1080 (large)
- 3840x2160 (4K)

**Expected Performance**:
- 800x600: < 100ms
- 1280x720: < 150ms
- 1920x1080: < 200ms
- 3840x2160: < 500ms

**File Sizes**:
- PNG: 50KB-500KB depending on complexity
- JPEG: 30-50% smaller than PNG

---

## Integration Test: Screenshot + Enhanced Layout

**Objective**: Verify screenshot works alongside existing layout tools

**Steps**:
1. Create widget `WBP_IntegrationTest`
2. Call `get_widget_component_layout` to get JSON hierarchy
3. Call `capture_widget_screenshot` to get visual
4. Compare JSON data with visual

**Verification**:
- ✅ Both tools work independently
- ✅ JSON shows correct hierarchy
- ✅ Screenshot shows visual representation
- ✅ AI can correlate JSON with visual
- ✅ Combined tools give complete picture

---

## Success Criteria

### Must Pass:
- ✅ All basic screenshot tests (Tests 1-5)
- ✅ All error handling tests (Tests 6-8)
- ✅ Integration test passes
- ✅ No crashes or exceptions
- ✅ AI can successfully view and analyze screenshots

### Should Pass:
- ✅ Performance within acceptable range (Test 10)
- ✅ Real-world use case succeeds (Test 9)

### Nice to Have:
- ✅ Screenshots work for all widget types
- ✅ Works with custom widgets
- ✅ Works with widget animations (static frame)

---

## Known Limitations

1. **Static Capture Only**: Screenshots capture a single frame, not animations
2. **Editor Context Required**: Widget must be in editor, not runtime
3. **Size Limits**: Maximum 8192x8192 pixels
4. **Format Support**: Only PNG and JPEG
5. **No Transparency Control**: PNG captures with whatever transparency is set in widget

---

## Regression Tests

After any code changes, verify:
- ✅ Basic capture still works (Test 1)
- ✅ Error handling still works (Test 6)
- ✅ Both PNG and JPEG work (Tests 1 and 3)

---

## Test Execution Checklist

### Pre-Test:
- [ ] Unreal Editor launched
- [ ] Python MCP server running
- [ ] Test widgets created
- [ ] AI assistant connected

### During Test:
- [ ] Run each test case
- [ ] Document results
- [ ] Save sample screenshots
- [ ] Note any issues

### Post-Test:
- [ ] All tests passed
- [ ] Issues logged (if any)
- [ ] Screenshots archived
- [ ] Performance metrics recorded

---

## Quick Start Testing

**Minimal test to verify it works**:
```python
# 1. Create a simple widget in Unreal Editor with a button
# 2. Run this command:

result = capture_widget_screenshot(
    widget_name="WBP_ScreenshotTest1",
    width=800,
    height=600,
    format="png"
)

# 3. Check result
print(f"Success: {result['success']}")
print(f"Image size: {result.get('image_size_bytes', 0)} bytes")

# 4. Ask AI: "Can you see the widget in the screenshot?"
# AI should be able to view the base64 image and describe what it sees
```

---

## Troubleshooting

### Issue: "Widget blueprint not found"
**Solution**: Make sure widget is saved and compiled in Unreal Editor

### Issue: "No editor world available"
**Solution**: Unreal Editor must be running

### Issue: "Failed to create widget preview instance"
**Solution**: Widget blueprint may have compilation errors - check Unreal Editor logs

### Issue: Image is blank/black
**Solution**: Widget may have no root widget or canvas panel

### Issue: Python import error
**Solution**: Restart Python MCP server after code changes

---

**Status**: Ready for testing
**Next**: Run Test 1 to verify basic functionality
