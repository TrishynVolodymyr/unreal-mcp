"""
Editor Operations for Unreal MCP.

This module provides utilities for working with the Unreal Editor.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def get_editor_state(ctx: Context) -> Dict[str, Any]:
    """Implementation for getting the current state of the Unreal Editor."""
    return send_unreal_command("get_editor_state", {})

def get_selected_actors(ctx: Context) -> List[Dict[str, Any]]:
    """Implementation for getting information about currently selected actors in the editor."""
    return send_unreal_command("get_selected_actors", {})

def select_actor(ctx: Context, name: str) -> Dict[str, Any]:
    """Implementation for selecting an actor in the editor."""
    params = {"name": name}
    return send_unreal_command("select_actor", params)

def deselect_all_actors(ctx: Context) -> Dict[str, Any]:
    """Implementation for deselecting all actors in the editor."""
    return send_unreal_command("deselect_all_actors", {})

def spawn_actor(
    ctx: Context,
    name: str,
    type: str,
    location: List[float] = None,
    rotation: List[float] = None,
    scale: List[float] = None
) -> Dict[str, Any]:
    """Implementation for spawning an actor in the editor."""
    params = {
        "name": name,
        "type": type
    }
    
    if location is not None:
        params["location"] = location
        
    if rotation is not None:
        params["rotation"] = rotation
        
    if scale is not None:
        params["scale"] = scale
    
    return send_unreal_command("spawn_actor", params)

def delete_selected_actors(ctx: Context) -> Dict[str, Any]:
    """Implementation for deleting all currently selected actors."""
    return send_unreal_command("delete_selected_actors", {})

def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
    """Implementation for deleting a specific actor by name."""
    params = {"name": name}
    return send_unreal_command("delete_actor", params)

def set_actor_property(
    ctx: Context,
    name: str,
    property_name: str,
    property_value
) -> Dict[str, Any]:
    """Implementation for setting a property on an actor."""
    params = {
        "name": name,
        "property_name": property_name,
        "property_value": property_value
    }
    return send_unreal_command("set_actor_property", params)

def get_actor_property(
    ctx: Context,
    name: str,
    property_name: str
) -> Dict[str, Any]:
    """Implementation for getting the value of a property on an actor."""
    params = {
        "name": name,
        "property_name": property_name
    }
    return send_unreal_command("get_actor_property", params)

def get_actor_properties(ctx: Context, name: str) -> Dict[str, Any]:
    """Implementation for getting all properties of an actor."""
    params = {"name": name}
    return send_unreal_command("get_actor_properties", params)

def set_actor_transform(
    ctx: Context,
    name: str,
    location: List[float] = None,
    rotation: List[float] = None,
    scale: List[float] = None
) -> Dict[str, Any]:
    """Implementation for setting the transform of an actor."""
    params = {"name": name}
    
    if location is not None:
        params["location"] = location
    
    if rotation is not None:
        params["rotation"] = rotation
        
    if scale is not None:
        params["scale"] = scale
    
    return send_unreal_command("set_actor_transform", params)

def get_actor_transform(ctx: Context, name: str) -> Dict[str, Any]:
    """Implementation for getting the transform of an actor."""
    params = {"name": name}
    return send_unreal_command("get_actor_transform", params)

def get_all_actors(ctx: Context, actor_class: str = None) -> List[Dict[str, Any]]:
    """Implementation for getting information about all actors in the level."""
    params = {}
    
    if actor_class:
        params["actor_class"] = actor_class
    
    return send_unreal_command("get_all_actors", params)

def get_actor_by_name(ctx: Context, name: str) -> Dict[str, Any]:
    """Implementation for getting information about a specific actor by name."""
    params = {"name": name}
    return send_unreal_command("get_actor_by_name", params)

def play_in_editor(ctx: Context, play_mode: str = "PlayInViewport") -> Dict[str, Any]:
    """Implementation for starting play-in-editor."""
    params = {"play_mode": play_mode}
    return send_unreal_command("play_in_editor", params)

def stop_play_in_editor(ctx: Context) -> Dict[str, Any]:
    """Implementation for stopping play-in-editor."""
    return send_unreal_command("stop_play_in_editor", {})

def get_content_browser_selection(ctx: Context) -> List[Dict[str, Any]]:
    """Implementation for getting the currently selected assets in the Content Browser."""
    return send_unreal_command("get_content_browser_selection", {})

def select_asset(ctx: Context, asset_path: str) -> Dict[str, Any]:
    """Implementation for selecting an asset in the Content Browser."""
    params = {"asset_path": asset_path}
    return send_unreal_command("select_asset", params)

def duplicate_asset(
    ctx: Context,
    source_asset_path: str,
    destination_name: str = None,
    destination_path: str = None
) -> Dict[str, Any]:
    """Implementation for duplicating an asset."""
    params = {"source_asset_path": source_asset_path}
    
    if destination_name:
        params["destination_name"] = destination_name
        
    if destination_path:
        params["destination_path"] = destination_path
    
    return send_unreal_command("duplicate_asset", params)

def delete_asset(ctx: Context, asset_path: str) -> Dict[str, Any]:
    """Implementation for deleting an asset."""
    params = {"asset_path": asset_path}
    return send_unreal_command("delete_asset", params)

def create_folder(ctx: Context, folder_path: str) -> Dict[str, Any]:
    """Implementation for creating a new folder in the Content Browser."""
    params = {"folder_path": folder_path}
    return send_unreal_command("create_folder", params)

def import_asset(
    ctx: Context,
    file_path: str,
    destination_path: str,
    options: Dict[str, Any] = None
) -> Dict[str, Any]:
    """Implementation for importing an external file as an asset."""
    params = {
        "file_path": file_path,
        "destination_path": destination_path
    }
    
    if options:
        params["options"] = options
    
    return send_unreal_command("import_asset", params)

def find_actors_by_name(ctx: Context, pattern: str) -> List[Dict[str, Any]]:
    """Implementation for finding actors by name pattern."""
    params = {"pattern": pattern}
    return send_unreal_command("find_actors_by_name", params)

def get_actors_in_level(ctx: Context) -> List[Dict[str, Any]]:
    """Implementation for getting all actors in the current level.
    
    Returns a list of dictionaries, each containing details about an actor in the current level.
    
    Returns:
        List[Dict[str, Any]]: A list of actors with their properties
        
    Examples:
        # Get all actors in the current level
        actors = get_actors_in_level()
        
        # Print names of all actors
        for actor in actors:
            print(actor["name"])
    """
    return send_unreal_command("get_actors_in_level", {})

def spawn_blueprint_actor(
    ctx: Context,
    blueprint_name: str,
    actor_name: str,
    location: List[float] = None,
    rotation: List[float] = None
) -> Dict[str, Any]:
    """Implementation for spawning an actor from a Blueprint."""
    # Process the blueprint path - if doesn't start with /Game/, assume it's in /Game/Blueprints/
    if not blueprint_name.startswith("/Game/"):
        blueprint_name = f"/Game/Blueprints/{blueprint_name}"
    
    params = {
        "blueprint_name": blueprint_name,
        "actor_name": actor_name
    }
    
    if location is not None:
        params["location"] = location
        
    if rotation is not None:
        params["rotation"] = rotation
        
    return send_unreal_command("spawn_blueprint_actor", params)

def focus_viewport(
    ctx: Context,
    target: str = None,
    location: List[float] = None,
    distance: float = 1000.0,
    orientation: List[float] = None
) -> Dict[str, Any]:
    """Implementation for focusing the viewport on a specific actor or location."""
    params = {"distance": distance}
    
    if target is not None:
        params["target"] = target
        
    if location is not None:
        params["location"] = location
        
    if orientation is not None:
        params["orientation"] = orientation
        
    return send_unreal_command("focus_viewport", params)

def set_light_property(
    ctx: Context,
    name: str,
    property_name: str,
    property_value
) -> Dict[str, Any]:
    """Implementation for setting properties on a light actor.

    This function specifically handles properties of light actors by using
    specific property names that correspond to light component methods.

    Args:
        name: Name of the light actor
        property_name: Name of the property to set (Intensity, LightColor, AttenuationRadius)
        property_value: Value to set the property to (various types)

    Returns:
        Dict containing response from Unreal
    """
    # Convert property_value to appropriate string format based on property type
    formatted_value = str(property_value)

    # Handle special formatting for different property types
    if property_name == "LightColor":
        # Handle color values - can be list/array or string
        if isinstance(property_value, (list, tuple)):
            # Convert array to comma-separated string
            if len(property_value) >= 3:
                formatted_value = ",".join(str(float(x)) for x in property_value[:3])
            else:
                raise ValueError(f"LightColor requires at least 3 values (R,G,B), got {len(property_value)}")
        elif isinstance(property_value, str):
            # Try to parse as JSON array first
            try:
                import json
                color_array = json.loads(property_value)
                if isinstance(color_array, list) and len(color_array) >= 3:
                    # Convert array to comma-separated string
                    formatted_value = ",".join(str(float(x)) for x in color_array[:3])
                else:
                    # Assume it's already in comma-separated format
                    formatted_value = property_value
            except (json.JSONDecodeError, ValueError):
                # Assume it's already in comma-separated format
                formatted_value = property_value
    elif property_name in ["Intensity", "AttenuationRadius", "SourceRadius", "SoftSourceRadius"]:
        # Numeric properties - ensure they're formatted as strings
        formatted_value = str(float(property_value))
    elif property_name == "CastShadows":
        # Boolean property - convert to string
        if isinstance(property_value, bool):
            formatted_value = "true" if property_value else "false"
        else:
            formatted_value = str(property_value).lower()

    # Create the parameters for the light-specific command
    params = {
        "name": name,
        "property_name": property_name,
        "property_value": formatted_value
    }

    return send_unreal_command("set_light_property", params)