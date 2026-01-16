---
name: blueprint-linter
description: >
  Analyze Unreal Engine Blueprints for quality issues using MCP metadata tools.
  Detects orphaned nodes, unconnected pins, naming violations, and function complexity.
  ALWAYS trigger on: lint blueprint, analyze blueprint, check blueprint, blueprint issues,
  blueprint problems, code review blueprint, blueprint quality, orphaned nodes,
  unconnected pins, naming conventions, complexity check, BP review, clean up blueprint.
---

# Blueprint Linter

Analyze Blueprints for quality issues using `get_blueprint_metadata`.

## Workflow

1. **Fetch metadata** - Call `get_blueprint_metadata` with asset path
2. **Run analysis** - Check each category below
3. **Report findings** - Severity-ranked list with fix suggestions

## Analysis Categories

### 1. Orphaned Nodes

Nodes disconnected from execution flow.

**Detection:**
- Nodes with no exec pin connections (for impure nodes)
- Nodes whose output is never consumed
- Reroute nodes leading nowhere

**Severity:** Warning (dead code, confusion)

### 2. Unconnected Pins

Required pins without connections.

**Detection:**
- Input pins with no connections and no default value
- Output pins marked "required" but unused
- Wildcard pins left unset

**Severity:** Error (will cause runtime issues)

### 3. Naming Violations

See `references/naming_conventions.md` for full rules.

**Quick checks:**
| Element | Convention | Bad Example | Good Example |
|---------|------------|-------------|--------------|
| Blueprint | `BP_` prefix | `MyActor` | `BP_MyActor` |
| Variable | PascalCase, descriptive | `hp`, `x` | `CurrentHealth`, `TargetLocation` |
| Function | Verb + noun | `Health` | `GetHealth`, `ApplyDamage` |
| Event | `On` prefix | `PlayerDied` | `OnPlayerDied` |
| Bool | Question form | `Dead` | `bIsDead`, `bCanJump` |

**Severity:** Info (maintainability)

### 4. Function Complexity

Functions that are too large or complex.

**Thresholds:**
| Metric | Warning | Error |
|--------|---------|-------|
| Node count | >30 | >50 |
| Branch depth | >3 | >5 |
| Parameters | >5 | >8 |
| Local variables | >5 | >10 |

**Severity:** Warning/Error based on threshold

### 5. Performance Red Flags

Patterns that hurt performance.

**Detect:**
- Tick events with heavy logic (GetAllActorsOfClass, etc.)
- Cast nodes inside loops
- SpawnActor in Tick
- No throttling on expensive operations

**Severity:** Warning (performance impact)

## Report Format

```
## Blueprint Lint Report: BP_PlayerCharacter

### ❌ Errors (2)
1. [Unconnected Pin] Function "TakeDamage" - Input "DamageType" has no connection
2. [Complexity] Function "UpdateInventory" has 67 nodes (max: 50)

### ⚠️ Warnings (3)
1. [Orphaned] Node "Print String" at position (450, 230) not connected to exec flow
2. [Naming] Variable "dmg" should be descriptive (e.g., "DamageAmount")
3. [Performance] "GetAllActorsOfClass" called in Event Tick

### ℹ️ Info (1)
1. [Naming] Function "Health" should start with verb (e.g., "GetHealth")

### Summary
- 2 errors requiring immediate fix
- 3 warnings to address
- 1 style suggestion
```

## MCP Tool Usage

```
get_blueprint_metadata(asset_path="/Game/Blueprints/BP_Example")
```

Response contains:
- `functions[]` - Each with nodes, variables, parameters
- `variables[]` - Class variables with types
- `event_graphs[]` - Event graph nodes and connections
- `node.connections[]` - Pin connection data

## Auto-Fix Suggestions

For each issue type, suggest the MCP operation to fix:

| Issue | Suggested Fix |
|-------|---------------|
| Orphaned node | `delete_blueprint_node` |
| Naming violation | `rename_variable` / `rename_function` |
| Complexity | Suggest extraction into sub-function |
| Unconnected pin | Identify what should connect or add default |
