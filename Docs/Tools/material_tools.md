# Material Instance Tools

Tools for creating and manipulating Material Instances and their parameters.

## Server

- **File**: `material_mcp_server.py`
- **Port**: 55557 (shared TCP)

---

## Tools

### `create_material_instance`

Create a new Material Instance from a parent material.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `instance_name` | string | Yes | - | Name for the new instance |
| `parent_material_path` | string | Yes | - | Path to parent material |
| `save_path` | string | No | "/Game/Materials" | Folder to save the instance |

**Returns**: Success status, instance path, and parent reference.

**Example**:
```python
create_material_instance(
    instance_name="MI_Character_Red",
    parent_material_path="/Game/Materials/M_Character_Base",
    save_path="/Game/Materials/Instances"
)
```

---

### `set_material_scalar_param`

Set a scalar (float) parameter on a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `instance_path` | string | Yes | Path to the Material Instance |
| `param_name` | string | Yes | Name of the scalar parameter |
| `value` | float | Yes | Value to set |

**Example**:
```python
set_material_scalar_param(
    instance_path="/Game/Materials/MI_Character_Red",
    param_name="Metallic",
    value=0.8
)
```

---

### `set_material_vector_param`

Set a vector/color parameter on a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `instance_path` | string | Yes | Path to the Material Instance |
| `param_name` | string | Yes | Name of the vector parameter |
| `value` | array | Yes | [R, G, B, A] values (0.0-1.0) |

**Example**:
```python
set_material_vector_param(
    instance_path="/Game/Materials/MI_Character_Red",
    param_name="BaseColor",
    value=[1.0, 0.2, 0.2, 1.0]
)
```

---

### `set_material_texture_param`

Set a texture parameter on a Material Instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `instance_path` | string | Yes | Path to the Material Instance |
| `param_name` | string | Yes | Name of the texture parameter |
| `texture_path` | string | Yes | Path to the texture asset |

**Example**:
```python
set_material_texture_param(
    instance_path="/Game/Materials/MI_Character_Red",
    param_name="DiffuseTexture",
    texture_path="/Game/Textures/T_Character_Diffuse"
)
```

---

### `batch_set_material_params`

Set multiple parameters on a Material Instance in a single operation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `instance_path` | string | Yes | Path to the Material Instance |
| `scalar_params` | object | No | Dict of scalar param name -> value |
| `vector_params` | object | No | Dict of vector param name -> [R,G,B,A] |
| `texture_params` | object | No | Dict of texture param name -> texture path |

**Example**:
```python
batch_set_material_params(
    instance_path="/Game/Materials/MI_Character_Red",
    scalar_params={
        "Metallic": 0.5,
        "Roughness": 0.3,
        "EmissiveStrength": 2.0
    },
    vector_params={
        "BaseColor": [1.0, 0.0, 0.0, 1.0],
        "EmissiveColor": [1.0, 0.5, 0.0, 1.0]
    },
    texture_params={
        "DiffuseTexture": "/Game/Textures/T_Red_Diffuse"
    }
)
```

---

### `get_material_instance_metadata`

Get metadata about a Material Instance including parent and current parameter values.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `instance_path` | string | Yes | Path to the Material Instance |

**Returns**:
- `instance_path`: Full asset path
- `parent_material`: Parent material path
- `scalar_parameters`: Dict of current scalar parameter values
- `vector_parameters`: Dict of current vector parameter values
- `texture_parameters`: Dict of current texture parameter assignments

**Example**:
```python
get_material_instance_metadata(
    instance_path="/Game/Materials/MI_Character_Red"
)
```

---

### `get_material_parameters`

List all available parameters from a material or material instance.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `material_path` | string | Yes | Path to material or material instance |

**Returns**:
- `scalar_params`: List of scalar parameter names with default values
- `vector_params`: List of vector parameter names with default values
- `texture_params`: List of texture parameter names

**Example**:
```python
# Check what parameters are available before setting them
get_material_parameters(
    material_path="/Game/Materials/M_Character_Base"
)
```

---

### `duplicate_material_instance`

Duplicate an existing Material Instance.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `source_path` | string | Yes | - | Path to source Material Instance |
| `new_name` | string | Yes | - | Name for the duplicate |
| `destination_path` | string | No | Same folder | Folder for the duplicate |

**Example**:
```python
duplicate_material_instance(
    source_path="/Game/Materials/MI_Character_Red",
    new_name="MI_Character_Blue",
    destination_path="/Game/Materials/Variants"
)
```

---

## Complete Workflow Example

```python
# 1. Check available parameters on base material
params = get_material_parameters(
    material_path="/Game/Materials/M_PBR_Master"
)
# Returns: scalar_params, vector_params, texture_params

# 2. Create instances for different character variants
create_material_instance(
    instance_name="MI_Character_Warrior",
    parent_material_path="/Game/Materials/M_PBR_Master",
    save_path="/Game/Characters/Materials"
)

# 3. Configure the instance with batch operation
batch_set_material_params(
    instance_path="/Game/Characters/Materials/MI_Character_Warrior",
    scalar_params={
        "Metallic": 0.9,
        "Roughness": 0.2,
        "NormalStrength": 1.5
    },
    vector_params={
        "BaseColor": [0.8, 0.7, 0.6, 1.0],  # Bronze tint
        "SubsurfaceColor": [0.9, 0.8, 0.7, 1.0]
    },
    texture_params={
        "DiffuseTexture": "/Game/Characters/Textures/T_Warrior_Diffuse",
        "NormalMap": "/Game/Characters/Textures/T_Warrior_Normal",
        "RoughnessMap": "/Game/Characters/Textures/T_Warrior_Roughness"
    }
)

# 4. Create variant by duplication
duplicate_material_instance(
    source_path="/Game/Characters/Materials/MI_Character_Warrior",
    new_name="MI_Character_Warrior_Gold"
)

# 5. Modify the variant
set_material_vector_param(
    instance_path="/Game/Characters/Materials/MI_Character_Warrior_Gold",
    param_name="BaseColor",
    value=[1.0, 0.85, 0.0, 1.0]  # Gold tint
)

set_material_scalar_param(
    instance_path="/Game/Characters/Materials/MI_Character_Warrior_Gold",
    param_name="Metallic",
    value=1.0
)
```

---

## Common Parameter Names

These are typical parameter names found in PBR materials:

| Parameter | Type | Typical Range | Description |
|-----------|------|---------------|-------------|
| `BaseColor` | Vector | 0-1 RGBA | Albedo/diffuse color |
| `Metallic` | Scalar | 0-1 | Metal vs non-metal |
| `Roughness` | Scalar | 0-1 | Surface smoothness |
| `Specular` | Scalar | 0-1 | Specular intensity |
| `EmissiveColor` | Vector | 0-1 RGBA | Glow color |
| `EmissiveStrength` | Scalar | 0-100+ | Glow intensity |
| `Opacity` | Scalar | 0-1 | Transparency |
| `NormalStrength` | Scalar | 0-2 | Normal map intensity |
| `DiffuseTexture` | Texture | - | Base color texture |
| `NormalMap` | Texture | - | Normal/bump texture |
| `RoughnessMap` | Texture | - | Roughness texture |
| `MetallicMap` | Texture | - | Metallic texture |
| `AOMap` | Texture | - | Ambient occlusion |
