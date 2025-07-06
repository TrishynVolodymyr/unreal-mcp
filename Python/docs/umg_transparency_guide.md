# UMG Transparency Guide for MCP

This guide explains how to work with transparency in UMG widgets when using the MCP (Model-Controlled Programming) system.

## Understanding Transparency in UMG

There are two primary ways to control transparency in UMG widgets:

1. **Brush Color Alpha** - Setting the alpha component of a brush color (4th value in color array: [R, G, B, A])
2. **Render Opacity** - Setting the overall opacity of a widget (affects the entire widget and its children)

For complex brush cases like rounded corners, you need to enable "Use Brush Transparency" for the alpha component to work properly.

## Transparency for Borders

Borders are the most common components that need transparency settings, especially for creating modern UI designs.

### Method 1: Creating a Border with Transparency

```python
# Creating a transparent border with rounded corners
add_widget_component_to_widget(
    widget_name="MyWidget",
    component_name="TransparentBorder",
    component_type="Border",
    position=[100, 100],
    size=[300, 200],
    background_color=[0.1, 0.2, 0.3, 0.5],  # 50% alpha in the color
    use_brush_transparency=True,            # Enable brush transparency (REQUIRED for proper alpha)
    padding=[10, 10, 10, 10]
)
```

### Method 2: Setting Transparency on an Existing Border

```python
# Set the border brush color with transparency
set_widget_component_property(
    widget_name="MyWidget",
    component_name="ExistingBorder",
    value={"BrushColor": "(R=0.1,G=0.2,B=0.3,A=0.5)"}  # 50% alpha
)

# Enable brush transparency (REQUIRED for alpha to work properly)
set_widget_component_property(
    widget_name="MyWidget",
    component_name="ExistingBorder",
    value={"UseBrushTransparency": "true"}
)
```

## Transparency for Images

Images can also have transparency settings, particularly useful for icons and decorative elements.

```python
# Create an image with transparency
add_widget_component_to_widget(
    widget_name="MyWidget",
    component_name="TransparentImage",
    component_type="Image",
    position=[150, 150],
    size=[100, 100],
    brush_color=[1.0, 1.0, 1.0, 0.7]  # 70% opacity white
)

# Enable transparency for complex images
set_widget_component_property(
    widget_name="MyWidget",
    component_name="TransparentImage",
    kwargs={"UseBrushTransparency": "true"}
)
```

## Transparency for Buttons

Buttons can be made transparent by modifying their style properties.

```python
# Create a semi-transparent button
add_widget_component_to_widget(
    widget_name="MyWidget",
    component_name="TransparentButton",
    component_type="Button",
    position=[200, 300],
    size=[150, 40],
    background_color=[0.2, 0.4, 0.8, 0.6]  # 60% opacity blue
)

# Enable brush transparency for proper alpha
set_widget_component_property(
    widget_name="MyWidget",
    component_name="TransparentButton",
    kwargs={"UseBrushTransparency": "true"}
)
```

## Combining Methods: Overall Widget Opacity

Sometimes you want to fade an entire widget including all its children. For this, use the `RenderOpacity` property:

```python
# Create a border with fully opaque background
add_widget_component_to_widget(
    widget_name="MyWidget",
    component_name="FadedContainer",
    component_type="Border",
    position=[50, 50],
    size=[400, 300],
    background_color=[0.1, 0.2, 0.3, 1.0],  # Solid background color
)

# Add a TextBlock as a child (would be added to the Border in the widget editor)
add_widget_component_to_widget(
    widget_name="MyWidget",
    component_name="ContentText",
    component_type="TextBlock",
    text="This entire container will be semi-transparent"
)

# Now make the ENTIRE container 50% transparent (affects all children too)
set_widget_component_property(
    widget_name="MyWidget",
    component_name="FadedContainer",
    value={"RenderOpacity": "0.5"}
)
```

## Troubleshooting Transparency Issues

If you're having issues with transparency:

1. **For rounded corners or complex brushes**: Ensure `UseBrushTransparency` is set to `true`
2. **For widgets that show black instead of transparency**: Check if the correct blend mode is set; use `UseBrushTransparency` to fix this
3. **For nested transparency**: Remember that child components inherit opacity from parents, so you may need to adjust accordingly
4. **For opacity artifacts**: In some cases, try wrapping your widget in a Retainer Box with a transparent material

## Best Practices

1. **Use brush alpha (A=value)** for controlling transparency of individual elements
2. **Use RenderOpacity** for fading entire widgets including all their children
3. **Always enable UseBrushTransparency** when working with semi-transparent borders, especially with rounded corners
4. **Test in Play mode** as some transparency effects may appear differently in the editor than at runtime 