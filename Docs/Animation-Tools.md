# Animation Tools - Unreal MCP

This document provides comprehensive information about using Animation Blueprint tools through the Unreal MCP (Model Context Protocol). These tools allow you to create and configure Animation Blueprints, state machines, states, transitions, variables, and animation slots using natural language commands.

## Overview

Animation tools enable you to:
- Create Animation Blueprints with custom skeleton and parent class
- Inspect Animation Blueprint metadata (variables, state machines, AnimGraph nodes)
- Create state machines for organizing animation states and transitions
- Add states with animation assets (sequences, blend spaces)
- Configure transitions between states with different rule types and blend durations
- Add animation variables (Bool, Float, Int, Vector, Rotator) for controlling logic
- Configure animation slots for montages and dynamic animations
- Connect AnimGraph nodes (state machines to output pose)
- Link animation layers for modular animation logic

## Natural Language Usage Examples

### Creating Animation Blueprints

```
"Create an Animation Blueprint called 'ABP_Player' for the player skeleton"

"Create an Animation Blueprint based on my custom AnimInstance class"

"Show me the metadata for ABP_Player — what state machines and variables does it have?"
```

### Building State Machines

```
"Create a state machine called 'Locomotion' in ABP_Player"

"Add an Idle state to the Locomotion state machine with the idle animation"

"Add Walk and Run states, then create transitions between Idle->Walk->Run"

"Set the blend duration to 0.15 seconds on the Idle to Walk transition"

"Connect the Locomotion state machine to the output pose"
```

### Variables and Slots

```
"Add a float variable called 'Speed' to ABP_Player"

"Add a boolean variable 'IsInAir' to control jump transitions"

"Configure an UpperBody animation slot for montage playback"
```

### Animation Layers

```
"Link the combat animation layer interface to ABP_Player"
```

## Tool Reference

---

### `create_animation_blueprint`

Create a new Animation Blueprint.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the Animation Blueprint (e.g., `"ABP_Player"`) |
| `skeleton_path` | string | ✅ | Path to the target skeleton asset |
| `folder_path` | string | | Folder path for the blueprint |
| `parent_class` | string | | Parent AnimInstance class (default: UAnimInstance) |
| `compile_on_creation` | boolean | | Whether to compile after creation (default: True) |

**Example:**
```
create_animation_blueprint(
  name="ABP_Player",
  skeleton_path="/Game/Characters/Player/Skeleton",
  folder_path="/Game/Characters/Player"
)
```

---

### `get_anim_blueprint_metadata`

Get metadata from an Animation Blueprint — variables, state machines, AnimGraph nodes, transitions, and linked layers.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the Animation Blueprint |

Returns state machines with their states, entry state, transitions (including blend duration and rule type), all AnimGraph nodes with positions and root connection status, and linked animation layers.

---

### `create_anim_state_machine`

Create a state machine in an Animation Blueprint's AnimGraph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `state_machine_name` | string | ✅ | Name for the new state machine (e.g., `"Locomotion"`) |

**Example:**
```
create_anim_state_machine(
  anim_blueprint_name="ABP_Player",
  state_machine_name="Locomotion"
)
```

---

### `add_anim_state`

Add a state to a state machine in an Animation Blueprint.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `state_machine_name` | string | ✅ | Name of the state machine |
| `state_name` | string | ✅ | Name for the new state (e.g., `"Idle"`, `"Walk"`, `"Run"`) |
| `animation_asset_path` | string | | Path to the animation asset (sequence, blend space) |
| `is_default_state` | boolean | | Whether this should be the default/entry state |
| `node_position_x` | number | | X position in the graph |
| `node_position_y` | number | | Y position in the graph |

**Example:**
```
add_anim_state(
  anim_blueprint_name="ABP_Player",
  state_machine_name="Locomotion",
  state_name="Idle",
  animation_asset_path="/Game/Animations/Idle_Anim",
  is_default_state=True
)
```

---

### `add_anim_transition`

Add a transition between two states in a state machine.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `state_machine_name` | string | ✅ | Name of the state machine |
| `from_state` | string | ✅ | Source state name |
| `to_state` | string | ✅ | Destination state name |
| `transition_rule_type` | string | | `"TimeRemaining"`, `"BoolVariable"`, `"CrossfadeBlend"` (default), `"Inertialization"`, `"Custom"` |
| `blend_duration` | number | | Blend transition duration in seconds (default: 0.2) |
| `condition_variable` | string | | Variable name for bool-based transitions |

**Example:**
```
add_anim_transition(
  anim_blueprint_name="ABP_Player",
  state_machine_name="Locomotion",
  from_state="Idle",
  to_state="Walk",
  transition_rule_type="CrossfadeBlend",
  blend_duration=0.15
)
```

---

### `add_anim_variable`

Add a variable to an Animation Blueprint for controlling animation logic and transitions.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `variable_name` | string | ✅ | Variable name (e.g., `"Speed"`, `"IsInAir"`) |
| `variable_type` | string | ✅ | `"Bool"`, `"Float"`, `"Int"`, `"Vector"`, `"Rotator"` |
| `default_value` | string | | Default value as string (e.g., `"0.0"`, `"true"`) |

**Example:**
```
add_anim_variable(
  anim_blueprint_name="ABP_Player",
  variable_name="Speed",
  variable_type="Float",
  default_value="0.0"
)
```

---

### `configure_anim_slot`

Configure an animation slot for montages and dynamic animations without disrupting the base state machine.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `slot_name` | string | ✅ | Slot name (e.g., `"UpperBody"`, `"FullBody"`, `"DefaultSlot"`) |
| `slot_group` | string | | Slot group name for organizing related slots |

---

### `connect_anim_graph_nodes`

Connect nodes in an Animation Blueprint's AnimGraph — typically connecting a state machine to the root output pose.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `source_node_name` | string | ✅ | Name of the source node (e.g., state machine name `"Locomotion"`) |
| `target_node_name` | string | | Target node name. Empty, `"OutputPose"`, or `"Root"` connects to the AnimGraph's root output |
| `source_pin_name` | string | | Source output pin name (default: `"Pose"`) |
| `target_pin_name` | string | | Target input pin name (default: `"Result"`) |

**Example:**
```
connect_anim_graph_nodes(
  anim_blueprint_name="ABP_Player",
  source_node_name="Locomotion"
)
```

---

### `link_animation_layer`

Link an animation layer to an Animation Blueprint for modular animation logic.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `anim_blueprint_name` | string | ✅ | Name of the target Animation Blueprint |
| `layer_interface` | string | ✅ | Name of the layer interface (e.g., `"IAnimLayerInterface_Combat"`) |
| `layer_class` | string | | Layer class implementing the interface |

## Advanced Usage Patterns

### Building a Complete Character Locomotion System

```
"Create a full locomotion Animation Blueprint:
1. Create an Animation Blueprint 'ABP_Player' for the player skeleton
2. Add float variables 'Speed' and 'Direction' for movement control
3. Add boolean variables 'IsInAir' and 'IsCrouching' for state tracking
4. Create a state machine called 'Locomotion'
5. Add states: Idle (default), Walk, Run, Jump, Fall, Land
6. Set each state's animation asset to the corresponding animation sequence
7. Create transitions: Idle→Walk, Walk→Run, Run→Walk, Walk→Idle with CrossfadeBlend
8. Create Jump→Fall and Fall→Land transitions with TimeRemaining rules
9. Set blend duration to 0.15 for ground transitions and 0.1 for air transitions
10. Connect the Locomotion state machine to the output pose"
```

### Setting Up Combat Animation Layers

```
"Add combat animations to the existing ABP_Player:
1. Get the metadata for ABP_Player to see current state machines
2. Create a second state machine called 'UpperBody' for combat
3. Add states: CombatIdle, Attack1, Attack2, Block
4. Add transitions between combat states
5. Configure an UpperBody animation slot for attack montages
6. Add a boolean variable 'IsInCombat' for enabling the combat layer
7. Link the IAnimLayerInterface_Combat layer to ABP_Player
8. Connect the UpperBody state machine to the output pose"
```

### Creating an NPC Animation Blueprint with Multiple States

```
"Build an NPC animation system:
1. Create ABP_NPC for the NPC skeleton
2. Add variables: Speed (Float), IsAlert (Bool), IsAttacking (Bool)
3. Create a state machine 'NPCBehavior'
4. Add states: Idle (default), Patrol, Alert, Chase, Attack, Death
5. Create transitions: Idle→Patrol, Patrol→Alert (BoolVariable: IsAlert)
6. Create transitions: Alert→Chase, Chase→Attack (BoolVariable: IsAttacking)
7. Add a Death state accessible from any state
8. Set short blend durations (0.1s) for combat transitions, longer (0.3s) for idle/patrol
9. Connect to output pose"
```

## Best Practices for Natural Language Commands

### Set the Default State Explicitly
When adding states, mark one as default: *"Add an Idle state to Locomotion as the default state with the idle animation"* — otherwise the state machine has no entry point.

### Use Descriptive Variable Names
Instead of: *"Add a bool variable 'b1'"*
Use: *"Add a boolean variable 'IsInAir' to control jump transitions in ABP_Player"* — clear naming helps with transition condition setup.

### Specify Transition Rule Types
Instead of: *"Add a transition from Idle to Walk"*
Use: *"Add a CrossfadeBlend transition from Idle to Walk with 0.15 second blend duration"* — this ensures the correct blending behavior.

### Get Metadata Before Adding Transitions
Before connecting states: *"Show me the metadata for ABP_Player"* to verify state names, existing transitions, and variable availability.

### Connect to Output Pose Last
Build your full state machine structure first, then: *"Connect the Locomotion state machine to the output pose"* — connecting early and then modifying can sometimes cause issues.

## Common Use Cases

### Player Locomotion
Building state machines with Idle, Walk, Run, Sprint, Jump, Fall, and Land states — controlled by Speed and IsInAir variables with smooth crossfade blending.

### Combat Systems
Creating attack state machines with combo chains, block states, and hit reactions — using animation slots for montage-based attacks that layer on top of locomotion.

### NPC and AI Behavior
Designing AI animation systems with patrol, alert, chase, attack, and death states — driven by boolean variables set from behavior trees or AI controllers.

### Cinematic Characters
Setting up Animation Blueprints for cutscene characters with custom AnimInstance parent classes and linked animation layers for facial expressions and body gestures.

### Vehicle and Mount Animations
Creating state machines for mount/dismount sequences, vehicle entry/exit, and in-vehicle idle/steering animations with specialized transition rules.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"Animation Blueprint not found"**: Verify the blueprint name (not the full path). Use the asset name as it appears in the Content Browser (e.g., `"ABP_Player"` not `/Game/Characters/ABP_Player`).
2. **State machine not appearing in output**: After creating states and transitions, you must explicitly connect the state machine to the output pose with `connect_anim_graph_nodes`. Use `get_anim_blueprint_metadata` to verify root connection status.
3. **Transition not working in-game**: Ensure the condition variable exists and matches the correct type. A `BoolVariable` transition needs the exact variable name — use `get_anim_blueprint_metadata` to list all variables.
4. **Animation asset not playing in state**: Verify the `animation_asset_path` points to a valid animation sequence or blend space that is compatible with the skeleton. Mismatched skeletons silently fail.
5. **Slot not receiving montages**: The slot name must match exactly what the montage targets. Common names are `"DefaultSlot"`, `"UpperBody"`, `"FullBody"` — check that both the ABP slot configuration and montage slot name agree.

## Performance Considerations

- **Keep state machine complexity reasonable**: Each state machine evaluates transitions every frame. Deeply nested state machines with many transitions increase per-frame CPU cost. Aim for focused, purpose-specific state machines (locomotion, combat, facial) rather than one massive tree.
- **Use Inertialization blending for responsiveness**: For combat and gameplay-critical transitions, `"Inertialization"` blending is more performant than `"CrossfadeBlend"` because it doesn't require simultaneously evaluating both source and target states during the blend.
- **Minimize variable count**: Each variable in the Animation Blueprint adds evaluation overhead. Remove unused variables and consolidate related booleans into enums where possible.
- **Configure slots only where needed**: Animation slots add evaluation nodes to the AnimGraph. Only configure slots that your montages actually target — unnecessary slots consume resources even when empty.
