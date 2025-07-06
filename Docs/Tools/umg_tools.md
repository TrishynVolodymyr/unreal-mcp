# Unreal MCP UMG Tools

This document provides detailed information about the UMG (Unreal Motion Graphics) tools available in the Unreal MCP integration.

## Overview

UMG tools allow you to create and manipulate UMG Widget Blueprints in Unreal Engine, including creating widget blueprints, adding components, binding events, setting properties, managing layouts, and displaying widgets in the viewport.

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

### add_widget_to_viewport

Add a Widget Blueprint instance to the viewport.

**Parameters:**
- `widget_name` (string) - Name of the Widget Blueprint to add
- `z_order` (int, optional) - Z-order for the widget (higher numbers appear on top), defaults to 0

**Returns:**
- Dict containing success status and widget instance information

**Example:**
```json
{
  "command": "add_widget_to_viewport",
  "params": {
    "widget_name": "MainMenu",
    "z_order": 5
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

### create_parent_and_child_widget_components

Create a new parent widget component with a new child component.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `parent_component_name` (string) - Name for the new parent component
- `child_component_name` (string) - Name for the new child component
- `parent_component_type` (string, optional) - Type of parent component to create, defaults to "Border"
- `child_component_type` (string, optional) - Type of child component to create, defaults to "TextBlock"
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

### check_widget_component_exists

Check if a component exists in the specified widget blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `component_name` (string) - Name of the component to check

**Returns:**
- Dict containing existence status of the component

**Example:**
```json
{
  "command": "check_widget_component_exists",
  "params": {
    "widget_name": "MyWidget",
    "component_name": "HeaderText"
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

### get_widget_container_component_dimensions

Get the dimensions of a container widget in a UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint
- `container_name` (string, optional) - Name of the container widget, defaults to "CanvasPanel_0" for the root canvas panel

**Returns:**
- Dict containing the container dimensions and position

**Example:**
```json
{
  "command": "get_widget_container_component_dimensions",
  "params": {
    "widget_name": "InventoryScreen",
    "container_name": "ItemsContainer"
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

### get_widget_component_layout

Get hierarchical layout information for all components within a UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `message` (string) - Status message
  - `hierarchy` (object) - Root component with hierarchical structure including:
    - `name` (string) - Component name
    - `type` (string) - Component class name
    - `slot_properties` (object) - Layout properties based on the slot type
    - `children` (array) - List of child components with the same structure

**Example:**
```json
{
  "command": "get_widget_component_layout",
  "params": {
    "widget_name": "WBP_MainMenu"
  }
}
```

## Common Usage Patterns

### Widget Creation Workflow

1. **Create Widget Blueprint**: Use `create_umg_widget_blueprint` to create a new widget
2. **Add Components**: Use `add_widget_component_to_widget` or `create_parent_and_child_widget_components` to add UI elements
3. **Set Properties**: Use `set_widget_component_property` to configure component appearance and behavior
4. **Bind Events**: Use `bind_widget_component_event` to handle user interactions
5. **Display Widget**: Use `add_widget_to_viewport` to show the widget in game

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