# Material Tools - Unreal MCP

This document provides comprehensive information about using Material tools through the Unreal MCP (Model Context Protocol). These tools allow you to create, modify, and manage Material and Material Instance assets in Unreal Engine using natural language commands through AI assistants.

## Overview

Material tools enable you to:
- Create base Materials with configurable blend modes, shading models, and usage flags
- Create and duplicate Material Instances with parameter overrides
- Set scalar, vector, texture, and batch parameters on Material Instances
- Inspect material parameters and expression graphs
- Add, connect, and delete material expression nodes in the graph editor
- Connect expressions to material outputs (BaseColor, Emissive, Opacity, etc.)
- Modify material properties (blend mode, shading model, two-sided, Niagara usage)
- Search the Material Palette for available expressions and functions
- Compile materials to apply changes

## Natural Language Usage Examples

### Creating Materials and Instances

```
"Create an additive unlit material called 'M_EmberParticle' for Niagara sprites"

"Make a material instance of M_Crystal called 'MI_Crystal_Red' with red base color and high metallic"

"Duplicate MI_Crystal_Red to create MI_Crystal_DarkRed"

"Create a translucent material called 'M_Glass' with default lit shading"
```

### Setting Parameters

```
"Set the roughness on MI_Crystal_Red to 0.3"

"Change the base color of MI_Crystal_Red to bright blue"

"Set the normal map texture on MI_Crystal_Red to T_Crystal_N"

"Batch update MI_Crystal_Red: metallic 0.9, roughness 0.1, emissive intensity 2.0, and base color to dark red"
```

### Inspecting and Modifying Material Graphs

```
"Show me all the parameters available on M_Crystal"

"Get the expression graph metadata for M_FireballProjectile"

"Add a Multiply node to M_Ember and connect it to EmissiveColor"

"Search the material palette for radial gradient expressions"

"Change this material to masked blend mode and enable two-sided rendering"
```

## Tool Reference

---

### `create_material`

Create a new base Material asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the material (e.g., `"M_EmberParticle"`) |
| `path` | string | | Folder path (default: `"/Game/Materials"`) |
| `blend_mode` | string | | `"Opaque"`, `"Masked"`, `"Translucent"`, `"Additive"` (default: `"Translucent"`) |
| `shading_model` | string | | `"DefaultLit"`, `"Unlit"`, `"SubsurfaceProfile"` (default: `"Unlit"`) |
| `used_with_niagara_sprites` | boolean | | Enable for Niagara sprite particles |
| `used_with_niagara_ribbons` | boolean | | Enable for Niagara ribbon particles |
| `used_with_niagara_mesh_particles` | boolean | | Enable for Niagara mesh particles |
| `used_with_skeletal_mesh` | boolean | | Enable for skeletal meshes |
| `used_with_static_lighting` | boolean | | Enable for static lighting |

**Example:**
```
create_material(
  name="M_EmberParticle",
  path="/Game/Effects/Materials",
  blend_mode="Additive",
  shading_model="Unlit",
  used_with_niagara_sprites=True
)
```

---

### `create_material_instance`

Create a new Material Instance Constant from a parent material.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the Material Instance (e.g., `"MI_Crystal_Red"`) |
| `parent_material` | string | ✅ | Path or name of the parent material |
| `folder_path` | string | | Folder path for the new asset |
| `scalar_params` | object | | Dictionary of scalar parameters `{"Metallic": 0.8}` |
| `vector_params` | object | | Dictionary of vector parameters `{"BaseColor": [1.0, 0.0, 0.0, 1.0]}` |
| `texture_params` | object | | Dictionary of texture parameters `{"NormalMap": "/Game/Textures/T_Name"}` |

**Example:**
```
create_material_instance(
  name="MI_Crystal_Red",
  parent_material="/Game/Materials/M_Crystal",
  scalar_params={"Metallic": 0.8, "Roughness": 0.2},
  vector_params={"BaseColor": [1.0, 0.0, 0.0, 1.0]}
)
```

---

### `duplicate_material_instance`

Duplicate an existing Material Instance to create a variation while preserving all current parameter values.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `source_material_instance` | string | ✅ | Path or name of the source Material Instance |
| `new_name` | string | ✅ | Name for the new duplicated Material Instance |
| `folder_path` | string | | Folder path for the new asset |

---

### `set_material_scalar_param`

Set a scalar (float) parameter on a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_instance` | string | ✅ | Path or name of the Material Instance |
| `param_name` | string | ✅ | Parameter name (e.g., `"Roughness"`) |
| `value` | number | ✅ | Float value to set |

---

### `set_material_vector_param`

Set a vector (color/4D vector) parameter on a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_instance` | string | ✅ | Path or name of the Material Instance |
| `param_name` | string | ✅ | Parameter name (e.g., `"BaseColor"`) |
| `r` | number | ✅ | Red component (0.0-1.0) |
| `g` | number | ✅ | Green component |
| `b` | number | ✅ | Blue component |
| `a` | number | | Alpha component (default: 1.0) |

---

### `set_material_texture_param`

Set a texture parameter on a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_instance` | string | ✅ | Path or name of the Material Instance |
| `param_name` | string | ✅ | Parameter name (e.g., `"NormalMap"`) |
| `texture_path` | string | ✅ | Path to the texture asset |

---

### `batch_set_material_params`

Set multiple parameters on a Material Instance in a single operation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_instance` | string | ✅ | Path or name of the Material Instance |
| `scalar_params` | object | | Dictionary of scalar parameters |
| `vector_params` | object | | Dictionary of vector parameters |
| `texture_params` | object | | Dictionary of texture parameters |

---

### `get_material_instance_metadata`

Get metadata and current parameter values from a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_instance` | string | ✅ | Path or name of the Material Instance |

---

### `get_material_parameters`

Get all available parameters from a Material (parent or instance) — names, types, groups, and current values.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material` | string | ✅ | Path or name of the Material |

---

### `get_material_graph_metadata`

Get comprehensive metadata about a material's expression graph — expressions, connections, outputs, orphans, and data flow.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `fields` | string[] | | Filter which fields to return: `"expressions"`, `"connections"`, `"material_outputs"`, `"orphans"`, `"flow"` |

---

### `add_material_expression`

Add a material expression node to a material graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `expression_type` | string | ✅ | Type of expression (e.g., `"Constant"`, `"Multiply"`, `"ParticleColor"`) |
| `position` | array | | `[X, Y]` position in the graph |
| `properties` | object | | Dictionary of expression properties |

---

### `set_material_expression_property`

Set a property on an existing material expression node.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `expression_id` | string | ✅ | GUID of the expression |
| `property_name` | string | ✅ | Property name (e.g., `"Texture"`, `"R"`, `"Constant"`) |
| `property_value` | string | ✅ | Value to set |

---

### `connect_material_expressions`

Connect two material expressions together.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `source_expression_id` | string | ✅ | GUID of the source expression |
| `target_expression_id` | string | ✅ | GUID of the target expression |
| `target_input_name` | string | ✅ | Name of the input on the target (e.g., `"A"`, `"B"`) |
| `source_output_index` | number | | Output index on source (default: 0) |

---

### `connect_expression_to_material_output`

Connect a material expression to a material output property.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `expression_id` | string | ✅ | GUID of the source expression |
| `material_property` | string | ✅ | Target property: `"BaseColor"`, `"Metallic"`, `"Roughness"`, `"Normal"`, `"EmissiveColor"`, `"Opacity"`, `"OpacityMask"`, `"WorldPositionOffset"`, etc. |
| `output_index` | number | | Output index on source expression (default: 0) |

---

### `delete_material_expression`

Delete a material expression from a material graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `expression_id` | string | ✅ | GUID of the expression to delete |

---

### `set_material_properties`

Set properties on an existing base Material (blend mode, shading model, usage flags).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |
| `material_domain` | string | | `"Surface"`, `"DeferredDecal"`, `"LightFunction"`, `"Volume"`, `"PostProcess"`, `"UserInterface"` |
| `blend_mode` | string | | `"Opaque"`, `"Masked"`, `"Translucent"`, `"Additive"`, `"Modulate"`, `"AlphaComposite"` |
| `shading_model` | string | | `"Unlit"`, `"DefaultLit"`, `"Subsurface"`, `"ClearCoat"`, `"Hair"`, `"Cloth"`, `"Eye"`, etc. |
| `two_sided` | boolean | | Whether the material renders on both sides |
| `used_with_niagara_sprites` | boolean | | Enable for Niagara sprite particles |
| `used_with_niagara_ribbons` | boolean | | Enable for Niagara ribbon particles |

---

### `search_material_palette`

Search the Material Palette for expressions and functions.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `search_query` | string | | Text to search (case-insensitive). Leave empty to list all |
| `category_filter` | string | | Filter by category (e.g., `"Math"`, `"Texture"`, `"Utility"`) |
| `type_filter` | string | | `"Expression"`, `"Function"`, or `"All"` (default) |
| `max_results` | number | | Maximum results (default: 50) |

---

### `compile_material`

Compile a material to apply changes and trigger shader recompilation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | ✅ | Path to the material |

## Advanced Usage Patterns

### Creating a Complete PBR Material from Scratch

```
"Build a full PBR material for crystal:
1. Create a base material called 'M_Crystal' with default lit shading and opaque blend mode
2. Add a TextureSample expression for the base color texture
3. Add scalar parameters for Metallic, Roughness, and EmissiveIntensity
4. Add a Multiply node to scale emissive color by intensity
5. Connect expressions to BaseColor, Metallic, Roughness, Normal, and EmissiveColor outputs
6. Compile the material
7. Create a material instance 'MI_Crystal_Red' with red base color, high metallic, and low roughness"
```

### Creating a Niagara-Ready Particle Material

```
"Set up a particle material for fire effects:
1. Create an additive unlit material called 'M_FireParticle' with Niagara sprite usage enabled
2. Search the material palette for ParticleColor and ParticleAlpha expressions
3. Add a TextureSample for the fire texture
4. Add a Multiply node to combine texture color with ParticleColor
5. Connect the result to EmissiveColor and particle alpha to Opacity
6. Compile the material
7. Create instances MI_FireParticle_Orange and MI_FireParticle_Blue with different emissive tints"
```

### Building Material Variations with Instances

```
"Create a family of crystal material variations:
1. Get the parameters available on M_Crystal to see what's exposed
2. Create MI_Crystal_Red with base color [1,0,0,1], metallic 0.9, roughness 0.1
3. Duplicate MI_Crystal_Red as MI_Crystal_Blue
4. Batch update MI_Crystal_Blue: base color to [0,0.2,1,1], emissive intensity 3.0
5. Duplicate MI_Crystal_Red as MI_Crystal_Green
6. Set the base color on MI_Crystal_Green to [0,1,0.3,1]"
```

## Best Practices for Natural Language Commands

### Use Full Asset Paths for Clarity
Instead of: *"Set the texture on the material"*
Use: *"Set the NormalMap texture parameter on MI_Crystal_Red to /Game/Textures/T_Crystal_N"*

### Batch Parameter Changes Together
Instead of setting scalar, vector, and texture params one at a time, use:
*"Batch update MI_Crystal_Red: metallic 0.9, roughness 0.1, emissive intensity 2.0, and base color to dark red"*

### Inspect Before Modifying
Before making changes, ask: *"Get the material parameters for M_Crystal"* or *"Show me the expression graph metadata for M_FireParticle"* to understand the current state.

### Specify Expression Types Precisely
When adding nodes, use the exact expression type name: *"Add a Multiply node"*, *"Add a LinearInterpolate node"*, *"Add a ParticleColor expression"*. Use `search_material_palette` when unsure.

### Always Compile After Graph Changes
After adding or connecting expressions, always: *"Compile M_Crystal to apply the changes"*. Uncompiled materials won't reflect graph edits in the viewport.

## Common Use Cases

### Environment Materials
Creating PBR materials for terrain, rocks, foliage, and architectural surfaces with full texture sets (base color, normal, roughness, ambient occlusion).

### Character and Creature Materials
Building materials for skeletal meshes with subsurface scattering profiles, cloth shading, or eye/hair shading models — enabling `used_with_skeletal_mesh`.

### VFX and Particle Materials
Creating additive or translucent unlit materials for Niagara particle systems, with ParticleColor integration and sprite/ribbon/mesh usage flags.

### UI and Decal Materials
Setting material domain to UserInterface or DeferredDecal for HUD elements and projected surface details.

### Material Instance Libraries
Rapidly generating material variations by duplicating instances and batch-adjusting parameters — useful for color palettes, wear levels, or biome-specific looks.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"Material not found"**: Verify the asset path with the full content path (e.g., `/Game/Materials/M_Crystal`). Use `get_material_parameters` to confirm the asset exists.
2. **Expression connection failures**: Use `get_material_graph_metadata` to inspect existing expressions and their GUIDs before attempting connections. Input names are case-sensitive (e.g., `"A"`, `"B"` for Multiply).
3. **Material Instance parameter not applying**: The parameter name must match exactly what the parent material exposes. Use `get_material_parameters` on the parent to list available parameter names.
4. **Shader compilation errors after graph edits**: Check for orphaned expressions or missing connections with `get_material_graph_metadata` using the `"orphans"` field. Ensure all required material outputs are connected.
5. **Usage flag mismatch**: If a material appears incorrect on Niagara particles or skeletal meshes, verify the appropriate `used_with_*` flag is enabled via `set_material_properties`.

## Performance Considerations

- **Minimize expression count**: Complex graphs with many nodes increase shader instruction count and compile time. Combine operations where possible (e.g., use a single Multiply instead of chaining multiple nodes).
- **Use Material Instances for variations**: Never duplicate base materials just to change parameter values — create Material Instances instead. They share the compiled shader and only override parameter data.
- **Compile strategically**: Material compilation triggers shader recompilation, which can be expensive. Batch all graph edits before calling `compile_material` once at the end.
- **Choose blend modes carefully**: Translucent and additive materials are significantly more expensive than opaque. Use masked blend mode when you only need binary transparency.
