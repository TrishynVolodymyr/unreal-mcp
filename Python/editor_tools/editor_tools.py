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
    set_actor_transform as set_actor_transform_impl,
    get_actor_properties as get_actor_properties_impl,
    set_actor_property as set_actor_property_impl,
    set_light_property as set_light_property_impl,
    focus_viewport as focus_viewport_impl,
    spawn_blueprint_actor as spawn_blueprint_actor_impl,
    get_level_metadata as get_level_metadata_impl
)

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""

    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = None,
        rotation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Spawn an actor in the current level.

        Args:
            name: Unique name for the spawned actor
            type: Actor type - built-in ("StaticMeshActor"|"PointLight"|"SpotLight"|"DirectionalLight"|"CameraActor")
                  or Blueprint path ("/Game/Blueprints/BP_Enemy" or "BP_Enemy")

        Example:
            spawn_actor(name="MyLight", type="PointLight")
            spawn_actor(name="Enemy1", type="/Game/Characters/BP_Enemy", location=[100, 200, 50])
        """
        # Route to appropriate command based on type format
        if "/" in type or type.startswith("BP_"):
            # Blueprint path - use spawn_blueprint_actor command
            return spawn_blueprint_actor_impl(ctx, type, name, location, rotation)
        else:
            # Built-in type - use spawn_actor command
            return spawn_actor_impl(ctx, name, type, location, rotation)
    
    @mcp.tool()
    def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
        """
        Delete an actor by name.

        Args:
            name: Name of the actor to delete

        Example:
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

        Example:
            set_actor_transform(name="MyCube", location=[100, 200, 50])
            set_actor_transform(name="MyCube", rotation=[0, 0, 45])
            set_actor_transform(name="MyCube", scale=[2.0, 2.0, 2.0])
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

        Example:
            props = get_actor_properties(name="MyCube")
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

        Example:
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
    def get_level_metadata(
        ctx: Context,
        fields: List[str] = None,
        actor_filter: str = None
    ) -> Dict[str, Any]:
        """
        Get level metadata with selective field querying.

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

    logger.info("Editor tools registered successfully")
