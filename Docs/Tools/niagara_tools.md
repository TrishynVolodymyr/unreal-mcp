# Niagara VFX Tools

Tools for creating and manipulating Niagara particle systems, emitters, and parameters.

## Server

- **File**: `niagara_mcp_server.py`
- **Port**: 55557 (shared TCP)

---

## Tools

### System Operations

#### `create_niagara_system`

Create a new Niagara System asset.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `system_name` | string | Yes | - | Name for the Niagara System |
| `save_path` | string | No | "/Game/Effects" | Folder to save the system |

**Returns**: Success status and system path.

**Example**:
```python
create_niagara_system(
    system_name="NS_FireExplosion",
    save_path="/Game/Effects/Combat"
)
```

---

#### `duplicate_niagara_system`

Duplicate an existing Niagara System.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `source_path` | string | Yes | - | Path to source system |
| `new_name` | string | Yes | - | Name for the duplicate |
| `destination_path` | string | No | Same folder | Destination folder |

**Example**:
```python
duplicate_niagara_system(
    source_path="/Game/Effects/NS_FireExplosion",
    new_name="NS_IceExplosion",
    destination_path="/Game/Effects/Combat"
)
```

---

#### `get_niagara_system_metadata`

Get metadata about a Niagara System including emitters and parameters.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |

**Returns**:
- `system_path`: Full asset path
- `emitters`: List of emitters with names and enabled state
- `user_parameters`: List of exposed user parameters
- `system_parameters`: System-level parameters

**Example**:
```python
get_niagara_system_metadata(
    system_path="/Game/Effects/NS_FireExplosion"
)
```

---

#### `compile_niagara_system`

Compile or recompile a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |

**Example**:
```python
compile_niagara_system(
    system_path="/Game/Effects/NS_FireExplosion"
)
```

---

### Emitter Operations

#### `add_emitter_to_system`

Add an emitter to a Niagara System from a template.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |
| `emitter_template_path` | string | Yes | Path to emitter template asset |
| `emitter_name` | string | No | Custom name for the emitter |

**Common Engine Templates**:
- `/Niagara/DefaultAssets/Templates/SimpleSpriteBurst`
- `/Niagara/DefaultAssets/Templates/SimpleSpriteEmitter`
- `/Niagara/DefaultAssets/Templates/SimpleMeshEmitter`
- `/Niagara/DefaultAssets/Templates/SimpleGPUSpriteBurst`
- `/Niagara/DefaultAssets/Templates/SimpleRibbonEmitter`

**Example**:
```python
add_emitter_to_system(
    system_path="/Game/Effects/NS_FireExplosion",
    emitter_template_path="/Niagara/DefaultAssets/Templates/SimpleSpriteBurst",
    emitter_name="MainBurst"
)
```

---

#### `remove_emitter_from_system`

Remove an emitter from a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |
| `emitter_name` | string | Yes | Name of the emitter to remove |

**Example**:
```python
remove_emitter_from_system(
    system_path="/Game/Effects/NS_FireExplosion",
    emitter_name="UnusedEmitter"
)
```

---

#### `set_emitter_enabled`

Enable or disable an emitter in a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |
| `emitter_name` | string | Yes | Name of the emitter |
| `enabled` | bool | Yes | Whether to enable or disable |

**Example**:
```python
set_emitter_enabled(
    system_path="/Game/Effects/NS_FireExplosion",
    emitter_name="DebugParticles",
    enabled=False
)
```

---

### Parameter Operations

#### `set_niagara_float_param`

Set a float user parameter on a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |
| `param_name` | string | Yes | Name of the parameter |
| `value` | float | Yes | Value to set |

**Example**:
```python
set_niagara_float_param(
    system_path="/Game/Effects/NS_FireExplosion",
    param_name="SpawnRate",
    value=100.0
)
```

---

#### `set_niagara_vector_param`

Set a vector user parameter on a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |
| `param_name` | string | Yes | Name of the parameter |
| `value` | array | Yes | [X, Y, Z] vector values |

**Example**:
```python
set_niagara_vector_param(
    system_path="/Game/Effects/NS_FireExplosion",
    param_name="EmitterDirection",
    value=[0.0, 0.0, 1.0]
)
```

---

#### `set_niagara_color_param`

Set a color user parameter on a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |
| `param_name` | string | Yes | Name of the parameter |
| `value` | array | Yes | [R, G, B, A] color values (0.0-1.0) |

**Example**:
```python
set_niagara_color_param(
    system_path="/Game/Effects/NS_FireExplosion",
    param_name="ParticleColor",
    value=[1.0, 0.5, 0.0, 1.0]
)
```

---

#### `get_niagara_parameters`

List all available user parameters from a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | Yes | Path to the Niagara System |

**Returns**:
- `float_params`: List of float parameter names and values
- `vector_params`: List of vector parameter names and values
- `color_params`: List of color parameter names and values
- `bool_params`: List of bool parameter names and values

**Example**:
```python
get_niagara_parameters(
    system_path="/Game/Effects/NS_FireExplosion"
)
```

---

### Renderer Operations

#### `add_renderer_to_emitter`

Add a renderer to an emitter in a Niagara System.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `system_path` | string | Yes | - | Path to the Niagara System |
| `emitter_name` | string | Yes | - | Name of the emitter |
| `renderer_type` | string | Yes | - | Type of renderer to add |
| `material_path` | string | No | - | Optional material for the renderer |

**Renderer Types**:
- `Sprite` - Billboard sprites
- `Ribbon` - Trail ribbons
- `Mesh` - Static mesh particles
- `Light` - Dynamic light emitters
- `Component` - Actor component renderer

**Example**:
```python
add_renderer_to_emitter(
    system_path="/Game/Effects/NS_FireExplosion",
    emitter_name="MainBurst",
    renderer_type="Sprite",
    material_path="/Game/Materials/M_Fire_Particle"
)
```

---

## Complete Workflow Example

```python
# 1. Create a new Niagara System for a magic spell effect
create_niagara_system(
    system_name="NS_MagicProjectile",
    save_path="/Game/Effects/Magic"
)

# 2. Add main particle burst emitter
add_emitter_to_system(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    emitter_template_path="/Niagara/DefaultAssets/Templates/SimpleSpriteBurst",
    emitter_name="CoreBurst"
)

# 3. Add trail ribbon emitter
add_emitter_to_system(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    emitter_template_path="/Niagara/DefaultAssets/Templates/SimpleRibbonEmitter",
    emitter_name="Trail"
)

# 4. Add glow light emitter
add_emitter_to_system(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    emitter_template_path="/Niagara/DefaultAssets/Templates/SimpleSpriteBurst",
    emitter_name="GlowParticles"
)

# 5. Configure parameters
set_niagara_float_param(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    param_name="SpawnRate",
    value=50.0
)

set_niagara_color_param(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    param_name="ParticleColor",
    value=[0.3, 0.6, 1.0, 1.0]  # Blue magic
)

set_niagara_vector_param(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    param_name="VelocityScale",
    value=[100.0, 100.0, 100.0]
)

# 6. Add custom renderer with material
add_renderer_to_emitter(
    system_path="/Game/Effects/Magic/NS_MagicProjectile",
    emitter_name="CoreBurst",
    renderer_type="Sprite",
    material_path="/Game/Materials/M_MagicParticle"
)

# 7. Compile the system
compile_niagara_system(
    system_path="/Game/Effects/Magic/NS_MagicProjectile"
)

# 8. Create variants by duplication
duplicate_niagara_system(
    source_path="/Game/Effects/Magic/NS_MagicProjectile",
    new_name="NS_FireProjectile"
)

# 9. Modify the variant colors
set_niagara_color_param(
    system_path="/Game/Effects/Magic/NS_FireProjectile",
    param_name="ParticleColor",
    value=[1.0, 0.4, 0.0, 1.0]  # Orange fire
)
```

---

## Common Parameter Names

These are typical user parameter names for Niagara systems:

| Parameter | Type | Description |
|-----------|------|-------------|
| `SpawnRate` | Float | Particles per second |
| `Lifetime` | Float | Particle lifetime in seconds |
| `StartSize` | Float/Vector | Initial particle size |
| `EndSize` | Float/Vector | Final particle size |
| `StartVelocity` | Vector | Initial velocity |
| `VelocityScale` | Vector | Velocity multiplier |
| `ParticleColor` | Color | Base particle color |
| `StartColor` | Color | Initial color |
| `EndColor` | Color | Final color |
| `EmitterDirection` | Vector | Emission direction |
| `EmitterScale` | Float | Overall scale |
| `Intensity` | Float | Effect intensity |
| `Duration` | Float | System duration |
