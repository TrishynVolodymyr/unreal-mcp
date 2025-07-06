"""
Editor Tools for Unreal MCP.

This module provides tools for controlling the Unreal Editor viewport and other editor functionality.
"""

import logging
from typing import Dict, List, Any, Optional, Union
from mcp.server.fastmcp import FastMCP, Context
from utils.editor.editor_operations import (
    get_actors_in_level as get_actors_in_level_impl,
    find_actors_by_name as find_actors_by_name_impl,
    spawn_actor as spawn_actor_impl,
    delete_actor as delete_actor_impl,
    set_actor_transform as set_actor_transform_impl,
    get_actor_properties as get_actor_properties_impl,
    set_actor_property as set_actor_property_impl,
    set_light_property as set_light_property_impl,
    focus_viewport as focus_viewport_impl,
    spawn_blueprint_actor as spawn_blueprint_actor_impl
)

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""
    
    @mcp.tool()
    def get_actors_in_level(ctx: Context) -> List[Dict[str, Any]]:
        """
        Get a list of all actors in the current level.
        
        Returns:
            List of actors in the current level with their properties
            
        Examples:
            actors = get_actors_in_level()
            # Print names of all actors in level
            for actor in actors:
                print(actor["name"])
        """
        return get_actors_in_level_impl(ctx)

    @mcp.tool()
    def find_actors_by_name(ctx: Context, pattern: str) -> List[str]:
        """
        Find actors by name pattern.
        
        Args:
            pattern: Name pattern to search for (supports wildcards using *)
            
        Returns:
            List of actor names matching the pattern
            
        Examples:
            # Find all Point Light actors
            lights = find_actors_by_name("*PointLight*")
            
            # Find a specific actor
            player = find_actors_by_name("Player*")
        """
        return find_actors_by_name_impl(ctx, pattern)
    
    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = None,
        rotation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Create a new basic Unreal Engine actor in the current level.
        This function is used for spawning built-in Unreal Engine actor types only.
        
        Args:
            name: The name to give the new actor (must be unique)
            type: The type of built-in actor to create. Supported types:
                  - StaticMeshActor: Basic static mesh actor
                  - PointLight: Point light source
                  - SpotLight: Spot light source
                  - DirectionalLight: Directional light source
                  - CameraActor: Camera actor
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the created actor's properties
            
        Examples:
            # Spawn a point light at origin
            spawn_actor(name="MyLight", type="PointLight")
            
            # Spawn a static mesh at a specific location
            spawn_actor(name="MyCube", type="StaticMeshActor", 
                       location=[100, 200, 50], 
                       rotation=[0, 45, 0])
                       
        Note:
            This function is for basic actor types only. For spawning custom Blueprint 
            actors with custom logic and components, use spawn_blueprint_actor() instead.
        """
        return spawn_actor_impl(ctx, name, type, location, rotation)
    
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
        property_value: str
    ) -> Dict[str, Any]:
        """
        Set a property on a light component.

        This function accesses the LightComponent of a light actor and sets properties on it.

        Args:
            name: Name of the light actor
            property_name: Property to set, one of:
                - "Intensity": Brightness of the light (float)
                - "LightColor": Color of the light (comma-separated R,G,B values 0-1)
                - "AttenuationRadius": How far the light reaches (float)
                - "SourceRadius": Size of the light source (float)
                - "SoftSourceRadius": Size of the soft light source border (float)
                - "CastShadows": Whether the light casts shadows (boolean)
            property_value: Value to set the property to (as string)
                - For numeric values: "5000.0"
                - For colors: "1.0,0.0,0.0" (R,G,B comma-separated)
                - For booleans: "true" or "false"

        Returns:
            Dict containing response information

        Examples:
            # Set light intensity
            set_light_property(
                name="MyPointLight",
                property_name="Intensity",
                property_value="5000.0"
            )

            # Set light color to red
            set_light_property(
                name="MyPointLight",
                property_name="LightColor",
                property_value="1.0,0.0,0.0"
            )

            # Set light attenuation radius
            set_light_property(
                name="MyPointLight",
                property_name="AttenuationRadius",
                property_value="500.0"
            )

            # Enable shadows
            set_light_property(
                name="MyPointLight",
                property_name="CastShadows",
                property_value="true"
            )
        """
        return set_light_property_impl(ctx, name, property_name, property_value)

    @mcp.tool()
    def set_light_property_fixed(
        ctx: Context,
        name: str,
        property_name: str,
        property_value: Union[str, int, float, bool, List[float]]
    ) -> Dict[str, Any]:
        """
        Set a property on a light component (fixed version).

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
            set_light_property_fixed(
                name="MyPointLight",
                property_name="Intensity",
                property_value=5000.0
            )

            # Set light color to red
            set_light_property_fixed(
                name="MyPointLight",
                property_name="LightColor",
                property_value=[1.0, 0.0, 0.0]
            )

            # Set light attenuation radius
            set_light_property_fixed(
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

    logger.info("Editor tools registered successfully")
