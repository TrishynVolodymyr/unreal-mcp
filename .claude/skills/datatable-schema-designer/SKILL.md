---
name: datatable-schema-designer
description: >
  Design-driven DataTable schema planning for Unreal Engine using datatableMCP tools.
  Enforces mandatory schema documentation before any DataTable creation. Requires data
  model planning, naming conventions, and relationship design. ALWAYS trigger on:
  datatable, data table, create table, schema, struct for table, row struct, DT_,
  item database, stats table, loot table, dialogue data, quest data, ability data,
  or any request involving game data storage structures.
---

# DataTable Schema Designer

## Core Principle

**No DataTable without schema design.** Every table must trace back to documented data model decisions.

## Mandatory Pre-Flight

### Step 1: Gather Requirements

Before using datatableMCP tools, ask:

> "Before creating DataTables, I need to understand:
> 1. What game system will use this data? (inventory, combat, dialogue, etc.)
> 2. What entities exist? (items, abilities, NPCs, quests)
> 3. How do they relate? (items have rarities, abilities deal damage types)
> 4. How will you query rows? (by ID, filtered by type, all of category)"

### Step 2: Plan Schema

Document before implementing:

| Element | Decision |
|---------|----------|
| Tables | List all tables needed |
| Primary Keys | Row name format for each |
| Foreign Keys | Which tables reference which |
| Lookup Tables | Shared enums as tables (rarities, categories) |
| Load Order | Reference tables first, then dependents |

### Step 3: Confirm Before Creating

State the plan:
> "Schema for [domain]:
> - Lookup tables: DT_Rarities, DT_Categories (create first)
> - Entity tables: DT_Items (references lookups)
> - Creating DT_Rarities first."

## Naming Conventions

### Assets

| Type | Prefix | Example |
|------|--------|---------|
| DataTable | `DT_` | `DT_Items` |
| Row Struct | `F[Domain]Data` | `FItemData` |
| Enum | `E[Domain]` | `EItemRarity` |

### Row Names (Primary Keys)

Format: `[Domain]_[Category]_[Name]`

| Domain | Example Row Names |
|--------|-------------------|
| Items | `Item_Weapon_IronSword`, `Item_Potion_Health` |
| Abilities | `Ability_Fire_Fireball`, `Ability_Ice_Freeze` |
| Quests | `Quest_Act1_FindSword`, `Quest_Side_HelpFarmer` |
| Dialogue | `Dialogue_Blacksmith_Greeting`, `Dialogue_Blacksmith_Quest01` |

### Fields

| Type | Convention | Examples |
|------|------------|----------|
| Foreign Key | `[Table]ID` | `RarityID`, `CategoryID` |
| Boolean | `bIs`, `bCan`, `bHas` | `bIsStackable`, `bCanSell` |
| Numeric | Unit suffix if ambiguous | `CooldownSeconds`, `DamageAmount` |
| Arrays | Plural | `StatusEffects`, `RequiredItems` |
| Asset Refs | `[Asset]Path` | `IconPath`, `MeshPath` |

## Relationship Patterns

### Lookup Tables (Create First)

Small tables with fixed values that other tables reference:
- Rarities, Categories, DamageTypes, QuestStates
- Use FName foreign keys to reference

### Entity Tables

Main data tables referencing lookups:
- Items → Rarities, Categories
- Abilities → DamageTypes, AbilityTypes

### Junction Tables (Many-to-Many)

When entities relate many-to-many:
- Recipe has many Ingredients, Item used in many Recipes
- Create `DT_RecipeIngredients` with `RecipeID` + `ItemID`

### Self-Referencing

For hierarchies (skill trees, dialogue branches):
- `ParentNodeID` field pointing to same table
- Handle null for root nodes

## Creation Order

Always create in dependency order:

1. **Enums** (if using UEnum instead of lookup tables)
2. **Lookup tables** (Rarities, Categories, Types)
3. **Entity tables** (Items, Abilities, NPCs)
4. **Junction tables** (RecipeIngredients, QuestRewards)

## Validation Checklist

Before finalizing:

- [ ] Row name format consistent within table
- [ ] Foreign keys match actual row names in referenced tables
- [ ] Lookup tables created before dependents
- [ ] No circular dependencies
- [ ] Soft references for optional/large assets
- [ ] Booleans prefixed with `b`

## Anti-Patterns

| Don't | Do Instead |
|-------|------------|
| Create tables without planning relationships | Map out entity relationships first |
| Use strings for bounded value sets | Create lookup table or enum |
| Hard-code repeated values | Extract to lookup table |
| Inconsistent row name formats | Define format per domain upfront |
| Deep nested structs | Separate tables with FKs |
| Create dependent table before reference table | Follow creation order |

## MCP Workflow

```
1. create_struct → FRarityData (for lookup)
2. create_datatable → DT_Rarities using FRarityData
3. add_datatable_row → populate Rarity rows
4. create_struct → FItemData (with RarityID field)
5. create_datatable → DT_Items using FItemData
6. add_datatable_row → populate Items (RarityID matches DT_Rarities row names)
```

## Integration with unreal-mcp-architect

- Architect handles feature structure
- This skill handles data model integrity
- Both enforce planning before implementation
