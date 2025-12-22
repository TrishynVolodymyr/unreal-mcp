# UMG Service Split Test Plan

This document contains test payloads for verifying the functionality displaced from `UMGService.cpp` into separate service files.

## Service Files Created

| Service | File | Functions Moved |
|---------|------|-----------------|
| WidgetLayoutService | `WidgetLayoutService.cpp` (402 lines) | `GetWidgetComponentLayout`, `CaptureWidgetScreenshot`, `BuildWidgetHierarchy` |
| WidgetBindingService | `WidgetBindingService.cpp` (296 lines) | `CreateEventBinding`, `CreateTextBlockBindingFunction` |
| WidgetInputHandlerService | `WidgetInputHandlerService.cpp` (456 lines) | `CreateWidgetInputHandler`, `RemoveWidgetFunctionGraph` |

---

## 1. WidgetLayoutService Tests

### 1.1 Get Widget Component Layout

**TCP Command:** `get_widget_component_layout`

**Test Payload:**
```json
{
    "command": "get_widget_component_layout",
    "params": {
        "widget_name": "WBP_TestWidget"
    }
}
```

**Expected Response:**
```json
{
    "success": true,
    "message": "Successfully retrieved widget component layout",
    "hierarchy": {
        "name": "CanvasPanel",
        "type": "CanvasPanel",
        "is_visible": true,
        "is_enabled": true,
        "slot_properties": {},
        "children": [
            {
                "name": "TextBlock_0",
                "type": "TextBlock",
                "slot_properties": {
                    "slot_type": "CanvasPanelSlot",
                    "position": [100, 100],
                    "size": [200, 50]
                }
            }
        ]
    }
}
```

**Pre-requisite:** Create test widget first:
```json
{
    "command": "create_widget_blueprint",
    "params": {
        "name": "WBP_TestWidget",
        "parent_class": "UserWidget",
        "path": "/Game/Widgets"
    }
}
```

---

### 1.2 Capture Widget Screenshot

**TCP Command:** `capture_widget_screenshot`

**Test Payload:**
```json
{
    "command": "capture_widget_screenshot",
    "params": {
        "widget_name": "WBP_TestWidget",
        "width": 800,
        "height": 600,
        "format": "png"
    }
}
```

**Expected Response:**
```json
{
    "success": true,
    "image_base64": "<base64_encoded_png_data>",
    "width": 800,
    "height": 600,
    "format": "png",
    "image_size_bytes": 12345
}
```

**Alternative - JPEG format:**
```json
{
    "command": "capture_widget_screenshot",
    "params": {
        "widget_name": "WBP_TestWidget",
        "width": 1920,
        "height": 1080,
        "format": "jpg"
    }
}
```

---

## 2. WidgetBindingService Tests

### 2.1 Bind Widget Event (e.g., Button OnClicked)

**TCP Command:** `bind_widget_event`

**Pre-requisite - Create widget with button:**
```json
{
    "command": "add_widget_component",
    "params": {
        "blueprint_name": "WBP_TestWidget",
        "component_name": "TestButton",
        "component_type": "Button",
        "position": [100, 100],
        "size": [200, 50]
    }
}
```

**Test Payload:**
```json
{
    "command": "bind_widget_event",
    "params": {
        "blueprint_name": "WBP_TestWidget",
        "component_name": "TestButton",
        "event_name": "OnClicked",
        "function_name": "HandleTestButtonClicked"
    }
}
```

**Expected Response:**
```json
{
    "success": true,
    "actual_function_name": "HandleTestButtonClicked",
    "message": "Event 'OnClicked' bound to widget 'TestButton'"
}
```

**Alternative Events to Test:**
- `OnPressed` (Button)
- `OnReleased` (Button)
- `OnHovered` (Button)
- `OnUnhovered` (Button)

---

### 2.2 Set Text Block Binding

**TCP Command:** `set_text_block_binding`

**Pre-requisite - Create widget with text block:**
```json
{
    "command": "add_widget_component",
    "params": {
        "blueprint_name": "WBP_TestWidget",
        "component_name": "ScoreText",
        "component_type": "TextBlock",
        "position": [100, 200],
        "size": [200, 50]
    }
}
```

**Test Payload - Text type:**
```json
{
    "command": "set_text_block_binding",
    "params": {
        "blueprint_name": "WBP_TestWidget",
        "text_block_name": "ScoreText",
        "binding_name": "PlayerScore",
        "variable_type": "Text"
    }
}
```

**Test Payload - Integer type:**
```json
{
    "command": "set_text_block_binding",
    "params": {
        "blueprint_name": "WBP_TestWidget",
        "text_block_name": "ScoreText",
        "binding_name": "ScoreValue",
        "variable_type": "Int"
    }
}
```

**Test Payload - String type:**
```json
{
    "command": "set_text_block_binding",
    "params": {
        "blueprint_name": "WBP_TestWidget",
        "text_block_name": "PlayerNameText",
        "binding_name": "PlayerName",
        "variable_type": "String"
    }
}
```

**Expected Response:**
```json
{
    "success": true,
    "message": "Text block binding created successfully",
    "function_name": "GetPlayerScore",
    "variable_name": "PlayerScore"
}
```

---

## 3. WidgetInputHandlerService Tests

### 3.1 Create Widget Input Handler (Right Mouse Button)

**TCP Command:** `create_widget_input_handler`

**Test Payload - Right Mouse Button Press:**
```json
{
    "command": "create_widget_input_handler",
    "params": {
        "widget_name": "WBP_TestWidget",
        "component_name": "",
        "input_type": "MouseButton",
        "input_event": "RightMouseButton",
        "trigger": "Pressed",
        "handler_name": "OnRightClick"
    }
}
```

**Expected Response:**
```json
{
    "success": true,
    "actual_handler_name": "OnRightClick",
    "message": "Input handler created successfully"
}
```

**Test Payload - Left Mouse Button Release:**
```json
{
    "command": "create_widget_input_handler",
    "params": {
        "widget_name": "WBP_TestWidget",
        "component_name": "",
        "input_type": "MouseButton",
        "input_event": "LeftMouseButton",
        "trigger": "Released",
        "handler_name": "OnLeftRelease"
    }
}
```

**Test Payload - Keyboard Key:**
```json
{
    "command": "create_widget_input_handler",
    "params": {
        "widget_name": "WBP_TestWidget",
        "component_name": "",
        "input_type": "Key",
        "input_event": "Escape",
        "trigger": "Pressed",
        "handler_name": "OnEscapePressed"
    }
}
```

---

### 3.2 Remove Widget Function Graph

**TCP Command:** `remove_widget_function_graph`

**Test Payload - Remove broken function:**
```json
{
    "command": "remove_widget_function_graph",
    "params": {
        "widget_name": "WBP_InventorySlot",
        "function_name": "OnMouseButtonDown"
    }
}
```

**Expected Response:**
```json
{
    "success": true,
    "message": "Function graph 'OnMouseButtonDown' removed successfully"
}
```

**Test Payload - Remove custom handler:**
```json
{
    "command": "remove_widget_function_graph",
    "params": {
        "widget_name": "WBP_TestWidget",
        "function_name": "OnRightClick"
    }
}
```

---

## 4. Integration Test Sequence

Complete test sequence to verify all displaced functionality:

### Step 1: Create Test Widget
```json
{
    "command": "create_widget_blueprint",
    "params": {
        "name": "WBP_SplitTestWidget",
        "parent_class": "UserWidget",
        "path": "/Game/Widgets/Tests"
    }
}
```

### Step 2: Add Components
```json
{
    "command": "add_widget_component",
    "params": {
        "blueprint_name": "WBP_SplitTestWidget",
        "component_name": "TestButton",
        "component_type": "Button",
        "position": [100, 100],
        "size": [200, 50]
    }
}
```

```json
{
    "command": "add_widget_component",
    "params": {
        "blueprint_name": "WBP_SplitTestWidget",
        "component_name": "InfoText",
        "component_type": "TextBlock",
        "position": [100, 200],
        "size": [300, 40]
    }
}
```

### Step 3: Test WidgetLayoutService
```json
{
    "command": "get_widget_component_layout",
    "params": {
        "widget_name": "WBP_SplitTestWidget"
    }
}
```

### Step 4: Test WidgetBindingService - Event Binding
```json
{
    "command": "bind_widget_event",
    "params": {
        "blueprint_name": "WBP_SplitTestWidget",
        "component_name": "TestButton",
        "event_name": "OnClicked",
        "function_name": "HandleButtonClick"
    }
}
```

### Step 5: Test WidgetBindingService - Text Binding
```json
{
    "command": "set_text_block_binding",
    "params": {
        "blueprint_name": "WBP_SplitTestWidget",
        "text_block_name": "InfoText",
        "binding_name": "DisplayInfo",
        "variable_type": "Text"
    }
}
```

### Step 6: Test WidgetInputHandlerService - Create Handler
```json
{
    "command": "create_widget_input_handler",
    "params": {
        "widget_name": "WBP_SplitTestWidget",
        "component_name": "",
        "input_type": "MouseButton",
        "input_event": "RightMouseButton",
        "trigger": "Pressed",
        "handler_name": "OnContextMenu"
    }
}
```

### Step 7: Test Screenshot Capture
```json
{
    "command": "capture_widget_screenshot",
    "params": {
        "widget_name": "WBP_SplitTestWidget",
        "width": 800,
        "height": 600,
        "format": "png"
    }
}
```

### Step 8: Verify Layout After All Changes
```json
{
    "command": "get_widget_component_layout",
    "params": {
        "widget_name": "WBP_SplitTestWidget"
    }
}
```

---

## 5. Error Case Tests

### 5.1 Non-existent Widget
```json
{
    "command": "get_widget_component_layout",
    "params": {
        "widget_name": "WBP_DoesNotExist"
    }
}
```
**Expected:** Error response with "not found" message

### 5.2 Invalid Event Name
```json
{
    "command": "bind_widget_event",
    "params": {
        "blueprint_name": "WBP_SplitTestWidget",
        "component_name": "TestButton",
        "event_name": "OnInvalidEvent",
        "function_name": "HandleInvalid"
    }
}
```
**Expected:** Error response with "delegate not found" message

### 5.3 Remove Non-existent Function
```json
{
    "command": "remove_widget_function_graph",
    "params": {
        "widget_name": "WBP_SplitTestWidget",
        "function_name": "NonExistentFunction"
    }
}
```
**Expected:** Error or warning response

---

## 6. Python MCP Tool Commands

For testing via Python MCP tools:

```python
# WidgetLayoutService
get_widget_component_layout(widget_name="WBP_TestWidget")
capture_widget_screenshot(widget_name="WBP_TestWidget", width=800, height=600, format="png")

# WidgetBindingService
bind_widget_event(blueprint_name="WBP_TestWidget", component_name="TestButton", event_name="OnClicked", function_name="HandleClick")
set_text_block_binding(blueprint_name="WBP_TestWidget", text_block_name="ScoreText", binding_name="Score", variable_type="Int")

# WidgetInputHandlerService
create_widget_input_handler(widget_name="WBP_TestWidget", component_name="", input_type="MouseButton", input_event="RightMouseButton", trigger="Pressed", handler_name="OnRightClick")
remove_widget_function_graph(widget_name="WBP_TestWidget", function_name="OnMouseButtonDown")
```

---

## Notes

1. **Pre-requisites**: Ensure Unreal Editor is running with the TCP server active on port 55557
2. **Widget Paths**: Default widget path is `/Game/Widgets` - adjust if your project uses different paths
3. **Compilation**: Most operations trigger automatic blueprint compilation and save
4. **Existing Bindings**: Event binding checks for duplicates and returns success if already bound
