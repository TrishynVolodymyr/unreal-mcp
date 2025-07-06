# Blueprint Action Tools - Unreal MCP

This document provides comprehensive information about using Blueprint Action tools through the Unreal MCP (Model Context Protocol). These tools allow you to dynamically discover available Blueprint actions, explore node possibilities, and create sophisticated Blueprint logic using natural language commands through AI assistants.

## Overview

Blueprint Action tools enable you to:
- Discover available actions for specific pin types and classes
- Search for Blueprint functions across the entire Unreal Engine action database
- Get detailed information about node pins and their requirements
- Explore class hierarchies and their available functionality
- Create Blueprint nodes by discovered action names
- Build complex Blueprint logic based on dynamic action discovery

## Key Concepts

**Action Discovery:** These tools use Unreal's `FBlueprintActionDatabase` to dynamically discover what nodes and functions are available, similar to the context menu that appears when you right-click in the Blueprint editor.

**Pin-Based Search:** Find actions that can connect to specific pin types (object, float, string, etc.) and their subcategories (PlayerController, Vector, etc.).

**Class-Based Search:** Discover all available functions for specific classes and their inheritance hierarchies.

**Dynamic Node Creation:** Create Blueprint nodes using discovered action names rather than hardcoded function lists.

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Discovering Actions for Pin Types

**Basic Pin Type Discovery:**
```
"What actions are available for float pins?"

"Show me all available actions for string pins"

"Find actions that can connect to boolean pins"

"What functions work with integer pins?"
```

**Object Pin Discovery:**
```
"What actions are available for PlayerController object pins?"

"Show me all functions for Pawn object pins"

"Find actions for StaticMeshComponent object pins"

"What can I do with Actor object pins?"
```

**Struct Pin Discovery:**
```
"What actions work with Vector struct pins?"

"Show me functions for Transform struct pins"

"Find actions for Rotator struct pins"

"What can I do with Color struct pins?"
```

**Filtered Pin Discovery:**
```
"Find math-related actions for float pins"

"Show me movement actions for Vector pins"

"Find input-related actions for PlayerController pins"

"What transform actions work with Actor pins?"
```

### Discovering Actions for Classes

**Basic Class Discovery:**
```
"What actions are available for the PlayerController class?"

"Show me all functions for the Pawn class"

"Find available actions for the Actor class"

"What can I do with StaticMeshComponent class?"
```

**Specific Class Paths:**
```
"What actions are available for '/Script/Engine.PlayerController'?"

"Show me functions for '/Script/Engine.Character'"

"Find actions for '/Script/UMG.UserWidget'"
```

**Filtered Class Discovery:**
```
"Find movement-related actions for the Character class"

"Show me input actions for PlayerController"

"Find rendering actions for StaticMeshComponent"

"What audio actions work with AudioComponent?"
```

### Discovering Class Hierarchy Actions

**Complete Hierarchy Discovery:**
```
"Show me all actions for PlayerController and its parent classes"

"Find comprehensive actions for the Character class hierarchy"

"What actions are available for Pawn and all its parent classes?"

"Show me complete functionality for StaticMeshActor hierarchy"
```

**Filtered Hierarchy Discovery:**
```
"Find movement actions across the entire Character class hierarchy"

"Show me input actions for PlayerController and parent classes"

"Find transform actions across the Actor hierarchy"

"What rendering actions exist across StaticMeshComponent hierarchy?"
```

### General Action Search

**Keyword-Based Search:**
```
"Search for actions containing 'add'"

"Find all actions with 'movement' in their name"

"Search for actions related to 'collision'"

"Find actions containing 'widget' or 'UI'"
```

**Category-Filtered Search:**
```
"Search for 'branch' actions in Flow Control category"

"Find 'multiply' actions in Math category"

"Search for 'spawn' actions in Utilities category"

"Find 'print' actions in Development category"
```

**Blueprint-Specific Search:**
```
"Search for 'health' variables in the PlayerCharacter Blueprint"

"Find 'score' related actions in the GameManager Blueprint"

"Search for local variables in the WeaponSystem Blueprint"
```

### Getting Node Pin Information

**Understanding Pin Requirements:**
```
"What does the 'Owning Player' pin expect on Create Widget nodes?"

"What type does the 'Target' pin require on SetActorLocation?"

"What should I connect to the 'Class' pin on Create Widget?"

"What does the 'Object' pin expect on Cast to PlayerController?"
```

**Pin Direction and Properties:**
```
"Is the 'Return Value' pin on Get Controller an input or output?"

"What type does the 'NewLocation' pin accept on SetActorLocation?"

"Is the 'Target' pin required on the Destroy Actor node?"

"What does the 'Component' pin expect on Set Static Mesh?"
```

**Available Pin Discovery:**
```
"What pins are available on the Create Widget node?"

"Show me all pins on the Get Player Controller node"

"What inputs does the Set Actor Transform node have?"

"List all pins on the Play Sound node"
```

### Creating Nodes by Action Name

**Math Function Nodes:**
```
"Create an Add node for float values in the Calculator Blueprint at position [100, 200]"

"Add a Multiply node from KismetMathLibrary to the Physics Blueprint"

"Create a Vector Length node in the Movement Blueprint"

"Add a Clamp node for integer values in the GameLogic Blueprint"
```

**Actor Function Nodes:**
```
"Create a GetActorLocation node in the Movement Blueprint"

"Add a SetActorTransform node to the Teleporter Blueprint"

"Create a GetActorForwardVector node in the AI Blueprint"

"Add a DestroyActor node to the Cleanup Blueprint"
```

**Component Function Nodes:**
```
"Create a SetStaticMesh node for StaticMeshComponent in the Building Blueprint"

"Add a SetLightColor node for LightComponent in the Lighting Blueprint"

"Create a PlaySound node for AudioComponent in the Audio Blueprint"

"Add a SetText node for TextComponent in the UI Blueprint"
```

**Custom Event Nodes:**
```
"Create a custom event called 'OnPlayerDied' in the GameManager Blueprint"

"Add a custom event named 'TriggerAnimation' in the Door Blueprint"

"Create a custom event called 'SpawnEnemy' at position [200, 300]"

"Add a custom event named 'CollectItem' in the Inventory Blueprint"
```

**Widget Creation Nodes:**
```
"Create a 'Create Widget' node for the MainMenu widget in the UI Controller Blueprint"

"Add a 'Create Widget' node for HealthBar widget in the HUD Blueprint"

"Create a 'Create Widget' node for DialogBox at position [400, 100]"
```

**Positioned Node Creation:**
```
"Create a Vector Addition node at position [300, 150] in the Math Blueprint"

"Add a Branch node at coordinates [200, 400] in the Logic Blueprint"

"Create a Print String node at position [500, 200] in the Debug Blueprint"

"Add a Get Player Controller node at coordinates [100, 300]"
```

## Advanced Usage Patterns

### Discovery-Based Blueprint Building

**Step 1: Discover Available Actions**
```
"What actions are available for PlayerController pins?"
```

**Step 2: Get Pin Information**
```
"What does the 'Target' pin expect on GetInputComponent?"
```

**Step 3: Create the Node**
```
"Create a GetInputComponent node targeting PlayerController in my Input Blueprint"
```

### Class Hierarchy Exploration

**Explore Full Functionality:**
```
"Show me comprehensive actions for the Character class hierarchy to understand all available functions"
```

**Filter by Category:**
```
"Find movement actions across the Pawn hierarchy"
```

**Create Discovered Nodes:**
```
"Create a GetMovementComponent node from the discovered Character actions"
```

### Pin-Type Based Development

**Discover Compatible Actions:**
```
"What actions work with Vector pins for movement calculations?"
```

**Understand Requirements:**
```
"What does the 'DeltaLocation' pin expect on AddActorWorldOffset?"
```

**Build the Logic:**
```
"Create an AddActorWorldOffset node and connect a Vector multiplication result to DeltaLocation"
```

### Search-Driven Development

**Find Functions by Keywords:**
```
"Search for all actions containing 'widget' for UI development"
```

**Explore Categories:**
```
"Search for actions in the 'Input' category for player controls"
```

**Create Based on Search:**
```
"Create an 'Add to Viewport' node from the widget search results"
```

## Common Use Cases

### Building UI Systems
1. **Discover Widget Actions:** "What actions are available for UserWidget class?"
2. **Find Creation Functions:** "Search for 'create widget' actions"
3. **Understand Requirements:** "What does the 'Class' pin expect on Create Widget?"
4. **Build the System:** "Create a Create Widget node for MainMenu in the UI Controller"

### Creating Movement Systems
1. **Explore Movement Actions:** "Find movement actions for the Character class hierarchy"
2. **Understand Input Requirements:** "What pins does AddMovementInput have?"
3. **Build Input Handling:** "Create AddMovementInput nodes for forward and right movement"

### Developing Audio Systems
1. **Discover Audio Actions:** "What actions work with AudioComponent?"
2. **Find Playback Functions:** "Search for 'play sound' actions"
3. **Create Audio Nodes:** "Add PlaySound nodes for weapon firing effects"

### Building Physics Systems
1. **Explore Physics Actions:** "Find physics actions for StaticMeshComponent"
2. **Understand Force Functions:** "What pins does AddForce have?"
3. **Create Physics Nodes:** "Add AddForce nodes for explosion effects"

## Tips and Best Practices

### Effective Discovery
- **Start Broad:** Begin with class-based discovery to understand available functionality
- **Narrow Down:** Use pin-based discovery for specific connection requirements
- **Use Hierarchy:** Explore class hierarchies to find inherited functionality
- **Filter Results:** Use search filters to focus on relevant actions

### Understanding Requirements
- **Check Pin Types:** Always verify what types pins expect before connecting
- **Understand Direction:** Know whether pins are inputs or outputs
- **Verify Requirements:** Check if pins are required or optional
- **Use Descriptions:** Read pin descriptions to understand their purpose

### Building Logic
- **Discover First:** Always discover available actions before hardcoding function names
- **Understand Context:** Use class-specific discovery for better results
- **Position Strategically:** Place nodes at logical positions in your graph
- **Test Incrementally:** Create and test nodes step by step

### Performance Considerations
- **Limit Results:** Use max_results parameter to avoid overwhelming responses
- **Use Filters:** Apply search filters to get more relevant results
- **Cache Discoveries:** Remember discovered actions for similar use cases
- **Be Specific:** Use specific class names and pin types for better results

## Integration with Other Tools

### With Node Tools
- **Discovery + Creation:** Use Blueprint Action tools to discover, then Node tools to connect
- **Pin Understanding:** Use pin info to understand connection requirements
- **Variable Integration:** Combine with variable discovery for complete automation

### With Blueprint Tools
- **Component Discovery:** Find actions for Blueprint components
- **Variable Actions:** Discover actions for Blueprint variables
- **Interface Functions:** Find actions for Blueprint interfaces

### With UMG Tools
- **Widget Actions:** Discover actions for UI components and widgets
- **Event Binding:** Find event-related actions for UI interactions
- **Widget Management:** Discover viewport and widget lifecycle actions

## Error Handling and Troubleshooting

### Common Issues
- **Action Not Found:** Try alternative search terms or class names
- **Pin Mismatch:** Verify pin types and requirements before connecting
- **Node Creation Failed:** Check if class names and function names are correct
- **Empty Results:** Broaden search criteria or check class availability

### Debugging Steps
1. **Verify Class Existence:** Ensure the target class exists in your project
2. **Check Search Terms:** Try different keywords or broader searches
3. **Validate Pin Names:** Verify pin names match exactly
4. **Test Simple Cases:** Start with basic actions before complex ones

The Blueprint Action tools provide a powerful way to discover and utilize the full potential of Unreal Engine's Blueprint system through dynamic action database exploration, making it easier to build complex gameplay systems using natural language commands. 