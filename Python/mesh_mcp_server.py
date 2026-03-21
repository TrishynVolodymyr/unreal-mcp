#!/usr/bin/env python3
"""
Mesh MCP Server - Static Mesh operations for Unreal Engine.
Includes: LOD management, mesh metadata, mesh properties.
"""

import asyncio
import json
from typing import Any, Dict, List

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("Mesh MCP Server")

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
# Mesh Metadata
# ============================================================================

@app.tool()
async def get_static_mesh_metadata(
    mesh_path: str
) -> Dict[str, Any]:
    """
    Get detailed metadata about a Static Mesh asset.

    Returns LOD count, vertex/triangle counts per LOD, bounds, material slots,
    screen sizes, and other mesh properties.

    Args:
        mesh_path: Path to the Static Mesh (e.g., "/Game/Environment/GroundCover/SM_GrassPatch_01")

    Returns:
        Dictionary containing:
        - lod_count: Number of LOD levels
        - lods: Array of per-LOD info (vertices, triangles, screen_size)
        - bounds: Bounding box dimensions
        - material_slots: Array of material slot names/paths
        - has_collision: Whether mesh has collision
        - nanite_enabled: Whether Nanite is enabled

    Example:
        get_static_mesh_metadata(mesh_path="/Game/Meshes/SM_Rock_01")
    """
    return await send_tcp_command("get_static_mesh_metadata", {"mesh_path": mesh_path})


# ============================================================================
# LOD Management
# ============================================================================

@app.tool()
async def set_lod_count(
    mesh_path: str,
    lod_count: int
) -> Dict[str, Any]:
    """
    Set the number of LOD levels on a Static Mesh.

    This adds or removes LOD source models. New LODs are empty until
    populated via import_lod or auto-reduction.

    Args:
        mesh_path: Path to the Static Mesh
        lod_count: Desired number of LODs (1-8)

    Example:
        set_lod_count(mesh_path="/Game/Meshes/SM_Grass_01", lod_count=3)
    """
    return await send_tcp_command("set_lod_count", {
        "mesh_path": mesh_path,
        "lod_count": lod_count
    })


@app.tool()
async def import_lod(
    mesh_path: str,
    lod_index: int,
    source_file_path: str
) -> Dict[str, Any]:
    """
    Import a mesh file (FBX/OBJ) into a specific LOD slot of a Static Mesh.

    The target LOD slot must exist (use set_lod_count first if needed).
    The imported mesh replaces any existing geometry in that LOD slot.

    Args:
        mesh_path: Path to the target Static Mesh in UE
        lod_index: LOD slot to import into (0=highest detail, 1=first reduction, etc.)
        source_file_path: Absolute path to the FBX/OBJ file on disk

    Example:
        import_lod(
            mesh_path="/Game/Meshes/SM_Grass_01",
            lod_index=1,
            source_file_path="E:/meshes/SM_Grass_01_LOD1.fbx"
        )
    """
    return await send_tcp_command("import_lod", {
        "mesh_path": mesh_path,
        "lod_index": lod_index,
        "source_file_path": source_file_path
    })


@app.tool()
async def set_lod_screen_sizes(
    mesh_path: str,
    screen_sizes: list
) -> Dict[str, Any]:
    """
    Set screen size thresholds for LOD transitions.

    Screen size is the percentage of screen height the mesh's bounds occupy.
    Lower values = mesh appears smaller = lower LOD used.

    Args:
        mesh_path: Path to the Static Mesh
        screen_sizes: Array of screen size values per LOD.
            LOD0 is typically 1.0 (always used when close).
            Example: [1.0, 0.5, 0.25] for 3 LODs.

    Example:
        set_lod_screen_sizes(
            mesh_path="/Game/Meshes/SM_Grass_01",
            screen_sizes=[1.0, 0.4, 0.15]
        )
    """
    return await send_tcp_command("set_lod_screen_sizes", {
        "mesh_path": mesh_path,
        "screen_sizes": screen_sizes
    })


@app.tool()
async def auto_generate_lods(
    mesh_path: str,
    target_lod_count: int = 3,
    reduction_percentages: list = None
) -> Dict[str, Any]:
    """
    Auto-generate LODs using UE's built-in mesh reduction.

    Creates simplified versions of LOD0 at specified reduction percentages.
    Much faster than manually creating LOD meshes in Blender.

    Args:
        mesh_path: Path to the Static Mesh
        target_lod_count: Total number of LODs to have (including LOD0)
        reduction_percentages: Triangle reduction per LOD (0.0-1.0).
            Default: evenly distributed (e.g., [1.0, 0.5, 0.25] for 3 LODs).
            Values are percentage of LOD0 triangles to KEEP.

    Example:
        auto_generate_lods(
            mesh_path="/Game/Meshes/SM_Grass_01",
            target_lod_count=3,
            reduction_percentages=[1.0, 0.35, 0.1]
        )
    """
    params = {
        "mesh_path": mesh_path,
        "target_lod_count": target_lod_count,
    }
    if reduction_percentages:
        params["reduction_percentages"] = reduction_percentages

    return await send_tcp_command("auto_generate_lods", params)


# ============================================================================
# Mesh Properties
# ============================================================================

@app.tool()
async def set_static_mesh_properties(
    mesh_path: str,
    enable_nanite: bool = None,
    has_collision: bool = None
) -> Dict[str, Any]:
    """
    Set properties on a Static Mesh asset.

    Note: cast_shadow, cull_distance, and WorldPositionOffsetDisableDistance are
    COMPONENT-level properties (UStaticMeshComponent), not mesh asset properties.
    For PCG-spawned instances, use set_pcg_node_property with dotted paths on the
    descriptor (e.g., "MeshEntries[0].Descriptor.WorldPositionOffsetDisableDistance").

    Args:
        mesh_path: Path to the Static Mesh asset
        enable_nanite: Enable/disable Nanite (note: doesn't work well with masked materials)
        has_collision: Enable/disable collision

    Example:
        set_static_mesh_properties(
            mesh_path="/Game/Meshes/SM_Grass_01",
            enable_nanite=False
        )
    """
    params = {"mesh_path": mesh_path}
    if enable_nanite is not None:
        params["enable_nanite"] = enable_nanite
    if has_collision is not None:
        params["has_collision"] = has_collision

    return await send_tcp_command("set_static_mesh_properties", params)


if __name__ == "__main__":
    app.run()
