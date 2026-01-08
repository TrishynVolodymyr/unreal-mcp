# Animation Blueprint Tools

Tools for creating and manipulating Animation Blueprints, state machines, and animation variables.

## Server

- **File**: `animation_mcp_server.py`
- **Port**: 55557 (shared TCP)

---

## Tools

### `create_animation_blueprint`

Create a new Animation Blueprint asset.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Name of the Animation Blueprint |
| `skeleton_path` | string | Yes | - | Path to the target skeleton asset |
| `folder_path` | string | No | "" | Folder path for the blueprint |
| `parent_class` | string | No | "" | Optional parent AnimInstance class |
| `compile_on_creation` | bool | No | true | Whether to compile after creation |

**Returns**: Success status, name, path, and skeleton reference.

**Example**:
```python
create_animation_blueprint(
    name="ABP_PlayerCharacter",
    skeleton_path="/Game/Characters/Player/Skeleton",
    folder_path="/Game/Characters/Player/Animations"
)
```

---

### `get_anim_blueprint_metadata`

Get metadata from an Animation Blueprint including variables, state machines, and graph structure.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | Yes | Name of the Animation Blueprint |

**Returns**:
- `name`: Blueprint name
- `path`: Full asset path
- `skeleton`: Associated skeleton path
- `parent_class`: Parent AnimInstance class
- `variables`: List of animation variables with types
- `linked_layers`: List of linked animation layers
- `has_root_connection`: Whether AnimGraph has valid root connection
- `animgraph_nodes`: Array of all AnimGraph nodes
- `state_machines`: Array of state machines with states and transitions

**Example**:
```python
get_anim_blueprint_metadata(anim_blueprint_name="ABP_PlayerCharacter")
```

---

### `create_anim_state_machine`

Create a state machine in an Animation Blueprint's AnimGraph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | Yes | Target Animation Blueprint |
| `state_machine_name` | string | Yes | Name for the new state machine |

**Example**:
```python
create_anim_state_machine(
    anim_blueprint_name="ABP_PlayerCharacter",
    state_machine_name="Locomotion"
)
```

---

### `add_anim_state`

Add a state to a state machine.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `anim_blueprint_name` | string | Yes | - | Target Animation Blueprint |
| `state_machine_name` | string | Yes | - | Target state machine |
| `state_name` | string | Yes | - | Name for the new state |
| `animation_asset_path` | string | No | "" | Path to animation asset |
| `is_default_state` | bool | No | false | Set as entry/default state |
| `node_position_x` | float | No | 0 | X position in graph |
| `node_position_y` | float | No | 0 | Y position in graph |

**Example**:
```python
add_anim_state(
    anim_blueprint_name="ABP_PlayerCharacter",
    state_machine_name="Locomotion",
    state_name="Idle",
    animation_asset_path="/Game/Animations/Idle_Loop",
    is_default_state=True
)
```

---

### `add_anim_transition`

Add a transition between two states in a state machine.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `anim_blueprint_name` | string | Yes | - | Target Animation Blueprint |
| `state_machine_name` | string | Yes | - | Target state machine |
| `from_state` | string | Yes | - | Source state name |
| `to_state` | string | Yes | - | Destination state name |
| `transition_rule_type` | string | No | "CrossfadeBlend" | Transition type |
| `blend_duration` | float | No | 0.2 | Blend duration in seconds |
| `condition_variable` | string | No | "" | Variable for bool-based transitions |

**Transition Rule Types**:
- `TimeRemaining` - Transitions when animation time remaining is below threshold
- `BoolVariable` - Transitions when a bool variable is true
- `CrossfadeBlend` - Simple crossfade blend (default)
- `Inertialization` - Use inertialization for smoother transitions
- `Custom` - Custom transition logic

**Example**:
```python
add_anim_transition(
    anim_blueprint_name="ABP_PlayerCharacter",
    state_machine_name="Locomotion",
    from_state="Idle",
    to_state="Walk",
    transition_rule_type="CrossfadeBlend",
    blend_duration=0.25
)
```

---

### `add_anim_variable`

Add a variable to an Animation Blueprint.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `anim_blueprint_name` | string | Yes | - | Target Animation Blueprint |
| `variable_name` | string | Yes | - | Name of the variable |
| `variable_type` | string | Yes | - | Type of variable |
| `default_value` | string | No | "" | Default value as string |

**Variable Types**:
- `Bool` - Boolean true/false
- `Float` - Floating-point number
- `Int` - Integer number
- `Vector` - 3D vector
- `Rotator` - Rotation

**Example**:
```python
add_anim_variable(
    anim_blueprint_name="ABP_PlayerCharacter",
    variable_name="Speed",
    variable_type="Float",
    default_value="0.0"
)
```

---

### `configure_anim_slot`

Configure an animation slot for montage playback.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `anim_blueprint_name` | string | Yes | - | Target Animation Blueprint |
| `slot_name` | string | Yes | - | Name of the slot |
| `slot_group` | string | No | "" | Optional slot group name |

**Example**:
```python
configure_anim_slot(
    anim_blueprint_name="ABP_PlayerCharacter",
    slot_name="UpperBody",
    slot_group="DefaultGroup"
)
```

---

### `connect_anim_graph_nodes`

Connect nodes in an Animation Blueprint's AnimGraph.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `anim_blueprint_name` | string | Yes | - | Target Animation Blueprint |
| `source_node_name` | string | Yes | - | Source node (e.g., state machine name) |
| `target_node_name` | string | No | "" | Target node (empty = root output pose) |
| `source_pin_name` | string | No | "Pose" | Source output pin |
| `target_pin_name` | string | No | "Result" | Target input pin |

**Example**:
```python
# Connect Locomotion state machine to output pose
connect_anim_graph_nodes(
    anim_blueprint_name="ABP_PlayerCharacter",
    source_node_name="Locomotion"
)
```

---

### `link_animation_layer`

Link an animation layer to an Animation Blueprint.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `anim_blueprint_name` | string | Yes | - | Target Animation Blueprint |
| `layer_interface` | string | Yes | - | Layer interface name |
| `layer_class` | string | No | "" | Optional layer implementation class |

**Example**:
```python
link_animation_layer(
    anim_blueprint_name="ABP_PlayerCharacter",
    layer_interface="IAnimLayerInterface_Combat"
)
```

---

## Complete Workflow Example

```python
# 1. Create the Animation Blueprint
create_animation_blueprint(
    name="ABP_Enemy",
    skeleton_path="/Game/Characters/Enemy/Skeleton"
)

# 2. Add animation variables
add_anim_variable(
    anim_blueprint_name="ABP_Enemy",
    variable_name="Speed",
    variable_type="Float"
)

add_anim_variable(
    anim_blueprint_name="ABP_Enemy",
    variable_name="IsInCombat",
    variable_type="Bool"
)

# 3. Create locomotion state machine
create_anim_state_machine(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion"
)

# 4. Add states
add_anim_state(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion",
    state_name="Idle",
    animation_asset_path="/Game/Animations/Enemy/Idle",
    is_default_state=True
)

add_anim_state(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion",
    state_name="Walk",
    animation_asset_path="/Game/Animations/Enemy/Walk"
)

add_anim_state(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion",
    state_name="Run",
    animation_asset_path="/Game/Animations/Enemy/Run"
)

# 5. Add transitions
add_anim_transition(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion",
    from_state="Idle",
    to_state="Walk",
    blend_duration=0.2
)

add_anim_transition(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion",
    from_state="Walk",
    to_state="Idle",
    blend_duration=0.2
)

add_anim_transition(
    anim_blueprint_name="ABP_Enemy",
    state_machine_name="Locomotion",
    from_state="Walk",
    to_state="Run",
    blend_duration=0.15
)

# 6. Connect state machine to output
connect_anim_graph_nodes(
    anim_blueprint_name="ABP_Enemy",
    source_node_name="Locomotion"
)

# 7. Compile
compile_blueprint(blueprint_name="ABP_Enemy")
```
