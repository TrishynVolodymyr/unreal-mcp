# DataTable Tools - Unreal MCP

This document provides comprehensive information about using DataTable tools through the Unreal MCP (Model Context Protocol). These tools allow you to create, manage, and manipulate DataTable assets for storing structured game data using natural language commands through AI assistants.

## Overview

DataTable tools enable you to:
- Create new DataTable assets with custom struct definitions
- Add, update, and delete rows of data
- Query specific rows or entire tables
- Get table structure information (row names and field names)
- Perform bulk operations on multiple rows
- Manage structured game data like items, characters, levels, and configurations

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Creating DataTables

**Creating Basic DataTables:**
```
"Create a DataTable called 'ItemTable' using the ItemStruct for rows"

"Make a new DataTable named 'CharacterStats' based on the PlayerStatsStruct"

"Create a DataTable called 'LevelData' using the '/Game/Data/LevelStruct' struct"

"Make a DataTable named 'WeaponTable' with the WeaponStruct in the Data folder"

"Create a DataTable called 'DialogueTable' using the ConversationStruct"
```

**Creating DataTables with Descriptions:**
```
"Create a DataTable called 'ItemTable' using ItemStruct with description 'Contains all game items and their properties'"

"Make a DataTable 'EnemyStats' using EnemyStruct with description 'Enemy character statistics and abilities'"

"Create 'QuestData' DataTable using QuestStruct with description 'Quest definitions and objectives'"
```

**Creating DataTables with Full Paths:**
```
"Create a DataTable called 'ItemTable' using '/Game/DataStructures/ItemStruct' in the '/Game/Data' folder"

"Make a DataTable 'NPCDialogue' using '/Game/Structs/DialogueStruct' struct"

"Create a DataTable 'GameSettings' using the '/Game/Data/ConfigStruct' in the Config folder"
```

### Querying DataTable Structure

**Getting Table Information:**
```
"What are the row names in the ItemTable?"

"Show me all the field names in the CharacterStats DataTable"

"List the structure of the WeaponTable DataTable"

"What rows exist in the LevelData table?"

"Get the field names and row names for the QuestData table"
```

### Reading DataTable Data

**Getting All Rows:**
```
"Show me all rows in the ItemTable"

"Get all data from the CharacterStats DataTable"

"Display all entries in the WeaponTable"

"Show me everything in the LevelData table"

"Get all rows from the DialogueTable"
```

**Getting Specific Rows:**
```
"Get the 'Sword' row from the ItemTable"

"Show me the 'PlayerCharacter' data from CharacterStats"

"Get rows 'Level1' and 'Level2' from the LevelData table"

"Show me the 'Fireball' and 'Lightning' rows from the SpellTable"

"Get the 'MainQuest_01' row from the QuestData table"
```

### Adding Data to DataTables

**Adding Single Rows:**
```
"Add a new row called 'HealthPotion' to the ItemTable with:
- Name: 'Health Potion'
- Price: 25.0
- Quantity: 1
- IsConsumable: true
- Tags: ['healing', 'consumable']"

"Add a row 'Warrior' to CharacterStats with:
- Health: 100
- Mana: 50
- Strength: 15
- Agility: 10
- Intelligence: 8"
```

**Adding Multiple Rows:**
```
"Add multiple items to ItemTable:
- 'Sword': Name='Iron Sword', Price=100.0, Damage=25, IsWeapon=true
- 'Shield': Name='Wooden Shield', Price=50.0, Defense=15, IsArmor=true
- 'Bow': Name='Hunter Bow', Price=75.0, Damage=20, IsWeapon=true"

"Add several characters to CharacterStats:
- 'Mage': Health=80, Mana=120, Intelligence=18, Strength=6
- 'Archer': Health=90, Mana=60, Agility=16, Strength=10
- 'Tank': Health=150, Mana=30, Strength=18, Defense=20"
```

### Updating DataTable Data

**Updating Single Rows:**
```
"Update the 'Sword' row in ItemTable to set Price=120.0 and Damage=30"

"Change the 'PlayerCharacter' in CharacterStats to have Health=150 and Mana=100"

"Update 'Level1' in LevelData to set Difficulty='Easy' and EnemyCount=5"

"Modify the 'HealthPotion' row to set Price=30.0 and Description='Restores 50 HP'"
```

**Updating Multiple Rows:**
```
"Update several items in ItemTable:
- 'Sword': increase Price to 150.0
- 'Shield': set Defense to 20
- 'Bow': change Damage to 25"

"Update character stats:
- 'Warrior': boost Health to 120
- 'Mage': increase Mana to 150
- 'Archer': set Agility to 18"
```

### Deleting DataTable Data

**Deleting Single Rows:**
```
"Delete the 'OldSword' row from ItemTable"

"Remove the 'TestCharacter' from CharacterStats"

"Delete the 'DebugLevel' row from LevelData"

"Remove the 'UnusedItem' from the ItemTable"
```

**Deleting Multiple Rows:**
```
"Delete rows 'TempItem1', 'TempItem2', and 'TestItem' from ItemTable"

"Remove 'OldMage', 'TestWarrior', and 'DebugCharacter' from CharacterStats"

"Delete all rows starting with 'Test' from the WeaponTable"

"Remove multiple outdated entries: 'Old_Sword', 'Old_Shield', 'Old_Bow' from ItemTable"
```

## Advanced Usage Patterns

### Game Item Management

**Creating Complete Item Database:**
```
"Set up a complete item system:
1. Create ItemTable using ItemStruct with description 'Game item definitions'
2. Add weapon categories:
   - 'Sword': Name='Iron Sword', Type='Weapon', Damage=25, Price=100
   - 'Bow': Name='Hunter Bow', Type='Weapon', Damage=20, Price=75
   - 'Staff': Name='Magic Staff', Type='Weapon', Damage=30, Price=150
3. Add armor pieces:
   - 'Helmet': Name='Iron Helmet', Type='Armor', Defense=10, Price=50
   - 'Chestplate': Name='Iron Chestplate', Type='Armor', Defense=25, Price=120
4. Add consumables:
   - 'HealthPotion': Name='Health Potion', Type='Consumable', HealAmount=50, Price=25
   - 'ManaPotion': Name='Mana Potion', Type='Consumable', ManaAmount=30, Price=20"
```

### Character System Setup

**Building Character Database:**
```
"Create comprehensive character system:
1. Create CharacterStats table using PlayerStatsStruct
2. Add player classes:
   - 'Warrior': Health=120, Mana=50, Strength=18, Agility=10, Intelligence=8
   - 'Mage': Health=80, Mana=150, Strength=6, Agility=8, Intelligence=20
   - 'Archer': Health=100, Mana=80, Strength=12, Agility=18, Intelligence=12
   - 'Paladin': Health=110, Mana=90, Strength=15, Agility=8, Intelligence=14
3. Add enemy types:
   - 'Goblin': Health=40, Damage=15, Speed=12, Aggression=8
   - 'Orc': Health=80, Damage=25, Speed=8, Aggression=15
   - 'Dragon': Health=500, Damage=100, Speed=6, Aggression=20"
```

### Level and World Data

**Level Configuration System:**
```
"Set up level data management:
1. Create LevelData table using LevelStruct
2. Add tutorial levels:
   - 'Tutorial_Movement': Difficulty=1, EnemyCount=0, Objectives=['Learn to move']
   - 'Tutorial_Combat': Difficulty=2, EnemyCount=3, Objectives=['Defeat enemies']
3. Add main levels:
   - 'Forest_01': Difficulty=5, EnemyCount=8, Environment='Forest', Boss='Orc Chief'
   - 'Cave_01': Difficulty=7, EnemyCount=12, Environment='Cave', Boss='Cave Troll'
   - 'Castle_01': Difficulty=10, EnemyCount=20, Environment='Castle', Boss='Dark Knight'"
```

### Configuration and Settings

**Game Configuration Tables:**
```
"Create game settings system:
1. Create GameConfig table using ConfigStruct
2. Add difficulty settings:
   - 'Easy': PlayerDamageMultiplier=1.5, EnemyDamageMultiplier=0.7, XPMultiplier=1.2
   - 'Normal': PlayerDamageMultiplier=1.0, EnemyDamageMultiplier=1.0, XPMultiplier=1.0
   - 'Hard': PlayerDamageMultiplier=0.8, EnemyDamageMultiplier=1.3, XPMultiplier=0.9
3. Add economy settings:
   - 'ShopPrices': BuyMultiplier=1.0, SellMultiplier=0.6, TaxRate=0.05
   - 'Rewards': QuestXP=100, KillXP=25, DiscoveryXP=50"
```

## Working with Struct Field Names

### Understanding GUID-Based Fields

When working with DataTables, Unreal Engine uses GUID-based internal field names. You need to get the correct field names first:

```
"Get the field names for the ItemTable to see the internal property names"

"Show me the structure of CharacterStats table to understand the field mappings"

"What are the exact field names I need to use for the WeaponTable?"
```

### Using Field Names in Operations

Once you have the field names, use them exactly in your data operations:

```
"First, get field names for ItemTable, then add a row 'MagicSword' using those exact field names"

"Get the structure of CharacterStats, then update the 'Warrior' row using the correct field names"

"Show me the field names for QuestData, then add multiple quest rows using proper field mapping"
```

## Best Practices for Natural Language Commands

### Be Specific with DataTable Names
Instead of: *"Add data to the table"*  
Use: *"Add a new row 'Sword' to the ItemTable"*

### Include All Required Data
Instead of: *"Add a character"*  
Use: *"Add 'Warrior' to CharacterStats with Health=100, Mana=50, Strength=15, Agility=10"*

### Use Clear Row Identifiers
Instead of: *"Update the item"*  
Use: *"Update the 'HealthPotion' row in ItemTable to set Price=30.0"*

### Specify Full Paths When Needed
Instead of: *"Create a table with the struct"*  
Use: *"Create ItemTable using '/Game/DataStructures/ItemStruct' in the '/Game/Data' folder"*

### Group Related Operations
*"Create ItemTable, add weapon rows (Sword, Bow, Staff), then add armor rows (Helmet, Chestplate, Boots)"*

## Common Use Cases

### Game Economy
- Item definitions with prices, stats, and properties
- Shop inventories and merchant data
- Economic balance configurations
- Loot table definitions

### Character Systems
- Player class statistics and abilities
- Enemy AI parameters and behaviors
- NPC dialogue and interaction data
- Progression and leveling tables

### Level Design
- Level configuration and difficulty settings
- Spawn point and objective definitions
- Environmental parameters and settings
- Quest and mission data

### Game Configuration
- Difficulty and balance settings
- Audio and visual configuration
- Input mapping and control schemes
- Localization and text data

### Content Management
- Asset references and metadata
- Version control and content tracking
- Performance and optimization settings
- Debug and development tools

## Data Architecture

### Struct-Based Design
Design your structs carefully before creating DataTables, as changing struct definitions later requires recreating the tables.

### Consistent Naming
Use consistent naming conventions for rows and fields to make data management easier and more predictable.

### Data Validation
Consider including validation data in your structs (min/max values, required fields) to ensure data integrity.

### Hierarchical Organization
Organize related data into separate tables rather than creating overly complex single tables.

## Error Handling and Troubleshooting

If you encounter issues:

1. **Struct Not Found**: Ensure the struct exists and use full paths when necessary
2. **Field Name Issues**: Always get the exact field names using "get field names" before data operations
3. **Row Not Found**: Use "get row names" to verify existing rows before updates or deletions
4. **Data Type Mismatches**: Ensure your data values match the expected types in the struct definition

## Performance Considerations

- Use appropriate data types in your structs to minimize memory usage
- Consider splitting large tables into smaller, more focused tables
- Avoid unnecessary nested structures that complicate data access
- Use bulk operations for multiple row changes to improve efficiency

Remember that all operations are performed through natural language with your AI assistant, making DataTable management intuitive and accessible without requiring detailed knowledge of Unreal Engine's DataTable editor interface. 