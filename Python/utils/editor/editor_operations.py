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
    """Implementation for spawning an actor in the editor with extended parameters."""
    params = {
        "name": name,
        "type": type
    }

    # Transform parameters
    if location is not None:
        params["location"] = location
    if rotation is not None:
        params["rotation"] = rotation
    if scale is not None:
        params["scale"] = scale

    # StaticMeshActor parameters
    if mesh_path is not None:
        params["mesh_path"] = mesh_path

    # TextRenderActor parameters
    if text_content is not None:
        params["text_content"] = text_content
    if text_size is not None:
        params["text_size"] = text_size
    if text_color is not None:
        params["text_color"] = text_color
    if text_halign is not None:
        params["text_halign"] = text_halign
    if text_valign is not None:
        params["text_valign"] = text_valign

    # Volume parameters
    if box_extent is not None:
        params["box_extent"] = box_extent
    if sphere_radius is not None:
        params["sphere_radius"] = sphere_radius

    # PlayerStart parameters
    if player_start_tag is not None:
        params["player_start_tag"] = player_start_tag

    # DecalActor parameters
    if decal_size is not None:
        params["decal_size"] = decal_size
    if decal_material is not None:
        params["decal_material"] = decal_material

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
        property_value: Value to set the property to

    Returns:
        Dict containing response from Unreal
    """
    # Create the parameters for the light-specific command
    params = {
        "name": name,
        "property_name": property_name,
        "property_value": property_value
    }

    return send_unreal_command("set_light_property", params)


def get_level_metadata(
    ctx: Context,
    fields: List[str] = None,
    actor_filter: str = None
) -> Dict[str, Any]:
    """Get level metadata with selective field querying.

    Consolidates: get_actors_in_level, find_actors_by_name

    Args:
        ctx: The MCP context
        fields: List of fields to include. Options:
            - "actors": All actors in the level
            - "*": All fields (default if None)
        actor_filter: Optional pattern for actor name filtering (supports wildcards *)

    Returns:
        Dictionary with requested level metadata
    """
    params = {}

    if fields:
        params["fields"] = fields

    if actor_filter:
        params["actor_filter"] = actor_filter

    logger.info(f"Getting level metadata with fields: {fields}, filter: {actor_filter}")
    return send_unreal_command("get_level_metadata", params)


def batch_delete_actors(ctx: Context, names: List[str]) -> Dict[str, Any]:
    """Delete multiple actors by name in a single operation.

    Args:
        ctx: The MCP context
        names: List of actor names to delete

    Returns:
        Dict containing results for each actor:
        {
            "results": [
                {"name": "Actor1", "success": true, "deleted": true},
                {"name": "Actor2", "success": false, "error": "Actor not found"}
            ],
            "total": 2,
            "succeeded": 1,
            "failed": 1,
            "success": true
        }
    """
    params = {"names": names}
    logger.info(f"Batch deleting {len(names)} actors: {names}")
    return send_unreal_command("batch_delete_actors", params)


def batch_spawn_actors(ctx: Context, actors: List[Dict[str, Any]]) -> Dict[str, Any]:
    """Spawn multiple actors in a single operation.

    Args:
        ctx: The MCP context
        actors: List of actor configurations, each containing:
            - name: Actor name (required)
            - type: Actor type (required)
            - location: [X, Y, Z] spawn location (optional)
            - rotation: [Pitch, Yaw, Roll] spawn rotation (optional)
            - scale: [X, Y, Z] scale (optional)
            - (type-specific params like mesh_path, text_content, box_extent, etc.)

    Returns:
        Dict containing results for each actor:
        {
            "results": [
                {"name": "Actor1", "success": true, "actor": {...}},
                {"name": "Actor2", "success": false, "error": "..."}
            ],
            "total": 2,
            "succeeded": 1,
            "failed": 1,
            "success": true
        }
    """
    params = {"actors": actors}
    logger.info(f"Batch spawning {len(actors)} actors")
    return send_unreal_command("batch_spawn_actors", params)