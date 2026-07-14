# Unreal MCP Editor Tools

This document provides detailed information about the editor tools available in the Unreal MCP integration.

## Overview

Editor tools allow you to control the Unreal Editor viewport and other editor functionality through MCP commands. These tools are particularly useful for automating tasks like focusing the camera on specific actors or locations.

## Editor Tools

### focus_viewport

Focus the viewport on a specific actor or location.

**Parameters:**
- `target` (string, optional) - Name of the actor to focus on (if provided, location is ignored)
- `location` (array, optional) - [X, Y, Z] coordinates to focus on (used if target is None)
- `distance` (float, optional) - Distance from the target/location (default: 1000.0)
- `orientation` (array, optional) - [Pitch, Yaw, Roll] for the viewport camera

**Returns:**
- Response from Unreal Engine containing the result of the focus operation

**Example:**
```json
{
  "command": "focus_viewport",
  "params": {
    "target": "PlayerStart",
    "distance": 500,
    "orientation": [0, 180, 0]
  }
}
```

### take_screenshot

Capture a screenshot of the viewport.

**Parameters:**
- `filename` (string, optional) - Name of the file to save the screenshot as (default: "screenshot.png")
- `show_ui` (boolean, optional) - Whether to include UI elements in the screenshot (default: false)
- `resolution` (array, optional) - [Width, Height] for the screenshot

**Returns:**
- Result of the screenshot operation

**Example:**
```json
{
  "command": "take_screenshot",
  "params": {
    "filename": "my_scene.png",
    "show_ui": false,
    "resolution": [1920, 1080]
  }
}
```

### import_static_mesh

Import one or more FBX/OBJ static meshes into a Content folder. Multi-mesh FBX
files are split by node; a single mesh is renamed to `asset_name`.

**Parameters:**
- `source_file_path` (string, required) - Absolute source file path.
- `asset_name` (string, required) - Name used for a single imported mesh.
- `folder_path` (string, optional) - Destination Content folder.
- `import_materials` (boolean, optional) - Import source materials and textures; default `false`.
- `auto_generate_collision` (boolean, optional) - Generate simple collision; default `true` for backward compatibility.
- `vertex_color_import_option` (string, optional) - `Ignore`, `Replace`, or `Override`; default `Replace`, matching the previous importer behavior. Use `Replace` to import vertex colors stored in the source mesh.
- `vertex_override_color` (array, conditionally required) - `[R,G,B,A]` values in `0..1`; required with `Override` so the importer never silently falls back to opaque white.

```python
import_static_mesh(
    source_file_path="E:/meshes/SM_Grass.fbx",
    asset_name="SM_Grass",
    folder_path="/Game/Flora",
    import_materials=False,
    auto_generate_collision=False,
    vertex_color_import_option="Replace",
)
```

Invalid vertex-color option text fails before import and lists the three valid values.

When an asset already exists at `folder_path/asset_name`, the command performs a
same-path reimport as one combined target. Before invoking UE's legacy FBX
reimport factory it normalizes non-FBX/Interchange metadata to configured
`UFbxStaticMeshImportData`, including source path, scene-unit conversion,
collision, and vertex-color options. The reimport factory reads persisted mesh
metadata rather than the command's transient `UFbxImportUI`.

UE 5.7's legacy static-mesh reimport handler forcibly disables source material
and texture import. Therefore `import_materials=true` is supported for a new
import but explicitly rejected for same-path reimport; returning success while
silently preserving the old material assignments would make the tool lie.

`folder_path` is canonicalized before the existing-asset lookup, so a trailing
slash cannot bypass the safe same-path reimport branch. Static meshes owned by an
FBX Scene import (`bImportAsScene`) are rejected before metadata mutation: UE's
standalone FBX reimport factory does not preserve that scene-ownership contract.

The command invokes `UReimportFbxStaticMeshFactory::Reimport` directly and uses
its `EReimportResult`. Do not route this path through `FReimportManager` in UE
5.7: with Interchange enabled, that manager can convert the freshly normalized
legacy metadata back to Interchange and no longer honors this command's FBX
options. A failed handler result returns an error and restores the previous
import metadata/source provenance. Existing assets are not rebroadcast as
newly-created Asset Registry entries.

UE 5.7's legacy FBX importer performs mesh rollback atomically: on an internal
reimport failure it replaces the mutated `UStaticMesh` UObject with a rooted
transient duplicate at the original object path. Command-side rollback must
therefore resolve the mesh again by object path before restoring import
metadata; the pre-reimport metadata duplicate must remain strongly referenced
across the handler call. Reusing the pre-call mesh pointer after failure can
write to an object that UE already renamed and marked as garbage.

## Error Handling

All command responses include a "status" field indicating whether the operation succeeded, and an optional "message" field with details in case of failure.

```json
{
  "status": "error",
  "message": "Failed to get active viewport"
}
```

## Usage Examples

### Python Example

```python
from unreal_mcp_server import get_unreal_connection

# Get connection to Unreal Engine
unreal = get_unreal_connection()

# Focus on a specific actor
focus_response = unreal.send_command("focus_viewport", {
    "target": "PlayerStart",
    "distance": 500,
    "orientation": [0, 180, 0]
})
print(focus_response)

# Take a screenshot
screenshot_response = unreal.send_command("take_screenshot", {"filename": "my_scene.png"})
print(screenshot_response)
```

## Troubleshooting

- **Command fails with "Failed to get active viewport"**: Make sure Unreal Editor is running and has an active viewport.
- **Actor not found**: Verify that the actor name is correct and the actor exists in the current level.
- **Invalid parameters**: Ensure that location and orientation arrays contain exactly 3 values (X, Y, Z for location; Pitch, Yaw, Roll for orientation).

## Future Enhancements

- Support for setting viewport display mode (wireframe, lit, etc.)
- Camera animation paths for cinematic viewport control
- Support for multiple viewports
