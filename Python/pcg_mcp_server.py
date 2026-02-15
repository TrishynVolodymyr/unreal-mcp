#!/usr/bin/env python3
"""
PCG MCP Server - Procedural Content Generation tools for Unreal Engine.
Includes: create_pcg_graph, add_pcg_node, connect_pcg_nodes, set_pcg_node_property,
           get_pcg_graph_metadata, search_pcg_palette, remove_pcg_node,
           spawn_pcg_actor, execute_pcg_graph.
"""

import asyncio
import json
from typing import Any, Dict

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("PCG MCP Server")

# TCP connection settings
TCP_HOST = "127.0.0.1"
TCP_PORT = 55557


async def send_tcp_command(command_type: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to the Unreal Engine TCP server."""
    try:
        command_data = {
            "type": command_type,
            "params": params
        }

        json_data = json.dumps(command_data)

        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)

        writer.write(json_data.encode('utf-8'))
        writer.write(b'\n')
        await writer.drain()

        response_data = await reader.read(49152)
        response_str = response_data.decode('utf-8').strip()

        writer.close()
        await writer.wait_closed()

        if response_str:
            try:
                return json.loads(response_str)
            except json.JSONDecodeError as json_err:
                return {"success": False, "error": f"JSON decode error: {str(json_err)}, Response: {response_str[:200]}"}
        else:
            return {"success": False, "error": "Empty response from server"}

    except Exception as e:
        return {"success": False, "error": f"TCP communication error: {str(e)}"}


# ============================================================================
# PCG Graph Management
# ============================================================================

@app.tool()
async def create_pcg_graph(
    name: str,
    path: str = "/Game/PCG",
    template: str = ""
) -> dict:
    """Create a new PCG Graph asset, optionally from a template.

    Without template: creates an empty graph with Input and Output nodes.
    With template: duplicates a built-in or custom template graph.

    Built-in templates include:
    - _Default_EmptyGraph, _Default_Loop, _Default_LoopFeedback
    - TPL_Sampler_Volume, TPL_Sampler_Spline, TPL_Sampler_Texture
    - TPL_Showcase_SimpleForest, TPL_Showcase_HierarchicalGenerationForest
    - TPL_Showcase_RuntimeGrassGPU, TPL_Showcase_ShapeGrammar
    Use search_pcg_palette() with templates or check Content Browser for full list.

    Args:
        name: Name of the PCG Graph asset (e.g., "PCG_ForestScatter")
        path: Content Browser folder path (default: "/Game/PCG")
        template: Optional template name (e.g., "TPL_Showcase_SimpleForest")

    Returns:
        Dict with graph_path and success status
    """
    params = {"name": name, "path": path}
    if template:
        params["template"] = template
    return await send_tcp_command("create_pcg_graph", params)


@app.tool()
async def get_pcg_graph_metadata(
    graph_path: str,
    include_properties: bool = False
) -> dict:
    """Get metadata about a PCG Graph including all nodes, connections, and pins.

    Args:
        graph_path: Path to the PCG Graph asset (e.g., "/Game/PCG/PCG_ForestScatter")
        include_properties: If True, include settings properties for each node

    Returns:
        Dict with graph structure: input/output nodes, all nodes with pins and connections
    """
    return await send_tcp_command("get_pcg_graph_metadata", {
        "graph_path": graph_path,
        "include_properties": include_properties
    })


@app.tool()
async def search_pcg_palette(
    search_query: str = "",
    max_results: int = 50
) -> dict:
    """Search for available PCG node types in the palette.

    Discovers available PCG Settings classes (Surface Sampler, Static Mesh Spawner,
    Density Filter, etc.) that can be added to PCG Graphs.

    Args:
        search_query: Text to search in node names (case-insensitive). Empty = list all.
        max_results: Maximum number of results to return (default: 50)

    Returns:
        Dict with matching PCG node types, their class names and categories
    """
    return await send_tcp_command("search_pcg_palette", {
        "search_query": search_query,
        "max_results": max_results
    })


@app.tool()
async def add_pcg_node(
    graph_path: str,
    settings_class: str,
    node_position: list[int] = None,
    node_label: str = ""
) -> dict:
    """Add a node to a PCG Graph.

    Use search_pcg_palette() to discover available settings_class names.

    Args:
        graph_path: Path to the PCG Graph asset (e.g., "/Game/PCG/PCG_ForestScatter")
        settings_class: Class name of the PCG settings (e.g., "PCGSurfaceSamplerSettings")
        node_position: Optional [X, Y] position in the graph editor
        node_label: Optional custom label for the node

    Returns:
        Dict with node_id, title, pins, and position
    """
    params = {"graph_path": graph_path, "settings_class": settings_class}
    if node_position:
        params["node_position"] = node_position
    if node_label:
        params["node_label"] = node_label
    return await send_tcp_command("add_pcg_node", params)


@app.tool()
async def connect_pcg_nodes(
    graph_path: str,
    source_node_id: str,
    source_pin: str = "Out",
    target_node_id: str = "",
    target_pin: str = "In"
) -> dict:
    """Connect two nodes in a PCG Graph.

    Standard pin names: "In", "Out", "Overrides".
    Use get_pcg_graph_metadata() to discover custom pin names on specific nodes.

    Args:
        graph_path: Path to the PCG Graph asset
        source_node_id: ID of the source node (from add_pcg_node or get_pcg_graph_metadata)
        source_pin: Output pin label on source node (default: "Out")
        target_node_id: ID of the target node
        target_pin: Input pin label on target node (default: "In")

    Returns:
        Dict with success status and connection description
    """
    return await send_tcp_command("connect_pcg_nodes", {
        "graph_path": graph_path,
        "source_node_id": source_node_id,
        "source_pin": source_pin,
        "target_node_id": target_node_id,
        "target_pin": target_pin
    })


@app.tool()
async def remove_pcg_node(
    graph_path: str,
    node_id: str
) -> dict:
    """Remove a node from a PCG Graph. Automatically disconnects all edges.
    Cannot remove Input/Output nodes.

    Args:
        graph_path: Path to the PCG Graph asset
        node_id: ID of the node to remove
    """
    return await send_tcp_command("remove_pcg_node", {
        "graph_path": graph_path,
        "node_id": node_id
    })


@app.tool()
async def set_pcg_node_property(
    graph_path: str,
    node_id: str,
    property_name: str,
    property_value: str
) -> dict:
    """Set a property on a PCG node's settings.

    Properties are set via string values that get parsed to the correct type.
    Use get_pcg_graph_metadata(include_properties=True) to discover available properties.

    Args:
        graph_path: Path to the PCG Graph asset
        node_id: ID of the target node
        property_name: Name of the property to set (e.g., "PointsPerSquaredMeter")
        property_value: Value as string (e.g., "0.5", "true", "/Game/Meshes/SM_Tree")

    Returns:
        Dict with success status and property info
    """
    return await send_tcp_command("set_pcg_node_property", {
        "graph_path": graph_path,
        "node_id": node_id,
        "property_name": property_name,
        "property_value": property_value
    })


# ============================================================================
# PCG Actor & Execution
# ============================================================================

@app.tool()
async def spawn_pcg_actor(
    graph_path: str,
    location: list[float] = None,
    rotation: list[float] = None,
    actor_label: str = "",
    generation_trigger: str = "GenerateOnLoad",
    volume_extents: list[float] = None
) -> dict:
    """Spawn an actor with a PCG Component in the editor level.

    Creates a new actor with a box volume (for PCG bounds), attaches a UPCGComponent,
    assigns the specified PCG Graph, and configures the generation trigger.

    The volume_extents define the half-size of the generation area in Unreal Units.
    Default is [500, 500, 500] = 10m x 10m x 10m area.

    Args:
        graph_path: Path to the PCG Graph asset to assign (e.g., "/Game/PCG/PCG_ForestScatter")
        location: [X, Y, Z] world position (default: origin)
        rotation: [Pitch, Yaw, Roll] rotation in degrees (default: zero)
        actor_label: Display name for the actor in the editor
        generation_trigger: "GenerateOnLoad", "GenerateOnDemand", or "GenerateAtRuntime"
        volume_extents: [X, Y, Z] half-extents of the bounds box in UU (default: [500, 500, 500])

    Returns:
        Dict with actor_name, actor_path, component_name, and success status
    """
    params = {"graph_path": graph_path}
    if location:
        params["location"] = location
    if rotation:
        params["rotation"] = rotation
    if actor_label:
        params["actor_label"] = actor_label
    if generation_trigger:
        params["generation_trigger"] = generation_trigger
    if volume_extents:
        params["volume_extents"] = volume_extents
    return await send_tcp_command("spawn_pcg_actor", params)


@app.tool()
async def execute_pcg_graph(
    actor_name: str,
    force: bool = True
) -> dict:
    """Execute/regenerate a PCG Graph on an actor's PCG Component.

    Triggers generation on the PCG Component attached to the specified actor.

    Args:
        actor_name: Name or label of the actor with PCG Component
        force: If True, force full regeneration even if inputs haven't changed

    Returns:
        Dict with success status and generation info
    """
    return await send_tcp_command("execute_pcg_graph", {
        "actor_name": actor_name,
        "force": force
    })


@app.tool()
async def configure_pcg_mesh_spawner(
    graph_path: str,
    node_id: str,
    mesh_entries: list[dict],
    append: bool = False
) -> dict:
    """Configure mesh entries on a PCG Static Mesh Spawner node.

    Sets the weighted mesh list on a Static Mesh Spawner's MeshSelectorWeighted.
    This reaches into nested sub-objects that set_pcg_node_property cannot access.

    Args:
        graph_path: Path to the PCG Graph asset (e.g., "/Game/PCG/PCG_Forest")
        node_id: Node name/ID in the graph
        mesh_entries: List of mesh entries, each a dict with "mesh" (asset path) and optional "weight" (int, default 1).
                      Example: [{"mesh": "/Game/Meshes/SM_Tree", "weight": 1}, {"mesh": "/Game/Meshes/SM_Bush", "weight": 3}]
        append: If True, append to existing entries; if False (default), replace all

    Returns:
        Dict with success status, entries count, and node_id
    """
    return await send_tcp_command("configure_pcg_mesh_spawner", {
        "graph_path": graph_path,
        "node_id": node_id,
        "mesh_entries": mesh_entries,
        "append": append
    })


if __name__ == "__main__":
    app.run(transport="stdio")
