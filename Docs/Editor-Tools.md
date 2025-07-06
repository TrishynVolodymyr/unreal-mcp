# Editor Tools - Unreal MCP

This document provides comprehensive information about using Editor tools through the Unreal MCP (Model Context Protocol). These tools allow you to control the Unreal Engine editor, manage actors in the level, and manipulate scene objects using natural language commands through AI assistants.

## Overview

Editor tools enable you to:
- Create and delete actors in the current level
- Find and query actors by name patterns
- Transform actors (position, rotation, scale)
- Get and set actor properties
- Control lights and their properties
- Spawn Blueprint actors with custom logic
- Manage the editor viewport and focus

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Actor Creation and Management

**Creating Basic Actors:**
```
"Create a point light called 'MainLight' in the scene"

"Spawn a static mesh actor named 'Platform' at position [0, 0, 100]"

"Add a camera actor called 'SecurityCamera' at [500, 200, 300] with rotation [0, 45, 0]"

"Create a directional light named 'SunLight' for scene lighting"

"Spawn a spot light called 'Flashlight' pointing downward"
```

**Creating Actors with Specific Transforms:**
```
"Create a point light at position [100, 200, 50] called 'RoomLight'"

"Spawn a static mesh actor at [0, 500, 0] rotated 45 degrees on the Z axis, call it 'RotatedCube'"

"Add a camera at position [0, -1000, 500] looking toward the origin"

"Create a spot light at [200, 0, 400] with 30-degree downward rotation"
```

### Finding and Querying Actors

**Finding Actors by Name:**
```
"Find all actors with 'Light' in their name"

"Look for any actors that start with 'Player'"

"Find all point light actors in the scene"

"Show me actors containing 'Camera' in their names"

"Search for actors that end with 'Trigger'"
```

**Getting Actor Information:**
```
"Show me all actors in the current level"

"List all actors and their positions"

"Get the properties of the actor named 'MainLight'"

"What's the current transform of the 'Platform' actor?"

"Show me the details of all actors in the scene"
```

### Actor Transformation

**Moving Actors:**
```
"Move the 'Platform' actor to position [100, 200, 0]"

"Set the 'MainLight' position to [0, 0, 500]"

"Move the 'SecurityCamera' to coordinates [300, -200, 250]"

"Place the 'SunLight' at the origin [0, 0, 0]"
```

**Rotating Actors:**
```
"Rotate the 'Platform' actor 45 degrees around the Z axis"

"Set the 'SecurityCamera' rotation to [0, 90, 0]"

"Rotate the 'Flashlight' to point downward"

"Set the 'SunLight' rotation to [-45, 30, 0] for realistic lighting"
```

**Scaling Actors:**
```
"Scale the 'Platform' actor to [2, 2, 1] to make it wider"

"Set the 'Cube' scale to [0.5, 0.5, 0.5] to make it smaller"

"Scale the 'Wall' actor uniformly to 150% size"

"Set the 'Trigger' scale to [3, 3, 1] for a larger trigger area"
```

**Combined Transform Operations:**
```
"Move 'Platform' to [100, 0, 50], rotate it 30 degrees, and scale it to [2, 1, 1]"

"Transform 'SecurityCamera': position [400, -300, 200], rotation [0, 45, 0], scale [1, 1, 1]"

"Set 'MainLight' to position [0, 0, 400] and rotate it 45 degrees on Y axis"
```

### Actor Properties

**Setting Basic Properties:**
```
"Hide the 'TestCube' actor"

"Make the 'Platform' actor visible again"

"Set the 'Trigger' actor mobility to Movable"

"Change the 'Wall' actor mobility to Static for better performance"

"Enable collision on the 'Platform' actor"
```

**Setting Actor Properties with Values:**
```
"Set the 'DebugSphere' actor's bHidden property to true"

"Change the 'MovingPlatform' Mobility property to 'Movable'"

"Set the 'CollisionBox' actor's collision enabled property to 'QueryOnly'"

"Change the 'StaticWall' actor's mobility to 'Static'"
```

### Light Management

**Basic Light Properties:**
```
"Set the 'MainLight' intensity to 5000"

"Change the 'RoomLight' color to red"

"Set the 'SecurityLight' intensity to 2000 and color to blue"

"Make the 'Flashlight' color warm white and intensity 8000"
```

**Advanced Light Configuration:**
```
"Configure 'MainLight': intensity 10000, white color, attenuation radius 2000"

"Set up 'RoomLight' with intensity 3000, green color, and enable shadows"

"Configure 'SecurityLight': intensity 1500, blue color, attenuation 500, source radius 20"

"Set 'Flashlight' properties: intensity 12000, yellow color, disable shadows, source radius 5"
```

**Specific Light Properties:**
```
"Set the 'MainLight' attenuation radius to 1500"

"Change the 'RoomLight' source radius to 10 for softer lighting"

"Enable shadow casting on the 'SunLight'"

"Set the 'Flashlight' soft source radius to 15"

"Disable shadows on the 'RoomLight' for better performance"
```

### Blueprint Actor Management

**Spawning Blueprint Actors:**
```
"Spawn a PlayerCharacter Blueprint actor at the origin"

"Create an instance of the 'BP_InteractableObject' Blueprint at position [200, 0, 0]"

"Spawn the '/Game/Characters/BP_Enemy' Blueprint at [500, 300, 0] with rotation [0, 180, 0]"

"Add an instance of the 'BP_Vehicle' Blueprint at coordinates [0, 1000, 100]"
```

**Spawning with Custom Names:**
```
"Spawn the PlayerCharacter Blueprint and name the instance 'MainPlayer'"

"Create a 'BP_HealthPotion' instance called 'HealthPickup1' at [100, 100, 0]"

"Spawn the enemy Blueprint as 'Patrol_Enemy_01' at the guard post location"

"Add a vehicle Blueprint instance named 'PlayerCar' in the garage area"
```

### Scene Management

**Deleting Actors:**
```
"Delete the actor named 'TestCube'"

"Remove the 'OldLight' actor from the scene"

"Delete all actors with 'Temp' in their name"

"Remove the 'DebugSphere' actor"
```

**Level Overview:**
```
"Show me a list of all actors in the current level"

"Count how many light actors are in the scene"

"List all the Blueprint actors currently spawned"

"Show me all static mesh actors and their locations"
```

## Advanced Usage Patterns

### Scene Setup and Lighting

**Creating a Basic Scene:**
```
"Set up a basic scene with:
- A directional light called 'SunLight' at rotation [-45, 30, 0] for sunlight
- A point light called 'FillLight' at [200, 200, 300] with soft intensity
- A static mesh platform at the origin scaled to [4, 4, 1]
- A camera at [-500, -500, 300] looking toward the origin"
```

**Setting Up Dramatic Lighting:**
```
"Create dramatic lighting setup:
- Main directional light 'KeyLight' with high intensity and cool color
- Point light 'RimLight' behind objects for edge lighting
- Spot light 'FillLight' with warm color and low intensity for fill"
```

### Level Layout

**Creating a Room Layout:**
```
"Create a simple room layout:
- Four wall actors positioned to form a square room
- A floor platform scaled appropriately
- Ceiling light in the center
- Camera positioned for overview"
```

**Setting Up a Test Environment:**
```
"Set up testing environment:
- Several static mesh actors at different heights for collision testing
- Point lights for visibility
- Camera actors at key viewpoints
- Target objects for interaction testing"
```

### Interactive Scene Elements

**Creating Interaction Points:**
```
"Set up interaction testing area:
- Spawn several 'BP_InteractableObject' instances at different locations
- Add point lights above each for visibility
- Position camera for player perspective
- Add trigger zones around interaction points"
```

**Vehicle Testing Setup:**
```
"Create vehicle testing track:
- Spawn road platform actors in a track layout
- Add 'BP_Vehicle' instance at starting position
- Place checkpoint markers along the track
- Set up cameras for different viewpoints"
```

## Best Practices for Natural Language Commands

### Be Specific with Names and Locations
Instead of: *"Create a light"*  
Use: *"Create a point light called 'RoomLight' at position [0, 0, 300]"*

### Include All Transform Data When Needed
Instead of: *"Move the actor"*  
Use: *"Move the 'Platform' actor to position [100, 200, 50] and rotate it 45 degrees on Z axis"*

### Use Descriptive Actor Names
Instead of: *"Create an actor"*  
Use: *"Create a static mesh actor called 'MainPlatform' for the central platform"*

### Specify Property Values Clearly
Instead of: *"Make the light brighter"*  
Use: *"Set the 'MainLight' intensity to 8000 and attenuation radius to 1500"*

### Group Related Operations
*"Set up 'SecurityLight': position [300, 200, 400], intensity 3000, blue color, enable shadows"*

## Common Use Cases

### Level Design
- Creating basic geometry and platforms
- Setting up lighting schemes for different moods
- Positioning cameras for cinematic shots
- Creating test environments for gameplay

### Gameplay Testing
- Spawning player and enemy Blueprint actors
- Setting up interaction test scenarios
- Creating obstacle courses and challenges
- Testing lighting and visibility conditions

### Scene Composition
- Arranging objects for visual appeal
- Setting up dramatic lighting
- Positioning cameras for screenshots
- Creating environment storytelling elements

### Performance Testing
- Spawning multiple actors to test performance
- Setting up complex lighting scenarios
- Testing Blueprint actor spawning and deletion
- Creating stress test environments

## Error Handling and Troubleshooting

If you encounter issues:

1. **Actor Not Found**: Use "find actors with [pattern]" to locate existing actors
2. **Transform Issues**: Check actor names are spelled correctly and exist in scene
3. **Property Problems**: Use "get properties of [actor name]" to see available properties
4. **Blueprint Spawning**: Ensure Blueprint paths are correct (use full paths like "/Game/Blueprints/BP_Actor")

## Performance Considerations

- Use static mobility for actors that won't move to improve performance
- Limit the number of dynamic lights in a scene
- Consider light attenuation radius to avoid unnecessary calculations
- Use appropriate Blueprint actor spawning for complex objects

Remember that all operations are performed through natural language with your AI assistant, making scene creation and actor management intuitive and accessible without requiring detailed knowledge of Unreal Engine's editor interface. 