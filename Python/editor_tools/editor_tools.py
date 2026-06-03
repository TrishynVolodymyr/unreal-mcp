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
    save_asset as save_asset_impl,
    create_render_target as create_render_target_impl,
    set_actor_transform as set_actor_transform_impl,
    get_actor_properties as get_actor_properties_impl,
    set_actor_property as set_actor_property_impl,
    set_light_property as set_light_property_impl,
    focus_viewport as focus_viewport_impl,
    spawn_blueprint_actor as spawn_blueprint_actor_impl,
    get_level_metadata as get_level_metadata_impl,
    batch_delete_actors as batch_delete_actors_impl,
    batch_spawn_actors as batch_spawn_actors_impl,
    import_static_mesh as import_static_mesh_impl,
    import_texture as import_texture_impl,
    create_level as create_level_impl,
    set_level_world_settings as set_level_world_settings_impl
)
from utils.mcp_help import get_help_registry, get_mcp_help as get_mcp_help_impl
from utils.unreal_connection_utils import send_unreal_command

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
                - "ContactShadowLength": Length of contact shadows (float, 0-1, screen-space by default)
                - "ContactShadowLengthInWS": Whether ContactShadowLength is in world space (boolean)
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
    def save_asset(ctx: Context, asset_path: str) -> Dict[str, Any]:
        """
        Save an asset's package to disk (persist a dirty / in-memory asset).

        Writes the asset's .uasset to disk regardless of dirty state. Use this when an
        asset was created or modified in memory but not yet persisted — e.g. a texture
        built in C++ via NewObject + MarkPackageDirty, or an asset edited via MCP that
        left the package dirty — so commits/reloads don't hit a stale or missing file.

        Args:
            asset_path: Full path to the asset to save (e.g., "/Game/Tests/VT_ColonyOccupancy")

        Returns:
            Dict containing:
            - success: Whether the save succeeded
            - asset_path: The saved asset path
            - message: Status message or error description

        Examples:
            # Persist a runtime-generated volume texture
            save_asset(asset_path="/Game/Tests/VT_ColonyOccupancy")

            # Persist a material edited via MCP
            save_asset(asset_path="/Game/Materials/M_MyMaterial")
        """
        return save_asset_impl(ctx, asset_path)

    @mcp.tool()
    def create_level(
        ctx: Context,
        level_name: str,
        save_path: str,
        template: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new level (umap) at a content path.

        Creates an empty level or clones from a template. The new level becomes the
        active editor world. Saves the level to disk after creation.

        Args:
            level_name: Name of the level without extension (e.g., "TestLevel_Building")
            save_path: Folder path under /Game where the level will be saved (e.g., "/Game/Tests")
            template: Optional content path to an existing level to clone from.
                      Empty (default) creates a blank empty level.

        Returns:
            Dict containing:
            - success: Whether creation succeeded
            - level_path: Full asset path of the new level (e.g., "/Game/Tests/TestLevel_Building")
            - template: The template used (empty if blank)
            - saved: Whether the initial save succeeded

        Side effect:
            The newly created level becomes the editor's active world.

        Examples:
            # Create an empty test level
            create_level(level_name="TestLevel_Building", save_path="/Game/Tests")

            # Clone from an existing level template
            create_level(
                level_name="TestLevel_Combat",
                save_path="/Game/Tests",
                template="/Game/Templates/CombatBase"
            )
        """
        return create_level_impl(ctx, level_name, save_path, template)

    @mcp.tool()
    def set_level_world_settings(
        ctx: Context,
        level_path: str,
        game_mode: str = None
    ) -> Dict[str, Any]:
        """
        Modify a level's World Settings. Currently supports GameMode override.

        Edits the level's WorldSettings in place and saves the map package. Does NOT
        switch the active editor world (the level is loaded as an object, not opened as
        the current map) — this avoids the editor map-load leak-check fatal that could
        occur when switching into a World Partition / malformed level. A missing or
        corrupt target map returns an error instead of crashing the editor.

        Args:
            level_path: Full asset path of the level (e.g., "/Game/Tests/TestLevel_Building")
            game_mode: GameMode override class path. Examples:
                       - "/Game/TopDown/Blueprints/BP_TopDownGameMode" (Blueprint)
                       - "/Script/SimPrototype.GameModeMain" (C++ class)
                       - "" (empty string) clears the override (uses project default)
                       - None (default): leaves GameMode unchanged

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - level_path: The level that was modified
            - game_mode_resolved: Full class path of the resolved GameMode (empty if cleared)
            - changed: Whether any change was applied

        Examples:
            # Set a Blueprint GameMode
            set_level_world_settings(
                level_path="/Game/Tests/TestLevel_Building",
                game_mode="/Game/TopDown/Blueprints/BP_TopDownGameMode"
            )

            # Clear the GameMode override (use project default)
            set_level_world_settings(
                level_path="/Game/Tests/TestLevel_Building",
                game_mode=""
            )
        """
        return set_level_world_settings_impl(ctx, level_path, game_mode)

    @mcp.tool()
    def open_level(
        ctx: Context,
        level_path: str
    ) -> Dict[str, Any]:
        """
        Open an EXISTING level (umap) as the ACTIVE editor world, by content path.

        The missing counterpart to create_level (makes a NEW level active) and
        set_level_world_settings (edits a level WITHOUT switching worlds): this switches the
        editor's current world to an already-existing level — e.g. to leave the default
        startup map and work on a test level.

        SAFETY: World Partition levels are REFUSED. Switching the active editor world into a
        WP map via code can trigger the engine map-load leak-check fatal (EditorServer.cpp
        "World Memory Leaks"). The target is first loaded as an object (no world switch) to
        detect WP; only a non-WP level is then opened. Open WP maps via the editor UI
        (File > Open Level).

        Args:
            level_path: Package path of the level (e.g., "/Game/Tests/TestLevel_ColonyView")

        Returns:
            Dict containing:
            - success: Whether the level was opened
            - level_path: The level path
            - was_world_partition: True when refused because the target is a WP level
            - error: Message when refused/failed

        Side effect:
            A non-WP target becomes the editor's active world.

        Examples:
            open_level(level_path="/Game/Tests/TestLevel_ColonyView")
        """
        return send_unreal_command("open_level", {"level_path": level_path})

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

    @mcp.tool()
    def import_static_mesh(
        ctx: Context,
        source_file_path: str,
        asset_name: str,
        folder_path: str = "/Game/Meshes",
        import_materials: bool = False
    ) -> Dict[str, Any]:
        """
        Import a static mesh from disk into the Unreal Engine project.

        Supports FBX, OBJ. Matches the editor's default Import behaviour: a multi-mesh
        ("pack"/"set") FBX is SPLIT into one Static Mesh per mesh, each named after its FBX
        node, at the FBX's own scale. (It is NOT merged into a single combined mesh.)

        Args:
            source_file_path: Absolute path to the mesh file on disk
                (e.g., "E:/meshes/SM_Rock_Boulder_01.fbx")
            asset_name: Used ONLY when the FBX contains a single mesh — that one asset is
                renamed to asset_name. For a multi-mesh FBX it is ignored (the per-node FBX
                names are used), so the props land as e.g. .../axe, .../jug, .../flask.
            folder_path: Destination Content FOLDER for the imported asset(s) (default: "/Game/Meshes")
            import_materials: Whether to import materials/textures from the source file (default: False)

        Returns:
            Dictionary containing:
            - success: Whether the import was successful
            - count: Number of static meshes created
            - folder: Destination folder
            - meshes: List of {name, path, vertex_count, triangle_count} — one per created mesh
            - path/name/vertex_count/triangle_count: mirror of the FIRST mesh (back-compat)
            - message: Success/error message

        Example:
            # A pack FBX -> separate props (alchemical_table, axe, jug, ...) under the folder
            import_static_mesh(
                source_file_path="C:/Downloads/alchemical_set.fbx",
                asset_name="alchemical_set",
                folder_path="/Game/Meshes/Props"
            )
        """
        return import_static_mesh_impl(ctx, source_file_path, asset_name, folder_path, import_materials)

    @mcp.tool()
    def import_texture(
        ctx: Context,
        source_file_path: str,
        asset_name: str,
        folder_path: str = "/Game/Textures",
        compression_settings: str = "Default",
        srgb: bool = True,
        preserve_alpha: bool = True
    ) -> Dict[str, Any]:
        """
        Import a texture from disk into the Unreal Engine project.

        Supports PNG, TGA, TIF, JPEG, EXR, HDR, BMP formats.

        Args:
            source_file_path: Absolute path to the texture file on disk
                (e.g., "E:/textures/T_Grass_Short_01.png")
            asset_name: Name for the imported Texture asset (e.g., "T_Grass_Short_01")
            folder_path: Content folder path for the imported asset (default: "/Game/Textures")
            compression_settings: Texture compression type (default: "Default")
                - "Default": Standard color texture (BC1/BC3)
                - "Normalmap": Normal map compression (BC5), forces sRGB off
                - "Masks": Mask/utility texture, forces sRGB off
                - "Grayscale": Grayscale texture, forces sRGB off
                - "HDR": High dynamic range (RGBA16F)
                - "Alpha": Alpha-only compression
            srgb: Whether texture uses sRGB color space (default: True).
                Automatically disabled for Normalmap, Masks, and Grayscale compression.
            preserve_alpha: Whether to preserve the alpha channel (default: True).
                Set to False for opaque textures to save memory.

        Returns:
            Dictionary containing:
            - success: Whether the import was successful
            - path: Full asset path of the imported texture
            - name: Name of the imported asset
            - size_x: Texture width in pixels
            - size_y: Texture height in pixels
            - has_alpha: Whether the texture has an alpha channel
            - message: Success/error message

        Example:
            import_texture(
                source_file_path="E:/project/Content/Environment/GroundCover/T_Grass_Short_01.png",
                asset_name="T_Grass_Short_01",
                folder_path="/Game/Environment/GroundCover",
                compression_settings="Default",
                srgb=True,
                preserve_alpha=True
            )
        """
        return import_texture_impl(ctx, source_file_path, asset_name, folder_path, compression_settings, srgb, preserve_alpha)

    @mcp.tool()
    def get_performance_stats(ctx: Context) -> Dict[str, Any]:
        """
        Get real-time performance statistics from Unreal Engine.

        Returns FPS, frame time, GPU time, memory usage, draw calls, and triangle count.
        Use this to profile performance and identify bottlenecks.

        Returns:
            Dictionary containing:
            - fps_current: Current frame FPS
            - fps_average: Smoothed average FPS
            - frame_time_ms: Current frame time in milliseconds
            - gpu_time_ms: GPU frame time in milliseconds
            - draw_calls: Number of draw calls this frame
            - primitives_drawn: Number of triangles/primitives drawn
            - memory: Object with used_physical_mb, available_physical_mb, peak_used_physical_mb
            - message: Human-readable summary

        Example:
            get_performance_stats()
        """
        return send_unreal_command("get_performance_stats", {})

    @mcp.tool()
    def execute_console_command(
        ctx: Context,
        command: str
    ) -> Dict[str, Any]:
        """
        Execute an Unreal Engine console command and capture output.

        Useful for toggling stat overlays, changing CVars, and runtime tweaks.

        Args:
            command: Console command to execute (e.g., "stat fps", "stat unit",
                    "stat scenerendering", "r.SetRes 1920x1080",
                    "t.MaxFPS 60", "stat memory")

        Returns:
            Dictionary containing:
            - success: Whether command was executed
            - command: The command that was run
            - output: Captured text output (may be empty for toggle commands)
            - message: Status message

        Examples:
            execute_console_command(command="stat fps")
            execute_console_command(command="stat unit")
            execute_console_command(command="r.ScreenPercentage 50")
        """
        return send_unreal_command("execute_console_command", {"command": command})

    @mcp.tool()
    def set_viewport_camera(
        ctx: Context,
        location: list = None,
        look_at: list = None,
        rotation: list = None,
        fov: float = None
    ) -> Dict[str, Any]:
        """
        Set the editor perspective viewport camera (to frame the level for screenshots).

        Lets you position the camera and then capture_viewport_screenshot from a chosen
        angle without a human dragging the viewport — including orbiting a point to
        check view-dependent rendering.

        Args:
            location: [X, Y, Z] camera world position. If omitted, keeps current.
            look_at: [X, Y, Z] point to aim the camera at. Overrides rotation — ideal
                     for orbiting: vary location around a target, aim at the target.
            rotation: [Pitch, Yaw, Roll] explicit rotation (used only if look_at omitted).
            fov: optional horizontal field of view in degrees.

        Returns:
            success, plus the applied location and rotation.

        Examples:
            # Look at a cavity centered at (0,0,300) from the +X side, slightly above
            set_viewport_camera(location=[2500, 0, 1200], look_at=[0, 0, 300])
            # Orbit 90 degrees: move to +Y side, still aiming at the cavity
            set_viewport_camera(location=[0, 2500, 1200], look_at=[0, 0, 300])
        """
        params = {}
        if location is not None:
            params["location"] = location
        if look_at is not None:
            params["look_at"] = look_at
        if rotation is not None:
            params["rotation"] = rotation
        if fov is not None:
            params["fov"] = fov
        return send_unreal_command("set_viewport_camera", params)

    @mcp.tool()
    def get_gpu_stats(ctx: Context) -> Dict[str, Any]:
        """
        Get per-pass GPU profiler breakdown.

        Returns detailed GPU timing for each render pass: BasePass, Shadows,
        Translucency, PostProcessing, etc. Sorted by cost (highest first).

        First call enables GPU stat collection. Call again for actual data.

        Returns:
            Dictionary containing:
            - total_gpu_ms: Total GPU frame time
            - passes: Array of {name, avg_ms, min_ms, max_ms} per render pass
            - pass_count: Number of active passes
            - message: Human-readable summary of top passes

        Example:
            get_gpu_stats()
        """
        return send_unreal_command("get_gpu_stats", {})

    @mcp.tool()
    def get_scene_breakdown(
        ctx: Context,
        mesh_filter: str = "",
        max_results: int = 50
    ) -> Dict[str, Any]:
        """
        Get per-mesh breakdown of scene rendering cost.

        Iterates all StaticMesh/ISM/HISM components, aggregates by mesh,
        sorted by total triangle cost (instances * LOD0 tris). Essential for
        identifying what's eating FPS.

        Args:
            mesh_filter: Optional filter by mesh name (case-insensitive contains)
            max_results: Max meshes to return (default: 50)

        Returns:
            Dictionary containing:
            - meshes: Array sorted by cost, each with:
                - mesh: Mesh asset name
                - instances: Total instance count (ISM) or component count (SMC)
                - lod0_tris: Triangle count at LOD0
                - total_tris: instances * lod0_tris
                - shadow_casters: Components with CastShadow=true
                - nanite: Whether Nanite is enabled
                - instanced: Whether using ISM/HISM
            - total_instances, total_tris_lod0, total_shadow_casters
            - message: Human-readable summary

        Example:
            get_scene_breakdown()
            get_scene_breakdown(mesh_filter="Grass")
        """
        params = {}
        if mesh_filter:
            params["mesh_filter"] = mesh_filter
        if max_results != 50:
            params["max_results"] = max_results
        return send_unreal_command("get_scene_breakdown", params)

    @mcp.tool()
    def get_rendering_stats(ctx: Context) -> Dict[str, Any]:
        """
        Get focused rendering diagnostics: draw call categories, VRAM, visibility/culling stats.

        Complements get_gpu_stats (GPU pass timing) and get_scene_breakdown (mesh instances).
        Shows VRAM usage, total draw calls / triangles, and initviews visibility data.

        Returns:
            Dictionary containing:
            - draw_calls: Total draw calls
            - primitives_drawn: Total triangles
            - gpu_time_ms: GPU frame time
            - draw_call_categories: Array (empty in UE 5.7+ due to RHI_NEW_GPU_PROFILER)
            - visibility: Culling/visibility stats (auto-enabled on first call):
                - processed_primitives: Total primitives processed
                - frustum_culled_primitives: Primitives culled by frustum
                - occluded_primitives: Primitives culled by occlusion
                - statically_occluded: Statically occluded primitives
                - visible_static_mesh_elements: Visible static mesh draw elements
                - visible_dynamic_primitives: Visible dynamic primitives
                - occlusion_queries: Number of occlusion queries
            - vram: {dedicated_vram_mb, budget_mb, streaming_mb, non_streaming_mb}
            - message: Human-readable summary

        Example:
            get_rendering_stats()
        """
        return send_unreal_command("get_rendering_stats", {})

    @mcp.tool()
    def get_mesh_draw_stats(ctx: Context, mesh_filter: str = None) -> Dict[str, Any]:
        """
        Get per-mesh draw count breakdown per render pass.

        Triggers mesh draw command stats dump and returns per-resource/material
        breakdown showing how many primitives, vertices, and instances each mesh
        contributes to each render pass (BasePass, ShadowDepths, etc.).

        First call triggers stats collection. Call again after one rendered frame for data.

        Args:
            mesh_filter: Optional filter by mesh/resource name (case-insensitive contains)

        Returns:
            Dictionary containing:
            - entries: Array of {pass, resource, category, material, visible_primitives,
              visible_vertices, visible_instances, lod_index, total_instances,
              total_primitives, total_vertices}
            - total_entries: Total CSV rows (after filter)
            - shown_entries: Entries returned (max 100)
            - total_visible_primitives/vertices/instances: Aggregated totals
            - mesh_filter: Applied filter (if any)
            - message: Human-readable summary

        Example:
            get_mesh_draw_stats()
            get_mesh_draw_stats(mesh_filter="Grass")
        """
        params = {}
        if mesh_filter:
            params["mesh_filter"] = mesh_filter
        return send_unreal_command("get_mesh_draw_stats", params)

    @mcp.tool()
    def start_pie(ctx: Context) -> Dict[str, Any]:
        """
        Start a Play In Editor (PIE) session.

        Queues PIE start for the next engine tick. The session begins
        asynchronously — it won't be active the instant this returns.

        Returns:
            Dictionary containing:
            - success: Whether the request was accepted
            - already_running: True if PIE was already active
            - message: Status message

        Example:
            start_pie()
        """
        return send_unreal_command("start_pie", {})

    @mcp.tool()
    def stop_pie(ctx: Context) -> Dict[str, Any]:
        """
        Stop the current Play In Editor (PIE) session.

        Returns:
            Dictionary containing:
            - success: Whether the request was accepted
            - already_stopped: True if no PIE session was running
            - message: Status message

        Example:
            stop_pie()
        """
        return send_unreal_command("stop_pie", {})

    # Register all tools with the help system
    _help_registry.register(spawn_actor, category="actors")
    _help_registry.register(delete_actor, category="actors")
    _help_registry.register(delete_asset, category="assets")
    _help_registry.register(save_asset, category="assets")
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
    _help_registry.register(import_static_mesh, category="assets")
    _help_registry.register(import_texture, category="assets")
    _help_registry.register(get_performance_stats, category="profiling")
    _help_registry.register(execute_console_command, category="profiling")
    _help_registry.register(get_gpu_stats, category="profiling")
    _help_registry.register(get_scene_breakdown, category="profiling")
    _help_registry.register(get_rendering_stats, category="profiling")
    _help_registry.register(get_mesh_draw_stats, category="profiling")
    _help_registry.register(start_pie, category="profiling")
    _help_registry.register(stop_pie, category="profiling")

    logger.info("Editor tools registered successfully")
