# PCG Tools - Unreal MCP

This document provides comprehensive information about using Procedural Content Generation (PCG) tools through the Unreal MCP (Model Context Protocol). These tools allow you to create, configure, and execute PCG graphs — Unreal Engine's system for procedural world-building — entirely through natural language commands.

## Overview

PCG tools enable you to:
- Create PCG Graph assets with optional templates (duplicate existing graphs as starting points)
- Inspect full graph structure: nodes, pins, connections, and settings properties
- Search the PCG palette for available node types (195+ in UE 5.7)
- Add nodes to graphs by class name (surface samplers, mesh spawners, filters, etc.)
- Connect nodes together to build procedural pipelines
- Set node properties using UE reflection for precise configuration
- Remove nodes with automatic edge disconnection
- Spawn actors with PCG components and assigned graphs
- Execute PCG generation on demand with cleanup of previous results

## Natural Language Usage Examples

### Creating PCG Graphs

```
"Create a new PCG Graph called 'PCG_ForestScatter' in /Game/PCG"

"Create a PCG Graph called 'PCG_RockFormation' using 'PCG_Template_Scatter' as a template"

"Make a new PCG graph for procedural grass placement"
```

### Inspecting Graphs

```
"Show me the full structure of PCG_ForestScatter — nodes, connections, and settings"

"Get the metadata for /Game/PCG/PCG_ForestScatter"

"What nodes are in the PCG_RockFormation graph?"
```

### Searching for Node Types

```
"Search the PCG palette for surface sampler nodes"

"What PCG node types are available for mesh spawning?"

"Search PCG nodes for 'density' to find filtering options"

"List all available PCG node categories"
```

### Building Graph Pipelines

```
"Add a Surface Sampler node to PCG_ForestScatter"

"Add a Static Mesh Spawner node to the forest scatter graph"

"Connect the Surface Sampler output to the Mesh Spawner input in PCG_ForestScatter"

"Add a Density Filter node between the sampler and spawner"
```

### Configuring Nodes

```
"Set the PointsPerSquaredMeter property to 0.5 on the Surface Sampler node in PCG_ForestScatter"

"Change the mesh spawner's StaticMesh to /Game/Meshes/SM_Tree_Pine"

"Set the MinDensity to 0.3 on the Density Filter"
```

### Spawning and Executing

```
"Spawn a PCG actor using PCG_ForestScatter at position [0, 0, 0] with volume extents [2000, 2000, 500]"

"Execute the PCG generation on the ForestScatter actor"

"Spawn a PCG actor with the rock formation graph at [1000, 500, 0] and run generation"
```

## Tool Reference

---

### `create_pcg_graph`

Create a new PCG Graph asset. Optionally duplicate from an existing template graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the graph (e.g., `"PCG_ForestScatter"`) |
| `path` | string | | Folder path for the asset (default: `"/Game/PCG"`) |
| `template` | string | | Name of an existing PCG Graph to duplicate as a starting point. Found via AssetRegistry by name |

**Example:**
```
create_pcg_graph(
  name="PCG_ForestScatter",
  path="/Game/PCG/Vegetation"
)
```

**Example with template:**
```
create_pcg_graph(
  name="PCG_DenseForest",
  template="PCG_ForestScatter"
)
```

**Notes:**
- Template lookup uses the AssetRegistry to find graphs by name, so you don't need the full path
- Template duplication uses `StaticDuplicateObject`, the same mechanism as the engine's PCGGraphFactory

---

### `get_pcg_graph_metadata`

Get full graph structure including all nodes, their pins, connections between nodes, and settings properties on each node.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graph_path` | string | ✅ | Path to the PCG Graph asset |

**Example:**
```
get_pcg_graph_metadata(
  graph_path="/Game/PCG/PCG_ForestScatter"
)
```

**Returns:**
- List of all nodes with labels and indices
- Pin names and types on each node
- All connections (source node/pin → target node/pin)
- Settings properties and current values for each node

**Notes:**
- Use this before modifying a graph to understand its current structure
- Nodes can be referenced by label (display name) or index in subsequent commands

---

### `search_pcg_palette`

Search available PCG node types that can be added to graphs. Returns class names usable with `add_pcg_node`.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `search_query` | string | | Text to search in node type names (e.g., `"sampler"`, `"spawner"`, `"filter"`) |
| `category` | string | | Filter by category |

**Example:**
```
search_pcg_palette(search_query="sampler")
```

**Example — browse all:**
```
search_pcg_palette()
```

**Notes:**
- UE 5.7 PCG plugin provides 195+ node types
- Returns the class names (e.g., `"PCGSurfaceSampler"`, `"PCGStaticMeshSpawner"`) that you pass to `add_pcg_node`
- Use this to discover available nodes when building pipelines

---

### `add_pcg_node`

Add a node to a PCG graph by its class name. The editor graph is auto-refreshed after the operation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graph_path` | string | ✅ | Path to the PCG Graph |
| `node_type` | string | ✅ | PCG node class name (e.g., `"PCGSurfaceSampler"`, `"PCGStaticMeshSpawner"`, `"PCGDensityFilter"`) |

**Example:**
```
add_pcg_node(
  graph_path="/Game/PCG/PCG_ForestScatter",
  node_type="PCGSurfaceSampler"
)
```

**Notes:**
- Use `search_pcg_palette` to discover valid node type names
- The graph is saved immediately after the node is added
- The PCG editor graph is auto-refreshed (dual-graph sync)

---

### `connect_pcg_nodes`

Connect two PCG nodes in a graph. The editor graph is auto-refreshed after the operation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graph_path` | string | ✅ | Path to the PCG Graph |
| `source_node` | string | ✅ | Source node label (display name) or index |
| `source_pin` | string | | Output pin name on the source node (default: `"Out"`) |
| `target_node` | string | ✅ | Target node label (display name) or index |
| `target_pin` | string | | Input pin name on the target node (default: `"In"`) |

**Example:**
```
connect_pcg_nodes(
  graph_path="/Game/PCG/PCG_ForestScatter",
  source_node="Surface Sampler",
  target_node="Static Mesh Spawner"
)
```

**Example with explicit pins:**
```
connect_pcg_nodes(
  graph_path="/Game/PCG/PCG_ForestScatter",
  source_node="Surface Sampler",
  source_pin="Out",
  target_node="Density Filter",
  target_pin="In"
)
```

**Notes:**
- Nodes can be referenced by their display label or their index in the graph (use `get_pcg_graph_metadata` to find these)
- Default pin names are `"Out"` and `"In"` which work for most standard nodes
- The graph is saved and the editor refreshed after connection

---

### `set_pcg_node_property`

Set a property on a PCG node's settings object. Uses Unreal Engine's reflection system and ImportText for value parsing.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graph_path` | string | ✅ | Path to the PCG Graph |
| `node` | string | ✅ | Node label (display name) or index |
| `property_name` | string | ✅ | Name of the property on the node's settings |
| `property_value` | string | ✅ | Value as string (parsed via UE ImportText) |

**Example — set a float property:**
```
set_pcg_node_property(
  graph_path="/Game/PCG/PCG_ForestScatter",
  node="Surface Sampler",
  property_name="PointsPerSquaredMeter",
  property_value="0.5"
)
```

**Example — set an asset reference:**
```
set_pcg_node_property(
  graph_path="/Game/PCG/PCG_ForestScatter",
  node="Static Mesh Spawner",
  property_name="StaticMesh",
  property_value="/Game/Meshes/SM_Tree_Pine.SM_Tree_Pine"
)
```

**Notes:**
- Property values are parsed using UE's `ImportText`, so format follows Unreal conventions
- Use `get_pcg_graph_metadata` to see available properties and their current values
- The graph is saved and editor refreshed after the change

---

### `remove_pcg_node`

Remove a node from a PCG graph. All connected edges are automatically disconnected before removal.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graph_path` | string | ✅ | Path to the PCG Graph |
| `node` | string | ✅ | Node label (display name) or index |

**Example:**
```
remove_pcg_node(
  graph_path="/Game/PCG/PCG_ForestScatter",
  node="Density Filter"
)
```

**Notes:**
- Connected edges are automatically removed before the node is deleted
- The graph is saved and editor refreshed after removal
- Removing a node in the middle of a pipeline will break the connection chain — reconnect the remaining nodes afterward

---

### `spawn_pcg_actor`

Spawn an actor in the level with a PCG Component and an assigned PCG Graph. The actor includes a volume defining the generation bounds.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graph_path` | string | ✅ | Path to the PCG Graph asset to assign |
| `location` | array | | `[X, Y, Z]` world location (default: `[0, 0, 0]`) |
| `rotation` | array | | `[Pitch, Yaw, Roll]` rotation in degrees |
| `actor_label` | string | | Display name/label for the spawned actor |
| `generation_trigger` | string | | When to trigger generation (default: `"GenerateOnDemand"`) |
| `volume_extents` | array | | `[X, Y, Z]` half-extents of the generation volume (default: `[500, 500, 500]`) |

**Example:**
```
spawn_pcg_actor(
  graph_path="/Game/PCG/PCG_ForestScatter",
  location=[0, 0, 0],
  actor_label="ForestScatter_Main",
  volume_extents=[2000, 2000, 500]
)
```

**Example with rotation:**
```
spawn_pcg_actor(
  graph_path="/Game/PCG/PCG_RockFormation",
  location=[1000, 500, 0],
  rotation=[0, 45, 0],
  actor_label="RockFormation_Cliff",
  volume_extents=[1000, 1000, 300]
)
```

**Notes:**
- `generation_trigger` defaults to `"GenerateOnDemand"` — call `execute_pcg_graph` to run generation
- Volume extents define the half-size of the bounding box within which PCG operates
- The actor label is used to reference the actor in `execute_pcg_graph`

---

### `execute_pcg_graph`

Execute PCG generation on an actor that has a PCG Component. Cleans up any previous generation results before running.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `actor_label` | string | ✅ | Label of the actor with PCG Component |
| `force` | boolean | | Force regeneration even if already generated (default: `true`) |

**Example:**
```
execute_pcg_graph(
  actor_label="ForestScatter_Main"
)
```

**Notes:**
- Previous generation output is cleaned up automatically before re-executing
- The actor must have a PCG Component with an assigned graph (use `spawn_pcg_actor` to set this up)
- With `force=true` (default), regeneration always runs regardless of whether results already exist

## Advanced Usage Patterns

### Building a Complete Forest Scatter Pipeline

```
"Build a procedural forest:
1. Create a PCG Graph called 'PCG_Forest' in /Game/PCG/Vegetation
2. Search the PCG palette for surface sampler nodes
3. Add a Surface Sampler node to PCG_Forest
4. Set PointsPerSquaredMeter to 0.3 on the Surface Sampler
5. Add a Density Filter node to the graph
6. Set MinDensity to 0.2 on the Density Filter
7. Add a Static Mesh Spawner node
8. Set the StaticMesh property on the spawner to /Game/Meshes/SM_Tree_Oak
9. Connect Surface Sampler → Density Filter → Static Mesh Spawner
10. Spawn a PCG actor with PCG_Forest at [0, 0, 0] with volume extents [5000, 5000, 1000]
11. Execute PCG generation on the actor"
```

### Creating a Rock Formation with Variations

```
"Build procedural rock placement:
1. Create PCG_RockFormation
2. Add a Surface Sampler — set density to 0.1
3. Add a Density Noise node for natural variation
4. Add a Static Mesh Spawner for large rocks
5. Set the mesh to SM_Rock_Large
6. Connect: Sampler → Density Noise → Spawner
7. Spawn the actor at [2000, 0, 0] with extents [3000, 3000, 500]
8. Execute generation"
```

### Duplicating and Tweaking Graphs

```
"Create a dense grass variant from the forest scatter:
1. Create PCG_DenseGrass using PCG_Forest as a template
2. Get the graph metadata to see existing nodes
3. Set PointsPerSquaredMeter to 2.0 on the Surface Sampler
4. Set the StaticMesh on the spawner to SM_Grass_Clump
5. Spawn a PCG actor at [0, 0, 0] with extents [3000, 3000, 200]
6. Execute generation"
```

### Inspecting and Modifying Existing Graphs

```
"Analyze and tweak the existing scatter graph:
1. Get metadata for PCG_ForestScatter to see all nodes and connections
2. Search the PCG palette for 'transform' to find rotation randomization nodes
3. Add a Transform Points node between the sampler and spawner
4. Remove the old direct connection
5. Connect Sampler → Transform Points → Spawner
6. Set the rotation range on Transform Points
7. Execute generation to see the changes"
```

### Multi-Layer Procedural Environment

```
"Build a multi-layer environment with ground cover, shrubs, and trees:
1. Create PCG_Environment_GroundCover with dense grass sampling
2. Create PCG_Environment_Shrubs with medium-density shrub placement
3. Create PCG_Environment_Trees with sparse tree distribution
4. Spawn three PCG actors at the same location with different volume sizes
5. Execute all three graphs for a layered procedural landscape"
```

## Best Practices for Natural Language Commands

### Inspect Before Building
Before adding or modifying nodes, ask: *"Get the metadata for PCG_ForestScatter"* to understand the existing graph structure, available pins, and current property values.

### Search the Palette First
Instead of guessing node type names, use: *"Search the PCG palette for 'density'"* to discover exact class names before adding nodes. The palette has 195+ node types.

### Reference Nodes by Label
When modifying existing nodes, use the display label: *"Set PointsPerSquaredMeter to 0.5 on the Surface Sampler node"*. If labels are ambiguous, use the index from `get_pcg_graph_metadata`.

### Build Pipelines Step by Step
Create the pipeline incrementally:
1. Add all nodes first
2. Connect them in order
3. Configure properties on each node
4. Spawn and execute

### Use Templates for Variations
Instead of rebuilding from scratch: *"Create PCG_Variant using PCG_Original as a template"* — then modify only the properties that differ.

### Set Volume Extents Appropriately
Match volume extents to your scene scale. Small props (rocks, flowers): `[500, 500, 200]`. Medium areas (gardens): `[2000, 2000, 500]`. Large landscapes (forests): `[10000, 10000, 2000]`.

## Common Use Cases

### Vegetation Scatter
Placing trees, bushes, grass, and flowers procedurally across terrain using surface samplers with density controls and mesh spawners. Layer multiple graphs for ground cover, mid-level shrubs, and canopy trees.

### Rock and Debris Placement
Distributing rocks, boulders, and debris naturally along surfaces with density noise for organic distribution. Use transform randomization for varied orientations and scales.

### Building and Structure Placement
Procedural placement of modular building pieces, fences, walls, and props along splines or within volumes. Combine with spatial filters for organized layouts.

### Environment Decoration
Scattering decorative elements (lanterns, barrels, crates, signs) in gameplay areas. Use distance-based filtering to control density around points of interest.

### Landscape Population
Large-scale world decoration combining multiple PCG graphs for biome-specific content. Different graphs handle different layers of the environment.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"Graph not found"**: Verify the full asset path (e.g., `/Game/PCG/PCG_ForestScatter`). Use `get_pcg_graph_metadata` to confirm the asset exists and see its exact path.

2. **"Node not found in graph"**: Node labels may differ from what you expect. Use `get_pcg_graph_metadata` to list all nodes with their exact labels and indices. Reference by index if labels are ambiguous.

3. **Node type not recognized**: Use `search_pcg_palette` to find the exact class name. Node types are case-sensitive (e.g., `"PCGSurfaceSampler"` not `"SurfaceSampler"`).

4. **Property not applying**: Property names must match the UE reflection name exactly. Use `get_pcg_graph_metadata` to list available properties on a node's settings object.

5. **No visible output after execution**: Check that:
   - The PCG actor has a valid graph assigned
   - The volume extents are large enough to cover the sampling area
   - The mesh spawner has a valid StaticMesh assigned
   - The Surface Sampler is sampling from an actual surface (landscape/mesh)

6. **Editor graph looks stale**: All modification commands auto-refresh the PCG editor by nulling the cached editor graph. If the visual editor still looks outdated, close and reopen the PCG Graph asset in the editor.

7. **Connections not working**: Verify pin names with `get_pcg_graph_metadata`. Most standard nodes use `"Out"` and `"In"` as default pin names, but some nodes have specialized pin names.

## Technical Details

### Dual-Graph Architecture
The PCG editor uses a dual-graph system:
- **UPCGGraph** — the data graph containing the actual node/connection data
- **UPCGEditorGraph** — the visual editor graph for display

All modification commands automatically refresh the editor by nulling the cached editor graph pointer and reopening, ensuring the visual editor stays in sync with the data graph.

### Immediate Persistence
Every modification command (add node, connect, set property, remove) calls `SavePackage` on the graph asset immediately. Changes are never left in an unsaved state.

### Node Referencing
Nodes can be referenced by:
- **Label** — the display name shown in the editor (e.g., `"Surface Sampler"`)
- **Index** — the numeric position in the graph's node array (useful when multiple nodes share similar labels)

### Template System
Template duplication uses `StaticDuplicateObject`, the same mechanism used by the engine's `PCGGraphFactory`. This creates a deep copy of the entire graph including all nodes, connections, and settings.

## Performance Considerations

- **Volume extents impact generation cost**: Larger volumes sample more points and spawn more actors. Start with smaller volumes during iteration, then scale up for final results.
- **Density matters**: High `PointsPerSquaredMeter` values with large volumes can generate thousands of instances. Use density filters to control the final spawn count.
- **Layer your graphs**: Instead of one massive graph with many spawner nodes, use separate graphs for different content layers (ground cover, mid-level, canopy). This allows independent iteration and optimization.
- **Use GenerateOnDemand**: The default trigger mode prevents automatic regeneration during editor operations. Call `execute_pcg_graph` explicitly when you want to see results.
- **Template graphs save time**: For similar setups, duplicate an existing graph as a template instead of rebuilding from scratch. Modify only the properties that differ.
