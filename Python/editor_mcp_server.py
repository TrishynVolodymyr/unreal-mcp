"""
Editor MCP Server

Exposes Editor-related tools for Unreal Engine via MCP.

## Self-Documentation

Use get_mcp_help() to discover available tools and their capabilities:
    get_mcp_help()                              # List all tools
    get_mcp_help(tool_name="spawn_actor")       # Detailed help for specific tool

## spawn_actor - Comprehensive Actor Spawning

Supports all common actor types with type-specific configuration:

TYPE RESOLUTION:
    1. Friendly names: StaticMeshActor, TriggerBox, PlayerStart, etc.
    2. Blueprint paths: "Blueprint:/Game/Path/To/BP_Name"
    3. Native class paths: "Class:/Script/Engine.TriggerBox"
    4. Direct paths: "/Game/Path/To/BP_Name"

SUPPORTED TYPES:
    Basic Actors:
        StaticMeshActor, PointLight, SpotLight, DirectionalLight, CameraActor

    Volumes/BSP:
        TriggerBox, TriggerSphere, TriggerCapsule, BlockingVolume,
        NavMeshBoundsVolume, PhysicsVolume, AudioVolume, PostProcessVolume,
        LightmassImportanceVolume, KillZVolume, PainCausingVolume

    Utility Actors:
        TextRenderActor, PlayerStart, TargetPoint, DecalActor, Note,
        ExponentialHeightFog, SkyLight, SphereReflectionCapture, BoxReflectionCapture

BASICSHAPES (for mesh_path):
    /Engine/BasicShapes/Cube, /Engine/BasicShapes/Sphere,
    /Engine/BasicShapes/Cylinder, /Engine/BasicShapes/Cone, /Engine/BasicShapes/Plane

Examples:
    # Platform using BasicShapes cube
    spawn_actor(name="Platform1", type="StaticMeshActor",
               mesh_path="/Engine/BasicShapes/Cube", scale=[10, 10, 0.5])

    # Text label
    spawn_actor(name="Label1", type="TextRenderActor",
               text_content="JUMP HERE", text_size=50)

    # Trigger zone
    spawn_actor(name="Checkpoint1", type="TriggerBox",
               box_extent=[200, 200, 100])

    # Player spawn
    spawn_actor(name="PlayerSpawn", type="PlayerStart")

    # Custom Blueprint
    spawn_actor(name="MyPortal", type="Blueprint:/Game/Testing/BP_TestPortal")

    # ANY native class
    spawn_actor(name="Volume", type="Class:/Script/Engine.CameraBlockingVolume")

## Other Tools
- get_mcp_help(tool_name) - Get help for any tool (START HERE)
- get_level_metadata() - Get actors and level info
- delete_actor(name) - Delete an actor
- set_actor_transform(name, location, rotation, scale) - Set transform
- get_actor_properties(name) - Get actor properties
- set_actor_property(name, property_name, value) - Set property
- set_light_property(name, property_name, value) - Set light property
- spawn_blueprint_actor() - Legacy Blueprint spawning (use spawn_actor instead)

See tool docstrings for full argument details, or use get_mcp_help(tool_name="...").
"""
from mcp.server.fastmcp import FastMCP
from editor_tools.editor_tools import register_editor_tools

mcp = FastMCP(
    "editorMCP",
    description="Editor tools for Unreal via MCP"
)

register_editor_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio') 