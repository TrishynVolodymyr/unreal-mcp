# Unreal MCP UMG Tools

This document provides detailed information about the UMG (Unreal Motion Graphics) tools available in the Unreal MCP integration.

## Overview

UMG tools allow you to create and manipulate UMG Widget Blueprints in Unreal Engine, including creating widget blueprints, adding components, binding events, setting properties, managing layouts, capturing widget screenshots, and configuring widget settings.

**Available Tools:**
- `create_umg_widget_blueprint` - Create a new widget blueprint
- `add_widget_component_to_widget` - Add any component to a widget
- `add_child_widget_component_to_parent` - Nest components in parent containers
- `create_parent_and_child_widget_components` - Create parent-child in one operation
- `set_widget_component_property` - Set component properties
- `set_widget_component_placement` - Position and size components
- `bind_widget_component_event` - Bind events to functions
- `set_text_block_widget_component_binding` - Set up text bindings
- `create_widget_input_handler` - Handle mouse/key/touch input
- `remove_widget_function_graph` - Remove function graphs
- `reorder_widget_children` - Reorder children in containers
- `set_widget_design_size_mode` - Configure design size mode
- `set_widget_parent_class` - Change widget parent class
- `get_widget_blueprint_metadata` - Get comprehensive widget info
- `capture_widget_screenshot` - Capture widget preview

## UMG Tools

### create_umg_widget_blueprint

Create a new UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the widget blueprint to create
- `parent_class` (string, optional) - Parent class for the widget, defaults to "UserWidget"
- `path` (string, optional) - Content browser path where the widget should be created, defaults to "/Game/Widgets"

**Returns:**
- Dict containing success status and widget path

**Example:**
```json
{
  "command": "create_umg_widget_blueprint",
  "params": {
    "widget_name": "MainMenu",
    "parent_class": "UserWidget",
    "path": "/Game/UI/Menus"
  }
}
```

### bind_widget_component_event

Bind an event on a widget component to a function.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `widget_component_name` (string) - Name of the widget component (button, etc.)
- `event_name` (string) - Name of the event to bind (OnClicked, etc.)
- `function_name` (string, optional) - Name of the function to create/bind to (defaults to "{widget_component_name}_{event_name}")

**Returns:**
- Dict containing success status and binding information

**Example:**
```json
{
  "command": "bind_widget_component_event",
  "params": {
    "widget_name": "LoginScreen",
    "widget_component_name": "LoginButton",
    "event_name": "OnClicked",
    "function_name": "HandleLoginClick"
  }
}
```

### set_text_block_widget_component_binding

Set up a property binding for a Text Block widget.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `text_block_name` (string) - Name of the Text Block to bind
- `binding_property` (string) - Name of the property to bind to
- `binding_type` (string, optional) - Type of binding (Text, Visibility, etc.), defaults to "Text"

**Returns:**
- Dict containing success status and binding information

**Example:**
```json
{
  "command": "set_text_block_widget_component_binding",
  "params": {
    "widget_name": "PlayerHUD",
    "text_block_name": "ScoreText",
    "binding_property": "CurrentScore",
    "binding_type": "Text"
  }
}
```

### add_child_widget_component_to_parent

Add a widget component as a child to another component.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `parent_component_name` (string) - Name of the parent component
- `child_component_name` (string) - Name of the child component to add to the parent
- `create_parent_if_missing` (boolean, optional) - Whether to create the parent component if it doesn't exist, defaults to false
- `parent_component_type` (string, optional) - Type of parent component to create if needed, defaults to "Border"
- `parent_position` (array, optional) - [X, Y] position of the parent component if created, defaults to [0.0, 0.0]
- `parent_size` (array, optional) - [Width, Height] of the parent component if created, defaults to [300.0, 200.0]

**Returns:**
- Dict containing success status and component relationship information

**Example:**
```json
{
  "command": "add_child_widget_component_to_parent",
  "params": {
    "widget_name": "GameMenu",
    "parent_component_name": "ButtonsContainer",
    "child_component_name": "StartButton",
    "create_parent_if_missing": true,
    "parent_component_type": "VerticalBox",
    "parent_position": [100.0, 100.0],
    "parent_size": [300.0, 400.0]
  }
}
```

### set_widget_component_placement

Change the placement (position/size) of a widget component.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `component_name` (string) - Name of the component to modify
- `position` (array, optional) - [X, Y] new position in the canvas panel
- `size` (array, optional) - [Width, Height] new size for the component
- `alignment` (array, optional) - [X, Y] alignment values (0.0 to 1.0)

**Returns:**
- Dict containing success status and updated placement information

**Example:**
```json
{
  "command": "set_widget_component_placement",
  "params": {
    "widget_name": "HUD",
    "component_name": "HealthBar",
    "position": [50.0, 25.0],
    "size": [250.0, 30.0],
    "alignment": [0.0, 0.0]
  }
}
```

### add_widget_component_to_widget

Unified function to add any type of widget component to a UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `component_name` (string) - Name to give the new component
- `component_type` (string) - Type of component to add (e.g., "TextBlock", "Button", etc.)
- `position` (array, optional) - [X, Y] position in the canvas panel
- `size` (array, optional) - [Width, Height] of the component
- `kwargs` (object, optional) - Additional parameters specific to the component type

**Returns:**
- Dict containing success status and component properties

**Example:**
```json
{
  "command": "add_widget_component_to_widget",
  "params": {
    "widget_name": "MyWidget",
    "component_name": "SubmitButton",
    "component_type": "Button",
    "position": [100, 100],
    "size": [200, 50],
    "kwargs": {
      "text": "Submit",
      "background_color": [0.2, 0.4, 0.8, 1.0]
    }
  }
}
```

### set_widget_component_property

Set one or more properties on a specific component within a UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `component_name` (string) - Name of the component to modify
- `kwargs` (object) - Properties to set as keyword arguments

**Returns:**
- Dict containing success status and property setting results

**Example:**
```json
{
  "command": "set_widget_component_property",
  "params": {
    "widget_name": "MyWidget",
    "component_name": "MyTextBlock",
    "kwargs": {
      "Text": "Red Text",
      "ColorAndOpacity": {
        "SpecifiedColor": {
          "R": 1.0,
          "G": 0.0,
          "B": 0.0,
          "A": 1.0
        }
      }
    }
  }
}
```

### get_widget_blueprint_metadata

Get comprehensive metadata about a Widget Blueprint. This consolidated tool replaces the deprecated `check_widget_component_exists`, `get_widget_component_layout`, and `get_widget_container_component_dimensions` tools.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint (e.g., "WBP_MainMenu", "/Game/UI/MyWidget")
- `fields` (array, optional) - List of fields to include. Options: "components", "layout", "dimensions", "hierarchy", "bindings", "events", "variables", "functions", "*" (all). Defaults to all fields.
- `container_name` (string, optional) - Container name for dimensions field, defaults to "CanvasPanel_0"

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `widget_name` (string) - Name of the widget blueprint
  - `asset_path` (string) - Full asset path
  - `parent_class` (string) - Parent class name
  - `components` (object) - Component list with count (if requested)
  - `layout` (object) - Hierarchical layout info (if requested)
  - `dimensions` (object) - Container dimensions (if requested)
  - `hierarchy` (object) - Simple widget tree (if requested)
  - `bindings` (object) - Property bindings (if requested)
  - `events` (object) - Event bindings (if requested)
  - `variables` (object) - Blueprint variables (if requested)
  - `functions` (object) - Blueprint functions (if requested)

**Examples:**

Get all metadata:
```json
{
  "command": "get_widget_blueprint_metadata",
  "params": {
    "widget_name": "WBP_MainMenu"
  }
}
```

Check if a component exists (replaces check_widget_component_exists):
```json
{
  "command": "get_widget_blueprint_metadata",
  "params": {
    "widget_name": "WBP_MainMenu",
    "fields": ["components"]
  }
}
```

Get layout info (replaces get_widget_component_layout):
```json
{
  "command": "get_widget_blueprint_metadata",
  "params": {
    "widget_name": "WBP_MainMenu",
    "fields": ["layout"]
  }
}
```

Get container dimensions (replaces get_widget_container_component_dimensions):
```json
{
  "command": "get_widget_blueprint_metadata",
  "params": {
    "widget_name": "WBP_MainMenu",
    "fields": ["dimensions"],
    "container_name": "CanvasPanel_0"
  }
}
```

### create_parent_and_child_widget_components

Create a new parent widget component with a new child component (one parent, one child) in a single operation.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `parent_component_name` (string) - Name for the new parent component
- `child_component_name` (string) - Name for the new child component
- `parent_component_type` (string, optional) - Type of parent component to create (e.g., "Border", "VerticalBox"), defaults to "Border"
- `child_component_type` (string, optional) - Type of child component to create (e.g., "TextBlock", "Button"), defaults to "TextBlock"
- `parent_position` (array, optional) - [X, Y] position of the parent component, defaults to [0.0, 0.0]
- `parent_size` (array, optional) - [Width, Height] of the parent component, defaults to [300.0, 200.0]
- `child_attributes` (object, optional) - Additional attributes for the child component (content, colors, etc.)

**Returns:**
- Dict containing success status and component creation information

**Example:**
```json
{
  "command": "create_parent_and_child_widget_components",
  "params": {
    "widget_name": "MyWidget",
    "parent_component_name": "HeaderBorder",
    "child_component_name": "TitleText",
    "parent_component_type": "Border",
    "child_component_type": "TextBlock",
    "parent_position": [50.0, 50.0],
    "parent_size": [400.0, 100.0],
    "child_attributes": {
      "text": "Welcome to My Game",
      "font_size": 24
    }
  }
}
```

### create_widget_input_handler

Create an input event handler in a Widget Blueprint for events not exposed as standard delegates. Use this for right-click context menus, keyboard shortcuts, touch gestures, drag and drop operations, etc.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `input_type` (string) - Type of input: "MouseButton", "Key", "Touch", "Focus", "Drag"
- `input_event` (string) - Specific event:
  - MouseButton: "LeftMouseButton", "RightMouseButton", "MiddleMouseButton", "ThumbMouseButton", "ThumbMouseButton2"
  - Key: Any key name (e.g., "Enter", "Escape", "SpaceBar", "A", "F1")
  - Touch: "Touch", "Pinch", "Swipe"
  - Focus: "FocusReceived", "FocusLost"
  - Drag: "DragDetected", "DragEnter", "DragLeave", "DragOver", "Drop"
- `trigger` (string, optional) - When to trigger: "Pressed" (default), "Released", "DoubleClick"
- `handler_name` (string, optional) - Name for the custom event function (auto-generated if empty)
- `component_name` (string, optional) - Optional component for component-specific handling

**Returns:**
- Dict containing success status and handler information

**Example:**
```json
{
  "command": "create_widget_input_handler",
  "params": {
    "widget_name": "WBP_InventorySlot",
    "input_type": "MouseButton",
    "input_event": "RightMouseButton",
    "handler_name": "OnSlotRightClicked"
  }
}
```

### remove_widget_function_graph

Remove a function graph from a Widget Blueprint. Use this to clean up broken/corrupt function graphs that prevent compilation, remove unwanted override functions, or reset widget event handlers.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `function_name` (string) - Name of the function graph to remove (e.g., "OnMouseButtonDown")

**Returns:**
- Dict containing success status and removal information

**Example:**
```json
{
  "command": "remove_widget_function_graph",
  "params": {
    "widget_name": "WBP_InventorySlot",
    "function_name": "OnMouseButtonDown"
  }
}
```

### reorder_widget_children

Reorder children within a container widget (HorizontalBox, VerticalBox, etc.). Use this when components are in wrong visual order within a container.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `container_name` (string) - Name of the container component (e.g., "ContentBox", "ButtonsContainer")
- `child_order` (array) - List of child component names in desired order

**Returns:**
- Dict containing success status and updated child order

**Example:**
```json
{
  "command": "reorder_widget_children",
  "params": {
    "widget_name": "WBP_MainMenu",
    "container_name": "ButtonsContainer",
    "child_order": ["PlayButton", "SettingsButton", "QuitButton"]
  }
}
```

### set_widget_design_size_mode

Set the design size mode for a Widget Blueprint. Controls how the widget preview appears in the designer and how the widget determines its size at runtime.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `design_size_mode` (string) - Design size mode. Options:
  - "DesiredOnScreen" - Widget uses its desired size based on content
  - "Custom" - Use custom width/height values
  - "FillScreen" - Fill entire screen (respects safe zones)
  - "CustomOnScreen" - Custom size that responds to DPI scaling
- `custom_width` (integer, optional) - Width when using Custom or CustomOnScreen modes, defaults to 1920
- `custom_height` (integer, optional) - Height when using Custom or CustomOnScreen modes, defaults to 1080

**Returns:**
- Dict containing success status and applied settings

**Example:**
```json
{
  "command": "set_widget_design_size_mode",
  "params": {
    "widget_name": "WBP_MainHUD",
    "design_size_mode": "FillScreen"
  }
}
```

### set_widget_parent_class

Change the parent class of a Widget Blueprint. Useful for reparenting a widget to a different base class for inheritance of shared functionality.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `new_parent_class` (string) - New parent class to reparent to. Can be:
  - Simple class name: "UserWidget", "BaseUserWidget"
  - Full path: "/Script/UMG.UserWidget"
  - Blueprint path: "/Game/UI/Base/WBP_BaseWidget"

**Returns:**
- Dict containing success status, new parent class, and old parent class

**Example:**
```json
{
  "command": "set_widget_parent_class",
  "params": {
    "widget_name": "WBP_GameMenu",
    "new_parent_class": "/Game/UI/Base/WBP_BaseMenuWidget"
  }
}
```

### capture_widget_screenshot

Capture a screenshot of a UMG Widget Blueprint preview. Returns base64-encoded image data that AI can view directly.

**Parameters:**
- `widget_name` (string) - Name of the widget blueprint to capture
- `width` (integer, optional) - Screenshot width in pixels (default: 800, range: 1-8192)
- `height` (integer, optional) - Screenshot height in pixels (default: 600, range: 1-8192)
- `format` (string, optional) - Image format - "png" (default) or "jpg"

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the screenshot was captured
  - `image_base64` (string) - Base64-encoded image data (viewable by AI)
  - `width` (integer) - Actual image width
  - `height` (integer) - Actual image height
  - `format` (string) - Image format used
  - `image_size_bytes` (integer) - Size of the compressed image
  - `message` (string) - Success or error message

**Example:**
```json
{
  "command": "capture_widget_screenshot",
  "params": {
    "widget_name": "WBP_MainMenu",
    "width": 1920,
    "height": 1080,
    "format": "png"
  }
}
```

## Deprecated Tools

The following tools have been removed and replaced by `get_widget_blueprint_metadata`:

- **check_widget_component_exists** - Use `get_widget_blueprint_metadata` with `fields=["components"]` and check for component in results
- **get_widget_container_component_dimensions** - Use `get_widget_blueprint_metadata` with `fields=["dimensions"]`
- **get_widget_component_layout** - Use `get_widget_blueprint_metadata` with `fields=["layout"]`

## Common Usage Patterns

### Widget Creation Workflow

1. **Create Widget Blueprint**: Use `create_umg_widget_blueprint` to create a new widget
2. **Add Components**: Use `add_widget_component_to_widget` to add UI elements, then `add_child_widget_component_to_parent` for nesting
3. **Set Properties**: Use `set_widget_component_property` to configure component appearance and behavior
4. **Bind Events**: Use `bind_widget_component_event` to handle user interactions
5. **Verify Layout**: Use `get_widget_blueprint_metadata` with `fields=["layout"]` to inspect the widget structure
6. **Capture Preview**: Use `capture_widget_screenshot` to visually verify the widget appearance

### Component Types Reference

Common component types for UMG widgets:
- `TextBlock` - Display text
- `Button` - Interactive button
- `Image` - Display images/textures
- `Border` - Container with border/background
- `VerticalBox` - Vertical layout container
- `HorizontalBox` - Horizontal layout container
- `ScrollBox` - Scrollable container
- `CanvasPanel` - Free-form positioning container
- `Slider` - Value slider control
- `CheckBox` - Boolean checkbox control
- `EditableText` - Text input field
- `ProgressBar` - Progress indicator

### Event Types Reference

Common event types for widget components:
- `OnClicked` - Button clicks
- `OnPressed` - Button press down
- `OnReleased` - Button release
- `OnHovered` - Mouse hover enter
- `OnUnhovered` - Mouse hover exit
- `OnValueChanged` - Value change (sliders, checkboxes)
- `OnTextChanged` - Text input changes
- `OnTextCommitted` - Text input committed

### Property Types Reference

Common properties that can be set:
- `Text` - Text content for TextBlock, Button
- `ColorAndOpacity` - Color and transparency
- `BrushColor` - Background/border color
- `Visibility` - Visibility state (Visible, Hidden, Collapsed)
- `IsEnabled` - Whether the component is enabled
- `Padding` - Internal spacing
- `Margin` - External spacing
- `HorizontalAlignment` - Horizontal alignment in container
- `VerticalAlignment` - Vertical alignment in container

### Alignment Values

Alignment values for positioning (0.0 to 1.0):
- `[0.0, 0.0]` - Top-left
- `[0.5, 0.0]` - Top-center
- `[1.0, 0.0]` - Top-right
- `[0.0, 0.5]` - Middle-left
- `[0.5, 0.5]` - Center
- `[1.0, 0.5]` - Middle-right
- `[0.0, 1.0]` - Bottom-left
- `[0.5, 1.0]` - Bottom-center
- `[1.0, 1.0]` - Bottom-right
