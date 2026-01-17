---
name: asset-state-extractor
description: >
  Extract and snapshot current state of Unreal Engine assets using MCP metadata tools.
  Creates reference files documenting asset structure, logic, parameters, and configuration.
  ALWAYS trigger on: "snapshot", "capture state", "extract state", "current state of",
  "document this asset", "what does this have", "show me what's in", "get the state",
  "save state", "dump", "export metadata", "create reference for", "before we start show me",
  "let's see what we have", "inspect asset", "asset overview", "what's the current setup".
  Uses get_blueprint_metadata, get_widget_blueprint_metadata, get_niagara_metadata,
  get_material_metadata, get_material_graph_metadata based on asset type.
---

# Asset State Extractor

Extract current state from UE assets into reference files for workflow context.

## When to Use

- Before modifying an asset — capture baseline state
- During iterative work — track what exists
- Debugging — understand current configuration
- Handoff — document what's implemented

## Core Workflow

### Step 1: Identify Asset Type

Ask or infer from context:

| Asset Type | Indicators |
|------------|-----------|
| Blueprint Class | `BP_`, gameplay logic, actors |
| Widget Blueprint | `WBP_`, `W_`, UI, menus |
| Niagara System | `NS_`, particle effects, VFX |
| Niagara Emitter | `NE_`, particle emitter |
| Material | `M_`, shaders, surfaces |
| DataTable | `DT_`, data rows |

### Step 2: Extract with Correct Tool

**Blueprint Class:**
```
get_blueprint_metadata(
  blueprint_name="BP_AssetName",
  fields=["variables", "functions", "components", "graph_nodes", "event_dispatchers"],
  detail_level="full"
)
```

**Widget Blueprint:**
```
get_widget_blueprint_metadata(
  widget_name="WBP_WidgetName",
  fields=["layout", "components", "bindings", "animations", "variables", "functions"]
)
```

**Niagara System/Emitter:**

**Step 1: Get structure**
```
get_niagara_metadata(
  asset_path="/Game/Effects/NS_SystemName"
)
```
Returns: emitters, modules list, parameters, renderers, data interfaces.

**Step 2: For EACH module, get actual values (CRITICAL)**
```
get_module_inputs(
  system_path="/Game/Effects/NS_SystemName",
  emitter_name="NE_EmitterName",
  module_name="ModuleName",        // e.g., "InitializeParticle", "Gravity Force", "Set"
  stage="Spawn"                    // or "Update", "EmitterSpawn", "EmitterUpdate"
)
```
Returns: All input parameters with actual values, dynamic input types, random ranges, curves.

**IMPORTANT: Query ALL modules including:**
- Standard modules (InitializeParticle, Gravity Force, Drag, etc.)
- **SetVariables/Scratch modules** (named "Set" or with GUID suffix) - These OVERRIDE particle attributes directly!
- Custom modules

**Step 3: Get renderer configuration**
```
get_renderer_properties(
  system_path="/Game/Effects/NS_SystemName",
  emitter_name="NE_EmitterName"
)
```
Returns: Material, alignment, bindings, sort mode, etc.

**Material:**
```
get_material_metadata(
  material_path="/Game/Materials/M_MaterialName"
)
```
For detailed node graph:
```
get_material_graph_metadata(
  material_path="/Game/Materials/M_MaterialName"
)
```

**DataTable:**
```
get_datatable_row_names(datatable_path="/Game/Data/DT_TableName")
get_datatable_rows(datatable_path, row_names=["Row1", "Row2"])
```

### Step 3: Write State File

Save to project working directory:

```
/home/claude/state/{AssetName}_state.md
```

## State File Format

```markdown
# {Asset Name} State
**Type:** {Blueprint|Widget|Niagara|Material|DataTable}
**Path:** {/Game/path/to/asset}
**Captured:** {timestamp}

## Summary
{1-2 sentence overview of what this asset does}

## Structure
{Asset-specific sections below}

### Variables (Blueprints/Widgets)
| Name | Type | Default | Category |
|------|------|---------|----------|

### Functions (Blueprints/Widgets)
| Name | Inputs | Outputs | Description |
|------|--------|---------|-------------|

### Components (Blueprints/Widgets)
- {Component hierarchy}

### Graph Logic (Blueprints)
{Key event handlers and their connections}

### Emitters (Niagara)
| Emitter | Sim Target | Renderer |
|---------|-----------|----------|

### Module Stack (Niagara) - Query EACH module for values!

**Emitter Update:**
| Module | Enabled | Key Parameters |
|--------|---------|----------------|

**Particle Spawn:**
| Module | Enabled | Key Parameters |
|--------|---------|----------------|
| InitializeParticle | Yes | Lifetime: X-Y, SpriteSize: (W,H), Color: RGBA |
| **SetVariables** | Yes | **⚠️ CRITICAL: SpriteSize: (15,15), Lifetime: 5** |

**Particle Update:**
| Module | Enabled | Key Parameters |
|--------|---------|----------------|

**⚠️ SetVariables modules OVERRIDE particle attributes - always capture their values!**

### Renderer (Niagara)
| Property | Value |
|----------|-------|
| Material | /Game/path |
| Alignment | VelocityAligned |
| Bindings | Position, Color, SpriteSize, etc. |

### Parameters (Niagara/Materials)
| Name | Type | Default | Exposed |
|------|------|---------|---------|

### Node Graph (Materials)
{Key nodes and connections to material outputs}

### Rows (DataTables)
{Row names and key field values}

## Notes
{Any observations: orphaned nodes, missing connections, optimization concerns}
```

## Output Guidelines

- **Be selective** — Include what's useful for workflow, not raw dump
- **Highlight issues** — Note orphaned nodes, missing bindings, etc.
- **Track key logic** — For blueprints, focus on main event handlers
- **Keep readable** — Human-scannable, not just data

## Quick Reference

| Asset | Tools | Key Fields |
|-------|-------|-----------|
| BP_ | `get_blueprint_metadata` | variables, functions, graph_nodes, components |
| WBP_ | `get_widget_blueprint_metadata` | layout, components, bindings, variables |
| NS_/NE_ | `get_niagara_metadata` + `get_module_inputs` (per module!) + `get_renderer_properties` | emitters, module values, parameters, renderer config |
| M_ | `get_material_metadata` + `get_material_graph_metadata` | expressions, parameters, connections |
| DT_ | `get_datatable_row_names` + `get_datatable_rows` | rows, fields |

**Niagara requires multiple tool calls:**
1. `get_niagara_metadata` → Get structure (emitter list, module names)
2. `get_module_inputs` → Call for EACH module to get actual parameter values
3. `get_renderer_properties` → Get renderer configuration

## Multiple Assets

When extracting state for related assets (e.g., a widget + its data source), create separate state files and cross-reference them.

## Integration with Other Skills

State files serve as context for:
- **unreal-mcp-architect** — Know what exists before adding
- **unreal-mcp-debugger** — Baseline for investigation
- **umg-widget-designer** — Current layout reference
- **niagara-vfx-architect** — Existing module configuration
- **unreal-mcp-materials** — Current node graph state
