# UMG Layout Visualization Improvements for AI

## Current State Analysis

### What We Have Now

**Existing `get_widget_component_layout` Tool:**
- Returns hierarchical JSON structure of widget tree
- Includes: widget names, types, slot properties
- Canvas panel slots show: position [x, y], size [width, height], z_order
- Generic panel slots: basic metadata only

**Example Current Output:**
```json
{
  "success": true,
  "widget_name": "MyWidget",
  "hierarchy": {
    "name": "CanvasPanel_0",
    "type": "CanvasPanel",
    "slot_properties": {},
    "children": [
      {
        "name": "Button_0",
        "type": "Button",
        "slot_properties": {
          "slot_type": "CanvasPanelSlot",
          "position": [100, 200],
          "size": [150, 50],
          "z_order": 0
        },
        "children": []
      }
    ]
  }
}
```

### Problems for AI

1. **No Visual Context**: AI cannot see the actual layout, colors, styling, or visual appearance
2. **Limited Spatial Understanding**: Numbers alone don't convey proportions, alignment, spacing
3. **No Style Information**: Missing colors, fonts, borders, padding, margins, alignment
4. **Complex Hierarchies Hard to Parse**: Deeply nested JSON is difficult to visualize mentally
5. **No Screenshot Capability**: AI cannot see what the user sees in Unreal Editor

---

## Proposed Improvements

### ✅ Solution 1: Enhanced Layout Data (EASY - Immediate Implementation)

**Add more detailed metadata to existing JSON:**

```cpp
// In BuildWidgetHierarchy() method:
TSharedPtr<FJsonObject> WidgetInfo = MakeShareable(new FJsonObject);

// Existing
WidgetInfo->SetStringField(TEXT("name"), Widget->GetName());
WidgetInfo->SetStringField(TEXT("type"), Widget->GetClass()->GetName());

// NEW: Add visual properties
if (UWidget* WidgetPtr = Widget)
{
    // Visibility
    WidgetInfo->SetBoolField(TEXT("is_visible"), WidgetPtr->GetVisibility() != ESlateVisibility::Hidden);

    // Size/Layout
    if (WidgetPtr->GetSlot())
    {
        // Add padding, alignment, anchors for all slot types
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WidgetPtr->Slot))
        {
            // Anchors
            FAnchors Anchors = CanvasSlot->GetAnchors();
            TSharedPtr<FJsonObject> AnchorsObj = MakeShareable(new FJsonObject);
            AnchorsObj->SetNumberField(TEXT("min_x"), Anchors.Minimum.X);
            AnchorsObj->SetNumberField(TEXT("min_y"), Anchors.Minimum.Y);
            AnchorsObj->SetNumberField(TEXT("max_x"), Anchors.Maximum.X);
            AnchorsObj->SetNumberField(TEXT("max_y"), Anchors.Maximum.Y);
            SlotProperties->SetObjectField(TEXT("anchors"), AnchorsObj);

            // Alignment
            FVector2D Alignment = CanvasSlot->GetAlignment();
            TArray<TSharedPtr<FJsonValue>> AlignmentArray;
            AlignmentArray.Add(MakeShareable(new FJsonValueNumber(Alignment.X)));
            AlignmentArray.Add(MakeShareable(new FJsonValueNumber(Alignment.Y)));
            SlotProperties->SetArrayField(TEXT("alignment"), AlignmentArray);
        }

        // Padding for box-type slots (HorizontalBox, VerticalBox)
        if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(WidgetPtr->Slot))
        {
            FMargin Padding = HBoxSlot->GetPadding();
            TSharedPtr<FJsonObject> PaddingObj = MakeShareable(new FJsonObject);
            PaddingObj->SetNumberField(TEXT("left"), Padding.Left);
            PaddingObj->SetNumberField(TEXT("top"), Padding.Top);
            PaddingObj->SetNumberField(TEXT("right"), Padding.Right);
            PaddingObj->SetNumberField(TEXT("bottom"), Padding.Bottom);
            SlotProperties->SetObjectField(TEXT("padding"), PaddingObj);

            SlotProperties->SetStringField(TEXT("horizontal_alignment"),
                UEnum::GetValueAsString(HBoxSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"),
                UEnum::GetValueAsString(HBoxSlot->GetVerticalAlignment()));
        }
    }

    // Text-specific properties
    if (UTextBlock* TextBlock = Cast<UTextBlock>(WidgetPtr))
    {
        TSharedPtr<FJsonObject> TextProps = MakeShareable(new FJsonObject);
        TextProps->SetStringField(TEXT("text"), TextBlock->GetText().ToString());
        TextProps->SetNumberField(TEXT("font_size"), TextBlock->GetFont().Size);
        FLinearColor Color = TextBlock->GetColorAndOpacity().GetSpecifiedColor();
        TextProps->SetStringField(TEXT("color"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(Color.R * 255), (int)(Color.G * 255), (int)(Color.B * 255), Color.A));
        WidgetInfo->SetObjectField(TEXT("text_properties"), TextProps);
    }

    // Image-specific properties
    if (UImage* Image = Cast<UImage>(WidgetPtr))
    {
        TSharedPtr<FJsonObject> ImageProps = MakeShareable(new FJsonObject);
        if (Image->GetBrush().GetResourceObject())
        {
            ImageProps->SetStringField(TEXT("texture"),
                Image->GetBrush().GetResourceObject()->GetPathName());
        }
        FLinearColor Tint = Image->GetColorAndOpacity().GetSpecifiedColor();
        ImageProps->SetStringField(TEXT("tint"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(Tint.R * 255), (int)(Tint.G * 255), (int)(Tint.B * 255), Tint.A));
        WidgetInfo->SetObjectField(TEXT("image_properties"), ImageProps);
    }
}
```

**Benefits:**
- ✅ Easy to implement (just extend existing method)
- ✅ No new dependencies
- ✅ Works immediately via existing MCP tool
- ✅ Gives AI much more context about layout

**Example Enhanced Output:**
```json
{
  "name": "Button_0",
  "type": "Button",
  "is_visible": true,
  "slot_properties": {
    "slot_type": "CanvasPanelSlot",
    "position": [100, 200],
    "size": [150, 50],
    "z_order": 0,
    "anchors": {"min_x": 0.5, "min_y": 0.5, "max_x": 0.5, "max_y": 0.5},
    "alignment": [0.5, 0.5]
  },
  "children": [
    {
      "name": "TextBlock_0",
      "type": "TextBlock",
      "text_properties": {
        "text": "Click Me!",
        "font_size": 24,
        "color": "rgba(255,255,255,1.00)"
      }
    }
  ]
}
```

---

### ✅ Solution 2: ASCII Layout Visualization (MEDIUM - Good Balance)

**Add a new tool that generates ASCII art representation of the layout:**

```python
@mcp.tool()
def get_widget_visual_layout(ctx: Context, widget_name: str, width: int = 80, height: int = 40) -> dict:
    """
    Get an ASCII visual representation of the widget layout.

    This generates a text-based "screenshot" that AI can see, showing:
    - Widget boundaries as boxes
    - Widget names/types as labels
    - Spatial relationships and nesting

    Args:
        widget_name: Name of the widget blueprint
        width: Width of ASCII canvas (default: 80 chars)
        height: Height of ASCII canvas (default: 40 chars)

    Returns:
        Dict with ASCII visualization and metadata

    Example output:
        ┌────────────────────────────────────────────┐
        │CanvasPanel_0 (1920x1080)                  │
        │                                            │
        │    ┌──────────────────┐                   │
        │    │Button_0 (150x50) │                   │
        │    │  [Click Me!]     │                   │
        │    └──────────────────┘                   │
        │                                            │
        │                          ┌──────────────┐ │
        │                          │Image_0       │ │
        │                          │(100x100)     │ │
        │                          └──────────────┘ │
        └────────────────────────────────────────────┘
    """
    # Implementation would:
    # 1. Call get_widget_component_layout to get hierarchy
    # 2. Generate ASCII canvas
    # 3. Draw boxes for each widget based on position/size
    # 4. Add labels with widget names
    # 5. Return ASCII string + layout data
```

**C++ Implementation:**
```cpp
// Add new command: GenerateWidgetASCIILayoutCommand
FString FUMGService::GenerateWidgetASCIILayout(const FString& WidgetName, int32 Width, int32 Height)
{
    // 1. Get widget hierarchy (reuse existing code)
    TSharedPtr<FJsonObject> LayoutInfo;
    GetWidgetComponentLayout(WidgetName, LayoutInfo);

    // 2. Create ASCII canvas
    TArray<FString> Canvas;
    Canvas.SetNum(Height);
    for (int32 i = 0; i < Height; i++)
    {
        Canvas[i] = FString::ChrN(Width, TEXT(' '));
    }

    // 3. Recursively draw widgets
    DrawWidgetASCII(Canvas, LayoutInfo->GetObjectField(TEXT("hierarchy")), Width, Height);

    // 4. Return joined canvas
    return FString::Join(Canvas, TEXT("\n"));
}
```

**Benefits:**
- ✅ AI can "see" the layout structure
- ✅ Shows spatial relationships clearly
- ✅ No image processing needed
- ✅ Works in pure text context
- ⚠️ Medium implementation effort
- ⚠️ Limited detail (no colors, styles)

---

### ✅ Solution 3: Screenshot Generation (BEST - Full Visual Context)

**Add capability to render widget to image and return as base64:**

```python
@mcp.tool()
def capture_widget_screenshot(
    ctx: Context,
    widget_name: str,
    width: int = 800,
    height: int = 600,
    format: str = "png"
) -> dict:
    """
    Capture a screenshot of the widget blueprint preview.

    Returns the widget rendered as an image that AI can see.
    This gives the most accurate representation of the layout.

    Args:
        widget_name: Name of the widget blueprint
        width: Screenshot width in pixels (default: 800)
        height: Screenshot height in pixels (default: 600)
        format: Image format (png, jpg)

    Returns:
        Dict containing:
        - success: bool
        - image_base64: Base64-encoded image data
        - width: Actual image width
        - height: Actual image height
        - widget_bounds: Bounding box of widget content

    Example:
        result = capture_widget_screenshot("MyWidget", 1024, 768)
        # AI receives actual visual of the widget layout
    """
```

**C++ Implementation using FWidgetRenderer:**

```cpp
// New command: CaptureWidgetScreenshotCommand

#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Base64.h"
#include "ImageUtils.h"

TSharedPtr<FJsonObject> FUMGService::CaptureWidgetScreenshot(
    const FString& WidgetName,
    int32 Width,
    int32 Height)
{
    UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(WidgetName);
    if (!WidgetBP || !WidgetBP->GeneratedClass)
    {
        return CreateErrorResponse(TEXT("Widget blueprint not found"));
    }

    // 1. Create a preview instance of the widget
    UUserWidget* PreviewWidget = CreateWidget<UUserWidget>(
        GEditor->GetEditorWorldContext().World(),
        WidgetBP->GeneratedClass);

    if (!PreviewWidget)
    {
        return CreateErrorResponse(TEXT("Failed to create widget preview"));
    }

    // 2. Get the Slate widget
    TSharedPtr<SWidget> SlateWidget = PreviewWidget->TakeWidget();
    if (!SlateWidget.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to get Slate widget"));
    }

    // 3. Create render target
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, true);
    RenderTarget->UpdateResourceImmediate(true);

    // 4. Render widget to texture
    FWidgetRenderer WidgetRenderer(true, false);
    WidgetRenderer.DrawWidget(
        RenderTarget,
        SlateWidget.ToSharedRef(),
        FVector2D(Width, Height),
        0.0f);

    // 5. Read pixels from render target
    FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
    TArray<FColor> OutPixels;
    RTResource->ReadPixels(OutPixels);

    // 6. Compress to PNG
    TArray<uint8> CompressedPNG;
    FImageUtils::CompressImageArray(Width, Height, OutPixels, CompressedPNG);

    // 7. Encode as base64
    FString Base64Image = FBase64::Encode(CompressedPNG);

    // 8. Build response
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("image_base64"), Base64Image);
    Response->SetNumberField(TEXT("width"), Width);
    Response->SetNumberField(TEXT("height"), Height);
    Response->SetStringField(TEXT("format"), TEXT("png"));

    // Clean up
    PreviewWidget->RemoveFromParent();
    PreviewWidget->MarkAsGarbage();

    return Response;
}
```

**Python Side:**
```python
# In utils/widgets/widget_components.py

def capture_widget_screenshot_impl(
    ctx: Context,
    widget_name: str,
    width: int = 800,
    height: int = 600,
    format: str = "png"
) -> dict:
    """Capture widget screenshot via TCP command."""
    response = send_tcp_command("capture_widget_screenshot", {
        "widget_name": widget_name,
        "width": width,
        "height": height,
        "format": format
    })
    return response
```

**Benefits:**
- ✅✅✅ **BEST SOLUTION** - AI can SEE exactly what user sees
- ✅ Shows colors, fonts, styling, images, everything
- ✅ Perfect for layout debugging and precise positioning
- ✅ Claude can analyze the image directly (multimodal)
- ✅ No ambiguity - visual truth
- ⚠️ Higher implementation effort
- ⚠️ Larger data transfer (base64 images ~100KB-500KB)
- ⚠️ Requires Editor mode (widget must be instantiable)

---

## Comparison Matrix

| Feature | Current JSON | Enhanced JSON | ASCII Layout | Screenshot |
|---------|-------------|---------------|--------------|------------|
| Implementation Effort | ✅ Done | ⭐ Easy | ⭐⭐ Medium | ⭐⭐⭐ Complex |
| Data Size | Small | Small | Small | Large |
| Visual Accuracy | ❌ None | ⭐ Low | ⭐⭐ Medium | ⭐⭐⭐⭐⭐ Perfect |
| Shows Colors/Styles | ❌ No | ⭐ Partial | ❌ No | ✅ Yes |
| Shows Spatial Layout | ⭐ Numbers only | ⭐⭐ Better numbers | ⭐⭐⭐ Visual | ⭐⭐⭐⭐⭐ Perfect |
| AI Understanding | ⭐ Poor | ⭐⭐ Better | ⭐⭐⭐ Good | ⭐⭐⭐⭐⭐ Excellent |
| Performance | Fast | Fast | Fast | Moderate |
| Editor Dependency | None | None | None | Requires Editor |

---

## Recommended Implementation Plan

### Phase 1: Quick Wins (Immediate)
✅ **Enhance existing `get_widget_component_layout`** with additional metadata:
- Visibility, anchors, alignment
- Padding, margins for all slot types
- Text content, colors, font sizes
- Image sources, tints
- Enable/disable states

**Estimated Time:** 2-4 hours
**Files to modify:**
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/UMG/UMGService.cpp` (BuildWidgetHierarchy method)

### Phase 2: Visual Layout (Week 2)
✅ **Add ASCII layout visualization tool**
- New command: `generate_widget_ascii_layout`
- Shows boxes, labels, spatial relationships
- Works entirely in text domain

**Estimated Time:** 1-2 days
**Files to create:**
- `GenerateWidgetASCIILayoutCommand.h/.cpp`
- Update `UMGService` with ASCII rendering method
- Python wrapper in `umg_tools.py`

### Phase 3: Screenshot Capability (Week 3-4)
✅ **Implement screenshot capture**
- New command: `capture_widget_screenshot`
- Uses `FWidgetRenderer` to render to texture
- Returns base64-encoded PNG
- AI can view actual rendered widget

**Estimated Time:** 3-5 days
**Files to create:**
- `CaptureWidgetScreenshotCommand.h/.cpp`
- Update `UMGService` with screenshot method
- Python wrapper in `umg_tools.py`
- Add image handling utilities

---

## Technical Considerations

### For Screenshot Solution

**Dependencies to add to `UnrealMCP.Build.cs`:**
```csharp
PrivateDependencyModuleNames.AddRange(new string[]
{
    "Slate",
    "SlateCore",
    "ImageWrapper",  // For PNG compression
    "RenderCore",    // For render targets
    "RHI"            // For reading pixels
});
```

**Memory Management:**
- Create temporary render targets (will be garbage collected)
- Immediate pixel readback (synchronous, but Editor context is OK)
- Base64 encoding adds ~33% size overhead
- Consider caching if same widget requested multiple times

**Error Handling:**
- Widget must be valid and instantiable
- Handle widgets with dynamic content carefully
- Timeout for rendering (in case of infinite loops in widget construction)

---

## Example Use Cases

### Use Case 1: AI Creates Login Screen
```
User: "Create a login screen with username, password fields and a login button"

AI calls:
1. create_umg_widget_blueprint("LoginScreen")
2. add_widget_component_to_widget("LoginScreen", "UsernameField", "EditableTextBox", ...)
3. add_widget_component_to_widget("LoginScreen", "PasswordField", "EditableTextBox", ...)
4. add_widget_component_to_widget("LoginScreen", "LoginButton", "Button", ...)
5. capture_widget_screenshot("LoginScreen", 1024, 768)

AI receives screenshot, sees:
- Fields are too close together
- Button is misaligned
- Colors don't match

AI adjusts:
6. set_widget_component_placement("LoginScreen", "UsernameField", position=[512, 300])
7. set_widget_component_placement("LoginScreen", "PasswordField", position=[512, 400])
8. set_widget_component_property("LoginScreen", "LoginButton", "HorizontalAlignment", "Center")
9. capture_widget_screenshot("LoginScreen", 1024, 768)

AI verifies layout looks correct visually ✅
```

### Use Case 2: AI Debugs Layout Issues
```
User: "Why is my button not visible?"

AI calls:
1. get_widget_component_layout("MyWidget")  # Gets enhanced JSON
2. capture_widget_screenshot("MyWidget")     # Gets visual

AI analyzes:
- JSON shows button position is [-1000, -1000] (off-screen)
- Screenshot confirms button not visible
- AI identifies: "Button is positioned outside canvas bounds"

AI fixes:
3. set_widget_component_placement("MyWidget", "Button", position=[100, 100])
4. capture_widget_screenshot("MyWidget")

AI verifies: "Button is now visible in the screenshot" ✅
```

---

## Conclusion

**YES, screenshot capability via MCP is ABSOLUTELY POSSIBLE** and would be **tremendously valuable** for AI-assisted UMG development.

**Recommended Priority:**
1. ✅ **Phase 1** (Do Now): Enhance JSON metadata - Easy, immediate value
2. ✅ **Phase 3** (Do Next): Implement screenshots - Maximum AI benefit
3. ⏸️ **Phase 2** (Optional): ASCII layout - Nice-to-have if screenshots prove difficult

The screenshot solution leverages Unreal's existing `FWidgetRenderer` API and is the gold standard for giving AI visual context. It requires moderate implementation effort but provides exponential improvement in AI's ability to create and debug UMG layouts.

**Would you like me to start implementing Phase 1 (Enhanced JSON metadata) right now?**
