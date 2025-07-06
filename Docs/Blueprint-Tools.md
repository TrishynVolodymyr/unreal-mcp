# Blueprint Tools - Unreal MCP

This document provides comprehensive information about using Blueprint tools through the Unreal MCP (Model Context Protocol). These tools allow you to create, modify, and manage Blueprint assets in Unreal Engine using natural language commands through AI assistants.

## Overview

Blueprint tools enable you to:
- Create new Blueprint classes with custom parent classes
- Add and configure components (meshes, lights, cameras, etc.)
- Set up variables and properties
- Configure physics and collision
- Manage Blueprint interfaces
- Add custom events and functionality
- Compile and test Blueprints

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Creating Blueprints

**Creating Basic Blueprints:**
```
"Create a new Blueprint called 'PlayerCharacter' based on the Character class"

"Make a new Actor Blueprint named 'InteractableObject'"

"Create a Pawn Blueprint called 'VehicleBase' in the Vehicles folder"
```

**Creating Blueprints with Path Structure:**
```
"Create a Blueprint called 'System/Managers/GameManager' based on Actor"

"Make a new Blueprint in the Controllers folder called 'InputController' based on PlayerController"
```

### Adding Components

**Adding Basic Components:**
```
"Add a Static Mesh component called 'MainMesh' to my PlayerCharacter Blueprint"

"Add a Point Light component named 'HealthLight' to the InteractableObject Blueprint"

"Add a Camera component called 'MainCamera' to my VehicleBase Blueprint"

"Add a Sphere Collision component named 'TriggerZone' to the InteractableObject"
```

**Adding Components with Transforms:**
```
"Add a Static Mesh component called 'Weapon' to PlayerCharacter at position [50, 0, 20]"

"Add a Spot Light at position [0, 0, 100] with rotation [45, 0, 0] to my scene Blueprint"

"Add a Camera component scaled to [2, 2, 2] at position [0, 0, 180] in my vehicle Blueprint"
```

### Setting Component Properties

**Static Mesh Properties:**
```
"Set the static mesh of the MainMesh component in PlayerCharacter to use the basic cube"

"Change the static mesh of the Weapon component to use '/Game/Models/Sword'"

"Set the mesh for the Platform component to use the engine's basic shapes cylinder"
```

**Light Properties:**
```
"Set the HealthLight intensity to 5000 and color to red in the InteractableObject Blueprint"

"Change the MainLight's attenuation radius to 1000 and enable shadows"

"Set the SpotLight's source radius to 15 and soft source radius to 25"
```

**Component Transform and Properties:**
```
"Move the MainCamera component to position [0, -200, 100] in the VehicleBase Blueprint"

"Set the TriggerZone collision to QueryOnly and radius to 150"

"Scale the Weapon component to [1.5, 1.5, 1.5] and rotate it 45 degrees on the Z axis"
```

### Physics Configuration

**Basic Physics Setup:**
```
"Enable physics simulation on the MainMesh component of InteractableObject"

"Set up physics on the Crate component with mass 50 and disable gravity"

"Enable physics simulation with linear damping 0.1 and angular damping 0.5 on the Sphere component"
```

**Advanced Physics Properties:**
```
"Configure the Boulder component with physics simulation, mass 100, gravity enabled, linear damping 0.05"

"Set up the Vehicle body with physics: mass 1500, no gravity, linear damping 0.2, angular damping 0.8"
```

### Blueprint Variables

**Adding Basic Variables:**
```
"Add an integer variable called 'Health' to the PlayerCharacter Blueprint and expose it to the editor"

"Add a string variable named 'PlayerName' to the character Blueprint"

"Add a boolean variable called 'IsAlive' and make it editable in the editor"

"Add a Vector variable named 'SpawnLocation' to store the player's spawn point"
```

**Adding Array Variables:**
```
"Add a string array variable called 'Inventory' to store item names"

"Add an integer array named 'Scores' to track multiple score values"

"Add a Vector array called 'PatrolPoints' for enemy movement waypoints"
```

**Adding Custom Struct Variables:**
```
"Add a variable called 'PlayerStats' using the PlayerStatsStruct to the character Blueprint"

"Add an ItemData variable using the '/Game/DataStructures/ItemStruct' struct"

"Create a variable named 'GameSettings' using the GameSettingsStruct and expose it"
```

**Adding Blueprint Reference Variables:**
```
"Add a variable to store a reference to the MainMenu widget Blueprint"

"Add a PlayerPawnClass variable that references the BP_PlayerPawn Blueprint"

"Create a variable called 'HealthBarWidget' that references the HealthBar widget Blueprint"
```

### Pawn-Specific Configuration

**Basic Pawn Setup:**
```
"Configure the PlayerCharacter to auto-possess Player 0"

"Set up the VehiclePawn to use controller rotation for yaw but not pitch or roll"

"Configure the EnemyPawn to not use any controller rotation and enable damage"
```

**Advanced Pawn Configuration:**
```
"Set up PlayerCharacter: auto-possess Player 0, use controller yaw rotation, enable damage"

"Configure VehiclePawn: no auto-possess, use controller yaw and pitch, disable roll, no damage"
```

### Blueprint Interfaces

**Creating and Using Interfaces:**
```
"Create a new Blueprint Interface called 'Interactable'"

"Add the Interactable interface to my PlayerCharacter Blueprint"

"Create an interface called 'Damageable' in the Interfaces folder"

"Apply the '/Game/Interfaces/Collectible' interface to the ItemPickup Blueprint"
```

### Custom Events

**Adding Custom Events:**
```
"Add a custom event called 'OnPlayerDeath' to the PlayerCharacter Blueprint"

"Create a custom event named 'TriggerAnimation' in the Door Blueprint"

"Add a custom event called 'SpawnEnemy' to the GameManager Blueprint"
```

### Blueprint Compilation and Management

**Compiling Blueprints:**
```
"Compile the PlayerCharacter Blueprint"

"Compile my VehicleBase Blueprint to check for errors"

"Build and compile the GameManager Blueprint"
```

**Blueprint Information:**
```
"Show me all components in the PlayerCharacter Blueprint"

"List the components in my VehicleBase Blueprint"

"What components does the InteractableObject Blueprint have?"
```

**Setting Blueprint Properties:**
```
"Set the MaxHealth property to 100 on the PlayerCharacter Blueprint class defaults"

"Change the MovementSpeed property to 600 in the VehicleBase Blueprint"

"Set the DefaultWeapon property to '/Game/Weapons/Sword' in the character Blueprint"
```

## Advanced Usage Patterns

### Creating Complete Game Objects

**Creating a Complete Pickup Item:**
```
"Create an Actor Blueprint called 'HealthPotion' with:
- A Static Mesh component using the basic sphere
- A Point Light component with green color and intensity 2000
- A Sphere Collision component for trigger detection
- Physics enabled with mass 0.5
- An integer variable 'HealAmount' set to 25 and exposed to editor"
```

**Creating a Vehicle Blueprint:**
```
"Create a Pawn Blueprint called 'Car' with:
- A Static Mesh component for the car body
- Four Static Mesh components for wheels positioned appropriately
- A Camera component positioned behind the car
- Two Spot Light components for headlights
- Auto-possess Player 0 and use controller yaw rotation"
```

### Creating Interactive Objects

**Creating a Door System:**
```
"Create an Actor Blueprint called 'InteractiveDoor' with:
- A Static Mesh component for the door
- A Box Collision component for interaction trigger
- A boolean variable 'IsOpen' exposed to editor
- A custom event called 'ToggleDoor'
- A reference variable to store the door sound effect"
```

**Creating an Enemy Character:**
```
"Create a Character Blueprint called 'BasicEnemy' with:
- Integer variables for Health and AttackDamage
- A Sphere Collision component for attack range detection
- Auto-possess disabled
- Don't use controller rotation
- Enable damage
- Add the Damageable interface"
```

## Best Practices for Natural Language Commands

### Be Specific with Names
Instead of: *"Add a component to my Blueprint"*  
Use: *"Add a Static Mesh component called 'MainBody' to the VehicleBlueprint"*

### Include Context
Instead of: *"Set the mesh"*  
Use: *"Set the static mesh of the MainBody component in VehicleBlueprint to use the car model"*

### Specify Locations When Needed
Instead of: *"Add a light"*  
Use: *"Add a Point Light component called 'Headlight' at position [100, 50, 20] to the vehicle"*

### Use Clear Variable Types
Instead of: *"Add a variable for health"*  
Use: *"Add an integer variable called 'MaxHealth' and expose it to the editor"*

### Group Related Operations
*"Set up physics on the MainBody component: enable simulation, mass 1200, gravity enabled, linear damping 0.1"*

## Common Use Cases

### Character Setup
- Creating player character Blueprints with movement components
- Setting up health and status variables
- Configuring input handling and camera systems
- Adding weapon attachment points

### Environment Objects
- Creating interactive objects (doors, switches, pickups)
- Setting up trigger volumes and collision detection
- Adding visual effects with lights and particles
- Configuring physics for destructible objects

### Vehicle Systems
- Creating drivable vehicles with proper physics
- Setting up camera systems and controls
- Adding visual elements (lights, effects)
- Configuring possession and input handling

### UI Integration
- Creating Blueprint variables to reference UI widgets
- Setting up data binding between gameplay and UI
- Managing widget visibility and state
- Storing references to different menu systems

## Error Handling and Troubleshooting

If you encounter issues:

1. **Compilation Errors**: Ask to "compile the [BlueprintName] Blueprint" to check for errors
2. **Component Issues**: Use "list components in [BlueprintName]" to verify component structure  
3. **Property Problems**: Be specific about component names and property types
4. **Path Issues**: Use full paths for custom assets: "/Game/Folder/AssetName"

Remember that all operations are performed through natural language with your AI assistant, making Blueprint creation and management intuitive and accessible without requiring deep technical knowledge of Unreal Engine's interface. 