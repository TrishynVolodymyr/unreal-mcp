"""
Editor Tools for Unreal MCP.

This module provides tools for controlling the Unreal Editor viewport and other editor functionality.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context
from utils.editor.editor_operations import (
    spawn_actor as spawn_actor_impl,
    delete_actor as delete_actor_impl,
    delete_asset as delete_asset_impl,
    create_render_target as create_render_target_impl,
    set_actor_transform as set_actor_transform_impl,
    get_actor_properties as get_actor_properties_impl,
    set_actor_property as set_actor_property_impl,
    set_light_property as set_light_property_impl,
    focus_viewport as focus_viewport_impl,
    spawn_blueprint_actor as spawn_blueprint_actor_impl,
    get_level_metadata as get_level_metadata_impl,
    batch_delete_actors as batch_delete_actors_impl,
    batch_spawn_actors as batch_spawn_actors_impl
)
from utils.mcp_help import get_help_registry, get_mcp_help as get_mcp_help_impl

# Get logger
logger = logging.getLogger("UnrealMCP")

# Get the help registry for this server
_help_registry = get_help_registry()

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""

    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = None,
        rotation: List[float] = None,
        scale: List[float] = None,
        # StaticMeshActor
        mesh_path: str = None,
        # TextRenderActor
        text_content: str = None,
        text_size: float = None,
        text_color: List[float] = None,
        text_halign: str = None,
        text_valign: str = None,
        # Volumes
        box_extent: List[float] = None,
        sphere_radius: float = None,
        # PlayerStart
        player_start_tag: str = None,
        # DecalActor
        decal_size: List[float] = None,
        decal_material: str = None
    ) -> Dict[str, Any]:
        """
        Spawn actors in the Unreal Editor level with comprehensive type support.

        TYPE RESOLUTION (in order):
            1. Friendly names: StaticMeshActor, TriggerBox, PlayerStart, etc.
            2. Blueprint paths: "Blueprint:/Game/Path/To/BP_Name"
            3. Native class paths: "Class:/Script/Engine.TriggerBox"
            4. Direct paths: "/Game/Path/To/BP_Name" (tries Blueprint then Class)

        SUPPORTED FRIENDLY NAMES:
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
            /Engine/BasicShapes/Cube
            /Engine/BasicShapes/Sphere
            /Engine/BasicShapes/Cylinder
            /Engine/BasicShapes/Cone
            /Engine/BasicShapes/Plane

        Args:
            name: The name to give the new actor (must be unique)
            type: Actor type (see TYPE RESOLUTION above)
            location: [x, y, z] world location
            rotation: [pitch, yaw, roll] rotation in degrees
            scale: [x, y, z] scale factor

            mesh_path: (StaticMeshActor) Path to mesh asset
            text_content: (TextRenderActor) Text to display
            text_size: (TextRenderActor) World size of text
            text_color: (TextRenderActor) [R, G, B] or [R, G, B, A] color (0-1 range)
            text_halign: (TextRenderActor) "Left", "Center", or "Right"
            text_valign: (TextRenderActor) "Top", "Center", or "Bottom"
            box_extent: (TriggerBox, etc.) [x, y, z] half-extents
            sphere_radius: (TriggerSphere) Sphere radius
            player_start_tag: (PlayerStart) Tag for spawn selection
            decal_size: (DecalActor) [x, y, z] decal dimensions
            decal_material: (DecalActor) Path to decal material

        Returns:
            Dict containing the created actor's properties

        Examples:
            # Platform using BasicShapes cube (scaled to 10x10x0.5 meters)
            spawn_actor(name="Platform1", type="StaticMeshActor",
                       mesh_path="/Engine/BasicShapes/Cube",
                       location=[0, 0, 0], scale=[10, 10, 0.5])

            # Text label
            spawn_actor(name="Label1", type="TextRenderActor",
                       text_content="JUMP HERE", text_size=50,
                       text_color=[1.0, 1.0, 0.0], location=[0, 0, 200])

            # Trigger zone for checkpoint
            spawn_actor(name="Checkpoint1", type="TriggerBox",
                       box_extent=[200, 200, 100], location=[500, 0, 50])

            # Player spawn point
            spawn_actor(name="PlayerSpawn", type="PlayerStart",
                       location=[0, 0, 100])

            # NavMesh bounds for AI
            spawn_actor(name="NavBounds1", type="NavMeshBoundsVolume",
                       scale=[50, 50, 10])

            # Custom Blueprint actor
            spawn_actor(name="MyPortal", type="Blueprint:/Game/Testing/BP_TestPortal",
                       location=[1000, 0, 0])

            # ANY native class via Class: prefix
            spawn_actor(name="CustomVolume", type="Class:/Script/Engine.CameraBlockingVolume")

        Note:
            This unified function supports both built-in actors and Blueprints.
            Use the "Blueprint:" prefix for custom Blueprint actors.
        """
        return spawn_actor_impl(
            ctx, name, type, location, rotation, scale,
            mesh_path,
            text_content, text_size, text_color, text_halign, text_valign,
            box_extent, sphere_radius,
            player_start_tag,
            decal_size, decal_material
        )
    
    @mcp.tool()
    def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
        """
        Delete an actor by name.
        
        Args:
            name: Name of the actor to delete
            
        Returns:
            Dict containing response information
            
        Examples:
            # Delete an actor named "MyCube"
            delete_actor(name="MyCube")
        """
        return delete_actor_impl(ctx, name)
    
    @mcp.tool()
    def set_actor_transform(
        ctx: Context,
        name: str,
        location: List[float] = None,
        rotation: List[float] = None,
        scale: List[float] = None
    ) -> Dict[str, Any]:
        """
        Set the transform of an actor.
        
        Args:
            name: Name of the actor
            location: Optional [X, Y, Z] position
            rotation: Optional [Pitch, Yaw, Roll] rotation in degrees
            scale: Optional [X, Y, Z] scale
            
        Returns:
            Dict containing response information
            
        Examples:
            # Move an actor named "MyCube" to a new position
            set_actor_transform(name="MyCube", location=[100, 200, 50])
            
            # Rotate an actor named "MyCube" 45 degrees around Z axis
            set_actor_transform(name="MyCube", rotation=[0, 0, 45])
            
            # Scale an actor named "MyCube" to be twice as big
            set_actor_transform(name="MyCube", scale=[2.0, 2.0, 2.0])
            
            # Move, rotate, and scale an actor all at once
            set_actor_transform(
                name="MyCube", 
                location=[100, 200, 50],
                rotation=[0, 0, 45],
                scale=[2.0, 2.0, 2.0]
            )
        """
        return set_actor_transform_impl(ctx, name, location, rotation, scale)
    
    @mcp.tool()
    def get_actor_properties(ctx: Context, name: str) -> Dict[str, Any]:
        """
        Get all properties of an actor.
        
        Args:
            name: Name of the actor
            
        Returns:
            Dict containing actor properties
            
        Examples:
            # Get properties of an actor named "MyCube"
            props = get_actor_properties(name="MyCube")
            
            # Print location
            print(props["transform"]["location"])
        """
        return get_actor_properties_impl(ctx, name)

    @mcp.tool()
    def set_actor_property(
        ctx: Context,
        name: str,
        property_name: str,
        property_value
    ) -> Dict[str, Any]:
        """
        Set a property on an actor.
        
        Note: Currently there's a limitation with the property_value parameter type.
        Please contact the MCP system administrator for proper usage.
        
        Args:
            name: Name of the actor
            property_name: Name of the property to set
            property_value: Value to set the property to. Different property types accept different value formats:
                - For boolean properties: True/False
                - For numeric properties: int or float values
                - For string properties: string values
                - For color properties: [R, G, B, A] list (0-255 values)
                - For vector properties: [X, Y, Z] list
                - For enum properties: String name of the enum value or integer index
            
        Returns:
            Dict containing response information
            
        Examples:
            # Change the color of a light
            set_actor_property(
                name="MyPointLight",
                property_name="LightColor",
                property_value=[255, 0, 0, 255]  # RGBA
            )
            
            # Change the mobility of an actor
            set_actor_property(
                name="MyCube",
                property_name="Mobility",
                property_value="Movable"  # "Static", "Stationary", or "Movable"
            )
            
            # Set a boolean property
            set_actor_property(
                name="MyCube",
                property_name="bHidden",
                property_value=True
            )
            
            # Set a numeric property
            set_actor_property(
                name="PointLightTest",
                property_name="Intensity",
                property_value=5000.0
            )
        """
        return set_actor_property_impl(ctx, name, property_name, property_value)

    @mcp.tool()
    def set_light_property(
        ctx: Context,
        name: str,
        property_name: str,
        property_value
    ) -> Dict[str, Any]:
        """
        Set a property on a light component.
        
        This function accesses the LightComponent of a light actor and sets properties on it.
        
        Args:
            name: Name of the light actor
            property_name: Property to set, one of:
                - "Intensity": Brightness of the light (float)
                - "LightColor": Color of the light (array [R, G, B] with values 0-1)
                - "AttenuationRadius": How far the light reaches (float)
                - "SourceRadius": Size of the light source (float)
                - "SoftSourceRadius": Size of the soft light source border (float)
                - "CastShadows": Whether the light casts shadows (boolean)
            property_value: Value to set the property to
            
        Returns:
            Dict containing response information
            
        Examples:
            # Set light intensity
            set_light_property(
                name="MyPointLight",
                property_name="Intensity",
                property_value=5000.0
            )
            
            # Set light color to red
            set_light_property(
                name="MyPointLight",
                property_name="LightColor",
                property_value=[1.0, 0.0, 0.0]
            )
            
            # Set light attenuation radius
            set_light_property(
                name="MyPointLight",
                property_name="AttenuationRadius",
                property_value=500.0
            )
        """
        return set_light_property_impl(ctx, name, property_name, property_value)

    # @mcp.tool() commented out because it's buggy
    def focus_viewport(
        ctx: Context,
        target: str = None,
        location: List[float] = None,
        distance: float = 1000.0,
        orientation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Focus the viewport on a specific actor or location.
        
        Args:
            target: Name of the actor to focus on (if provided, location is ignored)
            location: [X, Y, Z] coordinates to focus on (used if target is None)
            distance: Distance from the target/location
            orientation: Optional [Pitch, Yaw, Roll] for the viewport camera
            
        Returns:
            Response from Unreal Engine
            
        Examples:
            # Focus on an actor named "MyCube"
            focus_viewport(target="MyCube")
            
            # Focus on a specific location
            focus_viewport(location=[100, 200, 50])
            
            # Focus on an actor from a specific orientation
            focus_viewport(target="MyCube", orientation=[45, 0, 0])
        """
        return focus_viewport_impl(ctx, target, location, distance, orientation)

    @mcp.tool()
    def spawn_blueprint_actor(
        ctx: Context,
        blueprint_name: str,
        actor_name: str,
        location: List[float] = None,
        rotation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Spawn an actor from a Blueprint class in the current level.
        This function is used for spawning custom Blueprint actors that can have:
        - Custom components and hierarchies
        - Visual scripting logic
        - Custom variables and events
        - Complex behaviors and interactions
        
        Args:
            blueprint_name: Path to the Blueprint to spawn from. Can be:
                          - Absolute path: "/Game/Blueprints/BP_Character"
                          - Relative path: "BP_Character" (will be prefixed with "/Game/Blueprints/")
            actor_name: Name to give the spawned actor instance (must be unique)
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the spawned actor's properties
            
        Examples:
            # Spawn a blueprint actor with a relative path (looks in /Game/Blueprints/)
            spawn_blueprint_actor(
                blueprint_name="BP_Character",
                actor_name="MyCharacter_1"
            )
            
            # Spawn a blueprint actor with a full path
            spawn_blueprint_actor(
                blueprint_name="/Game/Characters/BP_Enemy",
                actor_name="Enemy_1",
                location=[100, 200, 50],
                rotation=[0, 45, 0]
            )
            
            # For ThirdPerson template project character
            spawn_blueprint_actor(
                blueprint_name="/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter",
                actor_name="Player1",
                location=[0, 0, 100]
            )
            
        Note:
            This function requires the Blueprint to exist and be properly compiled.
            For spawning basic actor types (lights, static meshes, etc.), use spawn_actor() instead.
        """
        return spawn_blueprint_actor_impl(ctx, blueprint_name, actor_name, location, rotation)

    @mcp.tool()
    def get_level_metadata(
        ctx: Context,
        fields: List[str] = None,
        actor_filter: str = None
    ) -> Dict[str, Any]:
        """
        Get level metadata with selective field querying.

        This tool consolidates multiple level query tools into one flexible interface.

        Args:
            fields: List of metadata fields to retrieve. Options:
                - "actors": All actors in the level (default)
                - "*": All available fields
            actor_filter: Optional pattern for actor name filtering (supports wildcards *)
                         When provided, only actors matching the pattern are returned.

        Returns:
            Dictionary with requested level metadata

        Examples:
            # Get all actors in the level
            get_level_metadata()

            # Get all actors (explicit)
            get_level_metadata(fields=["actors"])

            # Get actors matching a pattern
            get_level_metadata(actor_filter="*PointLight*")

            # Get all actors with "Player" in the name
            get_level_metadata(fields=["actors"], actor_filter="Player*")
        """
        return get_level_metadata_impl(ctx, fields, actor_filter)

    @mcp.tool()
    def get_mcp_help(
        ctx: Context,
        tool_name: str = None,
        include_full_docstring: bool = False
    ) -> Dict[str, Any]:
        """
        Get help and documentation for available MCP tools.

        This tool provides self-documentation for the MCP server, allowing you to
        discover available tools and their usage.

        Args:
            tool_name: Name of a specific tool to get detailed help for.
                      If not provided, returns a list of all available tools.
            include_full_docstring: If True, includes the complete raw docstring
                                   in addition to parsed sections.

        Returns:
            If tool_name is None:
                Dict with "tools" (list of tool names), "categories" (tools by category),
                and "usage" hint.

            If tool_name is specified:
                Dict with:
                - name: Tool name
                - description: What the tool does
                - parameters: Dict of parameter info (type, required, default, description)
                - returns: Description of return value
                - examples: List of usage examples
                - sections: Any custom documentation sections (e.g., SUPPORTED FRIENDLY NAMES)
                - notes: Additional notes

        Examples:
            # List all available tools
            get_mcp_help()

            # Get detailed help for spawn_actor
            get_mcp_help(tool_name="spawn_actor")

            # Get help with full docstring included
            get_mcp_help(tool_name="spawn_actor", include_full_docstring=True)
        """
        return get_mcp_help_impl(tool_name, include_full_docstring)

    @mcp.tool()
    def delete_actors(ctx: Context, names: List[str]) -> Dict[str, Any]:
        """
        Delete multiple actors by name in a single operation.

        This is more efficient than calling delete_actor() multiple times,
        especially for large batch operations like clearing test actors or
        resetting level sections.

        Args:
            names: List of actor names to delete

        Returns:
            Dict containing:
            - results: Array of per-actor results with name, success, deleted, and error fields
            - total: Total number of actors processed
            - succeeded: Number of successfully deleted actors
            - failed: Number of failed deletions
            - success: Overall operation success (true if command executed)

        Examples:
            # Delete multiple test actors
            delete_actors(names=["TestCube1", "TestCube2", "TestCube3"])

            # Delete actors matching a pattern (get names first)
            metadata = get_level_metadata(actor_filter="*Test*")
            names = [actor["name"] for actor in metadata.get("actors", [])]
            delete_actors(names=names)

            # Clean up a spawned arena
            arena_actors = ["Platform1", "Platform2", "TriggerZone1", "SpawnPoint1"]
            result = delete_actors(names=arena_actors)
            print(f"Deleted {result['succeeded']} of {result['total']} actors")
        """
        return batch_delete_actors_impl(ctx, names)

    @mcp.tool()
    def spawn_actors(ctx: Context, actors: List[Dict[str, Any]]) -> Dict[str, Any]:
        """
        Spawn multiple actors in a single operation.

        This is more efficient than calling spawn_actor() multiple times,
        especially for level construction, arena setup, or procedural generation.

        Each actor configuration supports the same parameters as spawn_actor():
        - Required: name, type
        - Transform: location, rotation, scale
        - Type-specific: mesh_path, text_content, box_extent, etc.

        Args:
            actors: List of actor configurations. Each dict should contain:
                - name: Unique actor name (required)
                - type: Actor type - friendly name, Blueprint:, or Class: path (required)
                - location: [X, Y, Z] world location (optional, default [0,0,0])
                - rotation: [Pitch, Yaw, Roll] in degrees (optional, default [0,0,0])
                - scale: [X, Y, Z] scale (optional, default [1,1,1])
                - mesh_path: (StaticMeshActor) Path to mesh
                - text_content: (TextRenderActor) Text to display
                - text_size: (TextRenderActor) World text size
                - text_color: (TextRenderActor) [R,G,B] or [R,G,B,A] (0-1 range)
                - box_extent: (TriggerBox, etc.) [X,Y,Z] half-extents
                - sphere_radius: (TriggerSphere) Radius
                - player_start_tag: (PlayerStart) Tag for spawn selection
                - decal_size: (DecalActor) [X,Y,Z] dimensions
                - decal_material: (DecalActor) Material path

        Returns:
            Dict containing:
            - results: Array of per-actor results with name, success, actor details, or error
            - total: Total number of actors processed
            - succeeded: Number of successfully spawned actors
            - failed: Number of failed spawns
            - success: Overall operation success

        Examples:
            # Spawn a simple arena with platforms and checkpoints
            spawn_actors(actors=[
                {
                    "name": "Platform1",
                    "type": "StaticMeshActor",
                    "mesh_path": "/Engine/BasicShapes/Cube",
                    "location": [0, 0, 0],
                    "scale": [10, 10, 0.5]
                },
                {
                    "name": "Platform2",
                    "type": "StaticMeshActor",
                    "mesh_path": "/Engine/BasicShapes/Cube",
                    "location": [1000, 0, 200],
                    "scale": [5, 5, 0.5]
                },
                {
                    "name": "Checkpoint1",
                    "type": "TriggerBox",
                    "location": [500, 0, 50],
                    "box_extent": [100, 100, 200]
                },
                {
                    "name": "StartLabel",
                    "type": "TextRenderActor",
                    "location": [0, 0, 100],
                    "text_content": "START",
                    "text_size": 50,
                    "text_color": [0, 1, 0]
                }
            ])

            # Spawn multiple lights in a row
            lights = [
                {"name": f"Light{i}", "type": "PointLight", "location": [i * 200, 0, 300]}
                for i in range(5)
            ]
            spawn_actors(actors=lights)

            # Spawn Blueprint actors in batch
            spawn_actors(actors=[
                {"name": "Portal1", "type": "Blueprint:/Game/Actors/BP_Portal", "location": [0, 0, 0]},
                {"name": "Portal2", "type": "Blueprint:/Game/Actors/BP_Portal", "location": [1000, 0, 0]}
            ])
        """
        return batch_spawn_actors_impl(ctx, actors)

    @mcp.tool()
    def delete_asset(ctx: Context, asset_path: str) -> Dict[str, Any]:
        """
        Delete an asset from the Content Browser.

        This deletes the asset from the project entirely, not just from the level.
        Use this to clean up test Blueprints, DataTables, Materials, and other assets.

        Args:
            asset_path: Full path to the asset to delete (e.g., "/Game/Blueprints/BP_TestActor")

        Returns:
            Dict containing:
            - success: Whether the deletion was successful
            - message: Status message or error description

        Examples:
            # Delete a Blueprint asset
            delete_asset(asset_path="/Game/Blueprints/BP_TestActor")

            # Delete a DataTable
            delete_asset(asset_path="/Game/Data/DT_TestItems")

            # Delete a Material
            delete_asset(asset_path="/Game/Materials/M_TestMaterial")

            # Delete a Widget Blueprint
            delete_asset(asset_path="/Game/UI/WBP_TestWidget")
        """
        return delete_asset_impl(ctx, asset_path)

    @mcp.tool()
    def create_render_target(
        ctx: Context,
        name: str,
        folder_path: str = "/Game",
        width: int = 256,
        height: int = 256
    ) -> Dict[str, Any]:
        """
        Create a TextureRenderTarget2D asset in the Content Browser.

        Used for Scene Capture, runtime rendering to texture, minimap cameras, portrait captures, etc.

        Args:
            name: Name of the render target asset (e.g., "RT_Portrait")
            folder_path: Folder path in Content Browser (default: "/Game")
            width: Texture width in pixels (1-4096, default: 256)
            height: Texture height in pixels (1-4096, default: 256)

        Returns:
            Dict containing:
            - success: Whether creation was successful
            - asset_path: Full path to the created asset
            - width/height: Dimensions

        Examples:
            create_render_target(name="RT_Portrait", folder_path="/Game/Rendering", width=256, height=256)
            create_render_target(name="RT_Minimap", folder_path="/Game/Rendering", width=512, height=512)
        """
        return create_render_target_impl(ctx, name, folder_path, width, height)

    # Register all tools with the help system
    _help_registry.register(spawn_actor, category="actors")
    _help_registry.register(delete_actor, category="actors")
    _help_registry.register(delete_asset, category="assets")
    _help_registry.register(create_render_target, category="assets")
    _help_registry.register(set_actor_transform, category="actors")
    _help_registry.register(get_actor_properties, category="actors")
    _help_registry.register(set_actor_property, category="actors")
    _help_registry.register(set_light_property, category="lighting")
    _help_registry.register(spawn_blueprint_actor, category="actors")
    _help_registry.register(get_level_metadata, category="level")
    _help_registry.register(get_mcp_help, category="help")
    _help_registry.register(delete_actors, category="actors")
    _help_registry.register(spawn_actors, category="actors")

    logger.info("Editor tools registered successfully")
