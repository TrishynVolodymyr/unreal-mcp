# Node Tools - Unreal MCP

This document provides comprehensive information about using Node tools through the Unreal MCP (Model Context Protocol). These tools allow you to create and manage Blueprint node graphs, build visual scripting logic, and connect gameplay systems using natural language commands through AI assistants.

## Overview

Node tools enable you to:
- Add event nodes for standard Blueprint events (BeginPlay, Tick, etc.)
- Create input action event nodes for player controls
- Add function call nodes with component targets
- Connect nodes to build execution and data flow
- Add component reference nodes
- Find and manage existing nodes in Blueprint graphs
- Get variable information for automation

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Event Nodes

**Adding Standard Event Nodes:**
```
"Add a BeginPlay event node to the PlayerCharacter Blueprint"

"Create a Tick event in the VehicleController Blueprint"

"Add an ActorBeginOverlap event to the TriggerZone Blueprint"

"Create an ActorEndOverlap event in the InteractableObject Blueprint"

"Add a ReceiveBeginPlay event node to my GameManager Blueprint"
```

**Positioning Event Nodes:**
```
"Add a BeginPlay event at position [100, 200] in the PlayerCharacter Blueprint"

"Create a Tick event node at coordinates [-200, 300] in the enemy Blueprint"

"Add an ActorBeginOverlap event at position [0, 500] in the trigger Blueprint"
```

### Input Action Nodes

**Adding Input Action Event Nodes:**
```
"Add an input action event for 'Jump' to the PlayerCharacter Blueprint"

"Create an input event for the 'Fire' action in the weapon Blueprint"

"Add an 'Interact' input action node to the player Blueprint"

"Create input event nodes for 'MoveForward' and 'MoveRight' actions"

"Add an input action event for 'OpenInventory' to the UI controller Blueprint"
```

**Positioning Input Action Nodes:**
```
"Add a 'Jump' input action event at position [200, 100] in the player Blueprint"

"Create a 'Fire' input action node at coordinates [300, 200] in the weapon Blueprint"

"Add an 'Interact' input event at position [-100, 400] in the character Blueprint"
```

### Function Call Nodes

**Adding Component Function Calls:**
```
"Add a function call node for 'SetActorLocation' targeting 'self' in the PlayerCharacter Blueprint"

"Create a 'SetStaticMesh' function call targeting the 'MainMesh' component"

"Add a 'SetLightColor' function call for the 'PointLight' component in the lamp Blueprint"

"Create a 'PlaySound' function call targeting the 'AudioComponent' in the weapon Blueprint"
```

**Function Calls with Parameters:**
```
"Add a 'SetActorLocation' function call with NewLocation parameter set to [0, 0, 100]"

"Create a 'SetLightIntensity' function call with Intensity parameter 5000 for the MainLight component"

"Add a 'SetStaticMesh' function call with the cube mesh parameter for the Platform component"
```

**Positioning Function Call Nodes:**
```
"Add a 'SetActorLocation' function call at position [400, 200] targeting self"

"Create a 'PlaySound' function call at coordinates [500, 300] for the AudioComponent"

"Add a 'SetLightColor' function call at position [600, 100] for the PointLight component"
```

### Component Reference Nodes

**Adding Component References:**
```
"Add a reference to the 'MainMesh' component in the PlayerCharacter Blueprint"

"Create a component reference node for 'PointLight' in the lamp Blueprint"

"Add a reference to the 'CameraComponent' in the vehicle Blueprint"

"Create a component reference for 'SphereCollision' in the trigger Blueprint"
```

**Positioning Component References:**
```
"Add a 'MainMesh' component reference at position [200, 300] in the player Blueprint"

"Create a 'PointLight' reference node at coordinates [100, 400] in the lamp Blueprint"

"Add a 'CameraComponent' reference at position [300, 500] in the vehicle Blueprint"
```

### Self Reference Nodes

**Adding Self References:**
```
"Add a 'Get Self' reference node to the PlayerCharacter Blueprint"

"Create a self reference at position [100, 100] in the enemy Blueprint"

"Add a self reference node to get this actor in the interactable Blueprint"

"Create a 'Get Self' node at coordinates [200, 200] in the game manager Blueprint"
```

### Node Connections

**Connecting Execution Pins:**
```
"Connect the BeginPlay event to the SetActorLocation function call in PlayerCharacter"

"Link the Jump input action to the AddActorWorldOffset function call"

"Connect the ActorBeginOverlap event to the PlaySound function call in the trigger Blueprint"

"Link the Tick event execution to the UpdateHealthBar function call"
```

**Connecting Data Pins:**
```
"Connect the MainMesh component reference output to the SetStaticMesh function input"

"Link the self reference output to the SetActorLocation target input"

"Connect the PointLight reference to the SetLightColor component input"

"Link the CameraComponent reference output to the SetWorldRotation target input"
```

**Complex Node Connections:**
```
"Connect BeginPlay event execution to SetActorLocation, then connect self reference to the target input of SetActorLocation"

"Link Jump input action to AddActorWorldOffset, and connect the movement vector [0, 0, 500] to the DeltaLocation input"

"Connect ActorBeginOverlap to PlaySound, and link the AudioComponent reference to the component input"
```

### Finding and Managing Nodes

**Finding Nodes:**
```
"Find all event nodes in the PlayerCharacter Blueprint"

"Show me all function call nodes in the weapon Blueprint"

"Find BeginPlay event nodes in the game manager Blueprint"

"List all input action nodes in the player controller Blueprint"

"Find nodes of type 'Event' in the interactable Blueprint"
```

**Finding Specific Event Types:**
```
"Find BeginPlay events in the PlayerCharacter Blueprint"

"Locate Tick event nodes in the enemy AI Blueprint"

"Find ActorBeginOverlap events in the trigger Blueprint"

"Show me all InputAction events in the player Blueprint"
```

### Variable Information

**Getting Variable Types:**
```
"What type is the 'Health' variable in the PlayerCharacter Blueprint?"

"Get the variable type information for 'PlayerStats' in the character Blueprint"

"Show me the type of the 'Inventory' variable in the player Blueprint"

"What's the data type of the 'WeaponData' variable in the weapon Blueprint?"
```

## Advanced Usage Patterns

### Building Complete Event Chains

**Player Movement Setup:**
```
"Set up player movement in PlayerCharacter Blueprint:
1. Add 'MoveForward' input action event at [100, 100]
2. Add 'AddActorWorldOffset' function call targeting self at [300, 100]
3. Connect the input action execution to the function call
4. Connect self reference to the function target input"
```

**Trigger System:**
```
"Create trigger system in TriggerZone Blueprint:
1. Add ActorBeginOverlap event at [100, 200]
2. Add PlaySound function call for AudioComponent at [400, 200]
3. Add SetLightColor function call for PointLight at [400, 300]
4. Connect overlap event to both function calls
5. Connect component references to respective function inputs"
```

### Interactive Object Logic

**Door Opening System:**
```
"Build door opening logic in InteractiveDoor Blueprint:
1. Add 'Interact' input action event
2. Add 'SetActorRotation' function call targeting self
3. Add 'PlaySound' function call for door sound
4. Connect input action to both function calls
5. Set rotation parameter to [0, 90, 0] for open position"
```

**Health Pickup Logic:**
```
"Create health pickup in HealthPotion Blueprint:
1. Add ActorBeginOverlap event
2. Add 'HealPlayer' custom function call
3. Add 'DestroyActor' function call targeting self
4. Connect overlap event to heal function, then to destroy function
5. Add component references as needed for visual effects"
```

### Combat System Nodes

**Weapon Firing System:**
```
"Set up weapon firing in WeaponBlueprint:
1. Add 'Fire' input action event
2. Add 'SpawnProjectile' function call
3. Add 'PlaySound' function call for gunshot
4. Add 'PlayAnimation' function call for muzzle flash
5. Connect fire action to all function calls in sequence"
```

**Damage Dealing:**
```
"Create damage system in ProjectileBlueprint:
1. Add ActorHit event
2. Add 'ApplyDamage' function call
3. Add 'SpawnEffect' function call for impact
4. Add 'DestroyActor' function call targeting self
5. Connect hit event through damage and effect to destruction"
```

## Graph Manipulation Tools

These low-level tools allow you to modify existing Blueprint node graphs by disconnecting, deleting, or completely replacing nodes while preserving connections.

### Disconnect Node

Disconnect all pin connections from a node while keeping the node in the graph. Useful for preparing nodes for modification or replacement.

**Basic Usage:**
```
"Disconnect all connections from the 'Get Owner' node in DialogueComponent"

"Disconnect the 'SetActorLocation' node in PlayerCharacter's EventGraph"

"Disconnect all pins from the 'BranchNode_3' node in the CanInteract function"
```

**Parameters:**
- `blueprint_name`: Name of the Blueprint containing the node
- `node_id`: Unique identifier of the node to disconnect
- `target_graph`: Graph name (defaults to "EventGraph", use function name for custom functions like "CanInteract")
- `disconnect_inputs`: Whether to disconnect input pins (default: true)
- `disconnect_outputs`: Whether to disconnect output pins (default: true)

**Response:**
```json
{
  "success": true,
  "message": "Disconnected 3 connections from node",
  "disconnections": 3
}
```

**Use Cases:**
- Preparing nodes for replacement without deleting them yet
- Isolating nodes to test different connection patterns
- Cleaning up connections before manual rewiring
- Debugging connection issues by starting fresh

### Delete Node

Permanently remove a node from the Blueprint graph. Automatically disconnects all pins before deletion.

**Basic Usage:**
```
"Delete the 'Get Owner' node from DialogueComponent"

"Remove the broken PrintString node from the EventGraph"

"Delete the unused 'BranchNode_2' from the CanInteract function"
```

**Parameters:**
- `blueprint_name`: Name of the Blueprint containing the node
- `node_id`: Unique identifier of the node to delete
- `target_graph`: Graph name (defaults to "EventGraph")

**Response:**
```json
{
  "success": true,
  "message": "Successfully deleted node 'Get Owner'. Disconnected 2 pins before deletion.",
  "disconnections": 2
}
```

**Use Cases:**
- Removing incorrect or obsolete nodes
- Cleaning up failed node creation attempts
- Refactoring Blueprint logic by removing unnecessary nodes
- Fixing Blueprint compilation errors caused by invalid nodes

### Replace Node

Completely replace one node with another while automatically restoring connections. This is the most powerful graph manipulation tool, featuring:

- **Smart Pin Matching**: Tries exact pin name match first, falls back to same-direction matching
- **Automatic Type Casting**: Uses `ConnectNodesWithAutoCast` for primitive type conversions
- **Connection Restoration**: Preserves all compatible connections from the old node
- **Position Preservation**: New node appears at the same position as the old node

**Basic Usage:**
```
"Replace the 'Get Owner' node with 'Get Owning Actor' in DialogueComponent's CanInteract function"

"Replace 'Get TestFloat' variable with 'Get ReplacementFloat' in the EventGraph"

"Swap the 'Add' math node with a 'Multiply' node in CalculateDamage function"
```

**Advanced Usage with Explicit Parameters:**
```
"Replace node 354D34B94684C47CC592A88E87C22772 with Get Owning Actor in DialogueComponent"

"In the CanInteract function, replace the Get Owner node with GetOwningActor"
```

**Parameters:**
- `blueprint_name`: Name of the Blueprint
- `old_node_id`: Unique identifier of the node to replace
- `new_node_type`: Type/name of the new node (e.g., "GetOwningActor", "Add")
- `target_graph`: Graph name (defaults to "EventGraph")
- `new_node_config`: Optional configuration for the new node

**Response:**
```json
{
  "success": true,
  "message": "Successfully replaced node 'Get Owner' with 'Get Owning Actor'. Restored 1 connections.",
  "old_node_id": "354D34B94684C47CC592A88E87C22772",
  "new_node_id": "A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6",
  "restored_connections": 1,
  "stored_connections": [
    {
      "source_node": "354D34B94684C47CC592A88E87C22772",
      "source_pin": "ReturnValue",
      "target_node": "789ABC123DEF456GHI789JKL012MNO345",
      "target_pin": "Target"
    }
  ]
}
```

**Pin Matching Strategy:**
1. **Exact Name Match**: If old and new nodes have pins with identical names, those are connected
2. **Direction Fallback**: If no exact match, connects pins with the same direction (input→input, output→output)
3. **Type Casting**: Automatically handles primitive type conversions (Int→Float, etc.)

**Use Cases:**
- Fixing incorrect function calls (Get Owner → Get Owning Actor for ActorComponents)
- Swapping variable references (Get OldVariable → Get NewVariable)
- Replacing math operations while preserving data flow
- Upgrading deprecated nodes to newer equivalents
- Refactoring Blueprint logic without manual reconnection

**Example Workflow:**
```
"I need to fix the DialogueComponent. Find the Get Owner node in the CanInteract function and replace it with Get Owning Actor"

Result:
1. Finds node "Get Owner" with ID 354D34B94684C47CC592A88E87C22772
2. Stores connection: ReturnValue pin connected to GetDistanceTo's Target pin
3. Deletes old Get Owner node
4. Creates new Get Owning Actor node at same position
5. Restores connection using auto-cast (both return AActor*)
6. Returns success with 1 restored connection
```

**Connection Restoration Details:**
The tool stores all connections before replacement:
```
{
  "pin_name": "ReturnValue",
  "pin_direction": "EGPD_Output", 
  "connected_nodes": ["789ABC..."],
  "connected_pins": ["Target"]
}
```

Then attempts to reconnect each using:
1. Find matching pin on new node (exact name or same direction)
2. Call `ConnectNodesWithAutoCast` for type-safe connection
3. Report success/failure per connection

**Limitations:**
- Cannot restore connections if new node has completely different pin structure
- Complex struct/object connections may fail if types are incompatible
- Event pins and execution flow pins must have matching signatures
- Some connections may require manual intervention after replacement

## Best Practices for Natural Language Commands

### Be Specific with Node Types
Instead of: *"Add a node to my Blueprint"*  
Use: *"Add a BeginPlay event node to the PlayerCharacter Blueprint"*

### Include Position Information
Instead of: *"Add a function call"*  
Use: *"Add a SetActorLocation function call at position [300, 200] targeting self"*

### Specify Connection Details
Instead of: *"Connect the nodes"*  
Use: *"Connect the BeginPlay event execution pin to the SetActorLocation function call"*

### Name Components and Targets Clearly
Instead of: *"Add a function for the component"*  
Use: *"Add a SetLightColor function call targeting the 'MainLight' component"*

### Group Related Operations
*"Add BeginPlay event, SetActorLocation function targeting self, connect them, and position at [100, 200] and [400, 200]"*

## Common Use Cases

### Character Setup
- Adding input action events for movement and actions
- Setting up BeginPlay initialization logic
- Creating component references for character parts
- Building health and status update systems

### Environment Interaction
- Creating trigger overlap events
- Setting up interactive object responses
- Building door, switch, and button logic
- Managing environmental audio and visual effects

### Combat Systems
- Adding weapon firing input events
- Creating damage application logic
- Setting up projectile collision responses
- Building health and death event chains

### UI Integration
- Creating input events for menu actions
- Setting up widget visibility changes
- Building score and status update logic
- Managing pause and game state changes

### AI Behavior
- Adding Tick events for AI updates
- Creating sight and hearing trigger responses
- Building patrol and combat state logic
- Setting up communication between AI actors

## Node Graph Architecture

### Event-Driven Design
Start with event nodes (input actions, overlaps, timers) as entry points for logic chains. These serve as the triggers for all gameplay functionality.

### Component-Based Logic
Use component reference nodes to access and modify specific parts of your actors. This keeps logic organized and maintainable.

### Execution Flow
Connect execution pins (white arrows) to define the order of operations. Use branching and sequence nodes for complex logic flow.

### Data Flow
Connect data pins (colored lines) to pass information between nodes. Use variables and component references as data sources.

## Error Handling and Troubleshooting

If you encounter issues:

1. **Node Not Found**: Use "find nodes in [BlueprintName]" to see existing nodes
2. **Connection Issues**: Verify pin names and types match between nodes
3. **Component References**: Ensure components exist in the Blueprint before creating references
4. **Function Parameters**: Check that function calls have the correct parameter names and types

## Performance Considerations

- Use Tick events sparingly - they run every frame
- Prefer event-driven logic over constant polling
- Cache component references rather than getting them repeatedly
- Use sequence nodes to organize multiple function calls efficiently

Remember that all operations are performed through natural language with your AI assistant, making Blueprint visual scripting accessible and intuitive without requiring detailed knowledge of Unreal Engine's node editor interface. 