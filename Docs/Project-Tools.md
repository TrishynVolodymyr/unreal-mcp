# Project Tools - Unreal MCP

This document provides comprehensive information about using Project tools through the Unreal MCP (Model Context Protocol). These tools allow you to manage project structure, input systems, structs, and organizational aspects of your Unreal Engine project using natural language commands through AI assistants.

## Overview

Project tools enable you to:
- Create and manage project folders and organization
- Set up Enhanced Input Actions and Mapping Contexts
- Create and manage legacy input mappings
- Create, update, and inspect Unreal structs
- List and browse project content
- Organize assets and development resources

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Enhanced Input System (UE 5.5+)

**Creating Enhanced Input Actions:**
```
"Create a digital Enhanced Input Action called 'Jump' for jumping mechanics"

"Make an analog input action named 'Move' for movement controls"

"Create a 2D axis input action called 'Look' for camera movement"

"Make a digital input action 'Fire' for weapon shooting"

"Create an analog input action 'Throttle' for vehicle acceleration"
```

**Creating Input Actions with Descriptions:**
```
"Create a digital input action 'Interact' with description 'Player interaction with objects'"

"Make an input action 'Pause' with description 'Pause/unpause game functionality'"

"Create a 2D axis action 'MouseLook' with description 'Mouse camera control for first person'"

"Make an analog action 'Zoom' with description 'Camera zoom in/out control'"
```

**Creating Input Mapping Contexts:**
```
"Create an input mapping context called 'Default' for main gameplay controls"

"Make a mapping context named 'Vehicle' for driving controls"

"Create an input mapping context 'Menu' for UI navigation"

"Make a mapping context 'Debug' for development and testing controls"

"Create an input mapping context 'Combat' for fighting mechanics"
```

**Adding Key Mappings to Contexts:**
```
"Map the Space key to the Jump action in the Default context"

"Bind the W key to the MoveForward action in the Default mapping context"

"Map Left Mouse Button to the Fire action in the Combat context"

"Bind the Escape key to the Pause action in the Default context"

"Map the E key to the Interact action in the Default context"
```

**Advanced Key Mapping with Modifiers:**
```
"Map Shift+W to the Sprint action in the Default context"

"Bind Ctrl+S to the QuickSave action with the Ctrl modifier"

"Map Alt+Tab to the SwitchWeapon action in the Combat context"

"Bind Ctrl+Shift+D to the DebugMode action with both modifiers"
```

**Querying Enhanced Input Assets:**
```
"Show me all Enhanced Input Actions in the project"

"List all input actions in the /Game/Input directory"

"What Enhanced Input Actions exist for combat?"

"Show me all Input Mapping Contexts and their key bindings"

"List the input mapping contexts in the Input folder"
```

### Legacy Input System

**Creating Legacy Input Mappings:**
```
"Create a legacy input mapping for 'Jump' bound to the Space key"

"Set up an axis mapping for 'MoveForward' on the W key"

"Create an action mapping for 'Fire' on the Left Mouse Button"

"Make an axis mapping for 'Turn' on the Mouse X axis"

"Set up action mapping for 'Interact' on the E key"
```

### Project Structure and Organization

**Creating Folders:**
```
"Create a content folder called 'Characters' for character assets"

"Make a folder named 'Weapons' in the Content browser"

"Create a project folder called 'Documentation' for project docs"

"Make a content folder structure: 'Art/Textures' for texture assets"

"Create an 'Audio/Music' folder in the content browser"
```

**Advanced Folder Creation:**
```
"Create a complete folder structure for an RPG project:
- Content/Characters for player and NPC assets
- Content/Weapons for weapon blueprints and meshes  
- Content/Environment for level and world assets
- Content/UI for interface and menu systems
- Content/Audio for sound effects and music"

"Set up a development folder structure:
- Intermediate/Builds for build outputs
- Documentation/Design for design documents
- Tools/Scripts for development scripts"
```

**Listing Project Contents:**
```
"Show me what's in the Content/Characters folder"

"List all files in the Content/Blueprints directory"

"What folders exist in the main Content directory?"

"Show me the contents of the /Game/Audio folder"

"List everything in the Content/UI directory"
```

### Struct Management

**Creating Structs:**
```
"Create a struct called 'PlayerStats' with properties:
- Health (Integer): Player's health points
- Mana (Integer): Player's mana points  
- Strength (Integer): Physical power attribute
- Agility (Integer): Speed and dexterity attribute"

"Make a struct named 'ItemData' with:
- Name (String): Item display name
- Price (Float): Item cost in currency
- Weight (Float): Item weight for inventory
- IsStackable (Boolean): Can items stack together"

"Create a 'WeaponStats' struct with:
- Damage (Integer): Base weapon damage
- AttackSpeed (Float): Attacks per second
- Range (Float): Weapon effective range
- Durability (Integer): Item condition points"
```

**Creating Complex Structs:**
```
"Create a 'QuestData' struct with:
- Title (String): Quest display name
- Description (String): Quest objective description
- Objectives (String Array): List of quest objectives
- RewardXP (Integer): Experience points reward
- RewardGold (Integer): Currency reward
- IsCompleted (Boolean): Quest completion status"

"Make a 'LevelConfiguration' struct with:
- LevelName (String): Display name for level
- Difficulty (Integer): Difficulty rating 1-10
- EnemySpawns (Vector Array): Enemy spawn locations
- PlayerStart (Vector): Player starting position
- TimeLimit (Float): Level time limit in seconds"
```

**Updating Existing Structs:**
```
"Update the PlayerStats struct to add a new property:
- Intelligence (Integer): Mental attribute for magic"

"Modify the ItemData struct to include:
- Rarity (String): Item rarity level
- Category (String): Item type category"

"Update WeaponStats struct to add:
- CriticalChance (Float): Critical hit probability
- ElementalType (String): Weapon elemental damage type"
```

**Inspecting Structs:**
```
"Show me the properties of the PlayerStats struct"

"What variables are in the ItemData struct?"

"List all properties and types in the WeaponStats struct"

"Display the structure of the QuestData struct"

"What's the current definition of the LevelConfiguration struct?"
```

## Advanced Usage Patterns

### Complete Input System Setup

**Full Enhanced Input Configuration:**
```
"Set up a complete Enhanced Input system for a third-person game:
1. Create input actions:
   - 'Move' (Axis2D) for movement
   - 'Look' (Axis2D) for camera control
   - 'Jump' (Digital) for jumping
   - 'Sprint' (Digital) for running
   - 'Interact' (Digital) for object interaction
2. Create mapping context 'ThirdPersonDefault'
3. Map keys:
   - WASD to Move action
   - Mouse movement to Look action
   - Space to Jump action
   - Shift to Sprint action
   - E to Interact action"
```

**Combat Input System:**
```
"Create combat input system:
1. Create input actions:
   - 'Attack' (Digital) for basic attacks
   - 'Block' (Digital) for defensive stance
   - 'Dodge' (Digital) for evasive movement
   - 'CastSpell' (Digital) for magic abilities
2. Create 'Combat' mapping context
3. Map keys:
   - Left Mouse Button to Attack
   - Right Mouse Button to Block
   - Space to Dodge  
   - Q to CastSpell"
```

### Vehicle Control System

**Vehicle Input Setup:**
```
"Set up vehicle control system:
1. Create input actions:
   - 'Accelerate' (Analog) for forward/reverse
   - 'Steer' (Analog) for left/right steering
   - 'Brake' (Digital) for emergency brake
   - 'Handbrake' (Digital) for parking brake
2. Create 'Vehicle' mapping context  
3. Map keys:
   - W/S to Accelerate (forward/reverse)
   - A/D to Steer (left/right)
   - Space to Brake
   - Shift to Handbrake"
```

### Data Structure Systems

**RPG Data Architecture:**
```
"Create RPG data structure system:
1. Create 'CharacterStats' struct:
   - Health, Mana, Stamina (Integers)
   - Strength, Agility, Intelligence, Wisdom (Integers)
   - Level, Experience (Integers)
2. Create 'Equipment' struct:
   - Weapon, Armor, Accessories (String references)
   - Durability values (Float array)
3. Create 'Inventory' struct:
   - Items (String array)
   - Quantities (Integer array)
   - MaxSlots (Integer)"
```

**Game Configuration System:**
```
"Set up game configuration structures:
1. Create 'GraphicsSettings' struct:
   - Resolution (String)
   - Quality (String: Low/Medium/High/Ultra)
   - VSync (Boolean)
   - FullScreen (Boolean)
2. Create 'AudioSettings' struct:
   - MasterVolume, MusicVolume, SFXVolume (Float 0-1)
   - Quality (String)
3. Create 'GameplaySettings' struct:
   - Difficulty (String)
   - AutoSave (Boolean)
   - TutorialEnabled (Boolean)"
```

### Project Organization

**Asset Organization System:**
```
"Create comprehensive asset organization:
1. Create folder structure:
   - Content/Art/Characters/Player
   - Content/Art/Characters/NPCs
   - Content/Art/Environment/Props
   - Content/Art/Environment/Landscapes
   - Content/Audio/Music/Combat
   - Content/Audio/Music/Ambient
   - Content/Audio/SFX/Weapons
   - Content/Audio/SFX/UI
   - Content/Blueprints/Characters
   - Content/Blueprints/Gameplay
   - Content/UI/Menus
   - Content/UI/HUD"
```

**Development Organization:**
```
"Set up development project structure:
1. Create project folders:
   - Documentation/Design for game design docs
   - Documentation/Technical for technical specs
   - Tools/Scripts for automation scripts
   - Builds/Debug for debug builds
   - Builds/Release for release builds
2. Create content folders:
   - Content/Dev/Testing for test assets
   - Content/Dev/Prototypes for prototype content"
```

## Best Practices for Natural Language Commands

### Be Specific with Input Actions
Instead of: *"Create an input action"*  
Use: *"Create a digital Enhanced Input Action called 'Jump' for player jumping"*

### Include Full Folder Paths
Instead of: *"Create a folder for characters"*  
Use: *"Create a content folder called 'Characters' in the main Content directory"*

### Specify Struct Properties Clearly
Instead of: *"Make a struct for player data"*  
Use: *"Create a PlayerStats struct with Health (Integer), Mana (Integer), and Level (Integer) properties"*

### Group Related Operations
*"Create Enhanced Input system: make Jump action, create Default context, map Space key to Jump"*

### Use Descriptive Names and Descriptions
*"Create input action 'Interact' with description 'Player interaction with world objects and NPCs'"*

## Common Use Cases

### Game Input Systems
- Setting up player movement and camera controls
- Creating combat and interaction input schemes
- Building vehicle and special mode controls
- Managing UI navigation and menu inputs

### Project Organization
- Creating asset category folders for art and audio
- Setting up Blueprint organization systems
- Managing development and testing content
- Organizing documentation and tools

### Data Structures
- Creating player progression and statistics systems
- Building inventory and equipment data structures
- Setting up configuration and settings structs
- Managing level and world data definitions

### Development Workflow
- Creating testing and debug input schemes
- Setting up development asset organization
- Managing build and deployment folders
- Creating tool and script organization

## Input System Architecture

### Enhanced Input vs Legacy
Enhanced Input (UE 5.5+) provides more flexible and powerful input handling compared to legacy input mappings. Use Enhanced Input for new projects.

### Context-Based Design
Organize input actions into logical contexts (Default, Combat, Vehicle, Menu) that can be enabled/disabled as needed.

### Modifier Support
Enhanced Input supports complex key combinations with Shift, Ctrl, Alt, and Cmd modifiers for advanced control schemes.

### Cross-Platform Compatibility
Design input actions that can work across different input devices (keyboard/mouse, gamepad, touch).

## Struct Design Principles

### Single Responsibility
Design structs with a clear, focused purpose rather than creating overly complex multi-purpose structures.

### Type Consistency
Use appropriate data types (Integer for whole numbers, Float for decimals, Boolean for true/false) for optimal performance.

### Future-Proofing
Consider potential future additions when designing structs, leaving room for expansion without requiring complete redesigns.

### Naming Conventions
Use clear, descriptive names for both structs and their properties to improve code readability and maintenance.

## Error Handling and Troubleshooting

If you encounter issues:

1. **Input Action Issues**: Ensure action names are unique and use proper value types
2. **Folder Creation Problems**: Check that parent folders exist and paths are valid
3. **Struct Update Issues**: Verify struct exists before attempting updates
4. **Path Resolution**: Use full paths when referencing assets or folders

## Performance Considerations

- Use appropriate data types in structs to minimize memory usage
- Organize input contexts efficiently to avoid unnecessary processing
- Keep folder structures logical but not overly deep
- Consider asset loading implications when organizing content

Remember that all operations are performed through natural language with your AI assistant, making project management and organization intuitive and accessible without requiring detailed knowledge of Unreal Engine's project management interface. 