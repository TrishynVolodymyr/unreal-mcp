"""
Blueprint Operations for Unreal MCP.

This module provides utility functions for working with Blueprints in Unreal Engine.
"""

import logging
from typing import Dict, List, Any, Tuple
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def list_all_blueprints(ctx: Context) -> List[Dict[str, Any]]:
    """Implementation for getting a list of all blueprints in the project."""
    return send_unreal_command("list_all_blueprints", {})

def search_blueprints(ctx: Context, query: str) -> List[Dict[str, Any]]:
    """Implementation for searching blueprints matching a query string."""
    params = {"query": query}
    return send_unreal_command("search_blueprints", params)

def get_blueprint_details(ctx: Context, blueprint_path: str) -> Dict[str, Any]:
    """Implementation for getting detailed information about a specific blueprint."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("get_blueprint_details", params)

def create_blueprint_from_actor(
    ctx: Context,
    actor_name: str,
    blueprint_name: str,
    folder_path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """Implementation for creating a new blueprint from an existing actor."""
    params = {
        "actor_name": actor_name,
        "blueprint_name": blueprint_name,
        "folder_path": folder_path
    }
    return send_unreal_command("create_blueprint_from_actor", params)

def create_blank_blueprint(
    ctx: Context,
    blueprint_name: str,
    parent_class: str = "Actor",
    folder_path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """Implementation for creating a new blank blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "parent_class": parent_class,
        "folder_path": folder_path
    }
    return send_unreal_command("create_blank_blueprint", params)

def compile_blueprint(ctx: Context, blueprint_name: str) -> Dict[str, Any]:
    """Implementation for compiling a blueprint."""
    # The C++ side will handle finding the path from the name, 
    # assuming it expects the 'blueprint_name' key.
    params = {"blueprint_name": blueprint_name} 
    return send_unreal_command("compile_blueprint", params)

def save_blueprint(ctx: Context, blueprint_path: str) -> Dict[str, Any]:
    """Implementation for saving a blueprint to disk."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("save_blueprint", params)

def get_blueprint_variables(ctx: Context, blueprint_path: str) -> List[Dict[str, Any]]:
    """Implementation for getting a list of variables defined in a blueprint."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("get_blueprint_variables", params)

def add_blueprint_variable(
    ctx: Context,
    blueprint_name: str,
    var_name: str,
    var_type: str,
    default_value: Any = None,
    is_instance_editable: bool = True,
    is_blueprint_read_only: bool = False,
    category: str = "Default"
) -> Dict[str, Any]:
    """Implementation for adding a new variable to a blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "variable_name": var_name,
        "variable_type": var_type,
        "is_exposed": is_instance_editable,
        "is_blueprint_read_only": is_blueprint_read_only,
        "category": category
    }
    
    # Only include default value if provided
    if default_value is not None:
        params["default_value"] = default_value
    
    return send_unreal_command("add_blueprint_variable", params)

def get_blueprint_functions(ctx: Context, blueprint_path: str) -> List[Dict[str, Any]]:
    """Implementation for getting a list of functions defined in a blueprint."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("get_blueprint_functions", params)

def add_blueprint_function(
    ctx: Context,
    blueprint_path: str,
    function_name: str,
    inputs: List[Dict[str, Any]] = None,
    outputs: List[Dict[str, Any]] = None,
    pure: bool = False,
    static: bool = False,
    category: str = "Default"
) -> Dict[str, Any]:
    """Implementation for adding a new function to a blueprint."""
    if inputs is None:
        inputs = []
    if outputs is None:
        outputs = []
    
    params = {
        "blueprint_path": blueprint_path,
        "function_name": function_name,
        "inputs": inputs,
        "outputs": outputs,
        "pure": pure,
        "static": static,
        "category": category
    }
    
    return send_unreal_command("add_blueprint_function", params)

def connect_graph_nodes(
    ctx: Context,
    blueprint_path: str,
    function_name: str,
    source_node_name: str,
    source_pin_name: str,
    target_node_name: str,
    target_pin_name: str
) -> Dict[str, Any]:
    """Implementation for connecting two nodes in a blueprint graph."""
    params = {
        "blueprint_path": blueprint_path,
        "function_name": function_name,
        "source_node_name": source_node_name,
        "source_pin_name": source_pin_name,
        "target_node_name": target_node_name,
        "target_pin_name": target_pin_name
    }
    
    return send_unreal_command("connect_graph_nodes", params)

def create_blueprint(
    ctx: Context,
    name: str,
    parent_class: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """Implementation for creating a new Blueprint class."""
    params = {
        "name": name,
        "parent_class": parent_class
    }
    
    if folder_path:
        params["folder_path"] = folder_path
        
    return send_unreal_command("create_blueprint", params)

def add_component_to_blueprint(
    ctx: Context,
    blueprint_name: str,
    component_type: str,
    component_name: str,
    location: List[float] = None,
    rotation: List[float] = None,
    scale: List[float] = None,
    component_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """Implementation for adding a component to a Blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "component_type": component_type,
        "component_name": component_name
    }
    
    if location is not None:
        params["location"] = location
        
    if rotation is not None:
        params["rotation"] = rotation
        
    if scale is not None:
        params["scale"] = scale
        
    if component_properties is not None:
        params["component_properties"] = component_properties
        
    return send_unreal_command("add_component_to_blueprint", params)

def set_static_mesh_properties(
    ctx: Context,
    blueprint_name: str,
    component_name: str,
    static_mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Implementation for setting static mesh properties on a StaticMeshComponent."""
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "static_mesh": static_mesh
    }
    
    return send_unreal_command("set_static_mesh_properties", params)

def set_component_property(
    ctx: Context,
    blueprint_name: str,
    component_name: str,
    **kwargs
) -> Dict[str, Any]:
    """
    Set one or more properties on a component in a Blueprint.

    Args:
        ctx: MCP context
        blueprint_name: Name of the target Blueprint
        component_name: Name of the component
        kwargs: Properties to set. You can pass properties as direct keyword arguments (recommended),
            or as a single dict using kwargs={...}. If double-wrapped (kwargs={'kwargs': {...}}),
            the function will automatically flatten it for convenience. This matches the widget property setter pattern.

    Returns:
        Response indicating success or failure for each property.

    Example:
        # Preferred usage (direct keyword arguments):
        set_component_property(ctx, "MyActor", "Mesh", StaticMesh="/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube", Mobility="Movable")

        # Also supported (dict):
        set_component_property(ctx, "MyActor", "Mesh", kwargs={"StaticMesh": "/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube", "Mobility": "Movable"})

    """
    # Debug: Log all incoming arguments
    logger.info(f"[DEBUG] set_component_property called with: blueprint_name={blueprint_name}, component_name={component_name}, kwargs={kwargs}")

    # Flatten if kwargs is double-wrapped (i.e., kwargs={'kwargs': {...}})
    if (
        len(kwargs) == 1 and
        'kwargs' in kwargs and
        isinstance(kwargs['kwargs'], dict)
    ):
        logger.info("[DEBUG] Flattening double-wrapped kwargs in set_component_property")
        kwargs = kwargs['kwargs']

    # Argument validation
    if not blueprint_name or not isinstance(blueprint_name, str):
        logger.error("[ERROR] 'blueprint_name' is required and must be a string.")
        raise ValueError("'blueprint_name' is required and must be a string.")
    if not component_name or not isinstance(component_name, str):
        logger.error("[ERROR] 'component_name' is required and must be a string.")
        raise ValueError("'component_name' is required and must be a string.")
    if not kwargs or not isinstance(kwargs, dict):
        logger.error("[ERROR] At least one property must be provided as a keyword argument.")
        raise ValueError("At least one property must be provided as a keyword argument.")

    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "kwargs": kwargs
    }
    logger.info(f"[DEBUG] Sending set_component_property params: {params}")
    return send_unreal_command("set_component_property", params)

def set_physics_properties(
    ctx: Context,
    blueprint_name: str,
    component_name: str,
    simulate_physics: bool = True,
    gravity_enabled: bool = True,
    mass: float = 1.0,
    linear_damping: float = 0.01,
    angular_damping: float = 0.0
) -> Dict[str, Any]:
    """Implementation for setting physics properties on a component."""
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "simulate_physics": simulate_physics,
        "gravity_enabled": gravity_enabled,
        "mass": mass,
        "linear_damping": linear_damping,
        "angular_damping": angular_damping
    }
    
    return send_unreal_command("set_physics_properties", params)

def set_blueprint_property(
    ctx: Context,
    blueprint_name: str,
    property_name: str,
    property_value: Any
) -> Dict[str, Any]:
    """Implementation for setting a property on a Blueprint class default object."""
    params = {
        "blueprint_name": blueprint_name,
        "property_name": property_name,
        "property_value": property_value
    }
    
    return send_unreal_command("set_blueprint_property", params)

def set_pawn_properties(
    ctx: Context,
    blueprint_name: str,
    auto_possess_player: str = "",
    use_controller_rotation_yaw: bool = None,
    use_controller_rotation_pitch: bool = None,
    use_controller_rotation_roll: bool = None,
    can_be_damaged: bool = None
) -> Dict[str, Any]:
    """Implementation for setting common Pawn properties on a Blueprint."""
    params = {
        "blueprint_name": blueprint_name
    }
    
    if auto_possess_player:
        params["auto_possess_player"] = auto_possess_player
        
    if use_controller_rotation_yaw is not None:
        params["use_controller_rotation_yaw"] = use_controller_rotation_yaw
        
    if use_controller_rotation_pitch is not None:
        params["use_controller_rotation_pitch"] = use_controller_rotation_pitch
        
    if use_controller_rotation_roll is not None:
        params["use_controller_rotation_roll"] = use_controller_rotation_roll
        
    if can_be_damaged is not None:
        params["can_be_damaged"] = can_be_damaged
        
    return send_unreal_command("set_pawn_properties", params)

def add_interface_to_blueprint(
    ctx: Context,
    blueprint_name: str,
    interface_name: str
) -> Dict[str, Any]:
    """Implementation for adding an interface to a blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "interface_name": interface_name
    }
    return send_unreal_command("add_interface_to_blueprint", params)

def create_blueprint_interface(
    ctx: Context,
    name: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """Implementation for creating a Blueprint Interface asset."""
    params = {
        "name": name,
        "folder_path": folder_path
    }
    return send_unreal_command("create_blueprint_interface", params)

def create_custom_blueprint_function(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    inputs: List[Dict[str, str]] = None,
    outputs: List[Dict[str, str]] = None,
    is_pure: bool = False,
    is_const: bool = False,
    access_specifier: str = "Public",
    category: str = "Default"
) -> Dict[str, Any]:
    """Implementation for creating a custom user-defined function in a blueprint.
    
    Args:
        blueprint_name: Name of the target Blueprint
        function_name: Name of the custom function to create
        inputs: List of input parameters, each with 'name' and 'type' keys
        outputs: List of output parameters, each with 'name' and 'type' keys  
        is_pure: Whether the function is pure (no execution pins)
        is_const: Whether the function is const
        access_specifier: Access level ("Public", "Protected", "Private")
        category: Category for organization in the functions list
        
    Returns:
        Dictionary containing success status and function information
    """
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name,
        "is_pure": is_pure,
        "is_const": is_const,
        "access_specifier": access_specifier,
        "category": category
    }
    
    if inputs is not None:
        params["inputs"] = inputs
        
    if outputs is not None:
        params["outputs"] = outputs
        
    return send_unreal_command("create_custom_blueprint_function", params)

def call_blueprint_function(
    ctx: Context,
    target_name: str,
    function_name: str,
    string_params: List[str] = None
) -> Dict[str, Any]:
    """Implementation for calling a BlueprintCallable function by name.

    Args:
        target_name: Name of the target object to call the function on
        function_name: Name of the BlueprintCallable function to execute
        string_params: List of string parameters to pass to the function

    Returns:
        Dictionary containing the function call result and success status
    """
    params = {
        "target_name": target_name,
        "function_name": function_name,
        "string_params": string_params or []
    }

    return send_unreal_command("call_blueprint_function", params)

def get_blueprint_metadata(
    ctx: Context,
    blueprint_name: str,
    fields: List[str],
    graph_name: str = None,
    node_type: str = None,
    event_type: str = None,
    detail_level: str = None
) -> Dict[str, Any]:
    """Implementation for getting comprehensive metadata about a Blueprint.

    IMPORTANT: At least one field must be specified. When using "graph_nodes",
    graph_name is REQUIRED to limit response size.

    Args:
        blueprint_name: Name or path of the Blueprint
        fields: REQUIRED list of metadata fields. At least one must be specified.
                Options: "parent_class", "interfaces", "variables", "functions",
                        "components", "graphs", "status", "metadata",
                        "timelines", "asset_info", "orphaned_nodes", "graph_nodes"
        graph_name: REQUIRED when using "graph_nodes" field. Specifies which graph.
        node_type: Optional filter for "graph_nodes": "Event", "Function", "Variable", "Comment".
        event_type: Optional filter for "graph_nodes" when node_type="Event".
        detail_level: Optional detail level for "graph_nodes" field. Options:
                     - "summary": Node IDs and titles only (minimal output)
                     - "flow": Node IDs, titles, and exec pin connections only (DEFAULT)
                     - "full": Everything including all data pin connections and default values

    Returns:
        Dictionary containing requested metadata fields
    """
    # Validate fields parameter
    if not fields or len(fields) == 0:
        return {
            "success": False,
            "error": "Missing required 'fields' parameter. Specify at least one field."
        }

    # Validate graph_name is provided when graph_nodes is requested
    if "graph_nodes" in fields and not graph_name:
        return {
            "success": False,
            "error": "When requesting 'graph_nodes' field, 'graph_name' parameter is required. Use fields=['graphs'] first to discover available graph names."
        }

    params = {
        "blueprint_name": blueprint_name,
        "fields": fields
    }

    if graph_name is not None:
        params["graph_name"] = graph_name

    if node_type is not None:
        params["node_type"] = node_type

    if event_type is not None:
        params["event_type"] = event_type

    if detail_level is not None:
        params["detail_level"] = detail_level

    return send_unreal_command("get_blueprint_metadata", params)


def modify_blueprint_function_properties(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    is_pure: bool = None,
    is_const: bool = None,
    access_specifier: str = None,
    category: str = None
) -> Dict[str, Any]:
    """Implementation for modifying properties of an existing Blueprint function.

    Args:
        blueprint_name: Name or path of the Blueprint
        function_name: Name of the function to modify
        is_pure: If set, changes whether the function is pure (no side effects)
        is_const: If set, changes whether the function is const
        access_specifier: If set, changes access level ("Public", "Protected", "Private")
        category: If set, changes the function's category for organization

    Returns:
        Dictionary containing success status and modified properties
    """
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name
    }

    if is_pure is not None:
        params["is_pure"] = is_pure

    if is_const is not None:
        params["is_const"] = is_const

    if access_specifier is not None:
        params["access_specifier"] = access_specifier

    if category is not None:
        params["category"] = category

    return send_unreal_command("modify_blueprint_function_properties", params)


def capture_blueprint_graph_screenshot(
    ctx: Context,
    blueprint_name: str,
    graph_name: str = "EventGraph",
    width: int = 1280,
    height: int = 720,
    format: str = "png"
) -> Dict[str, Any]:
    """Implementation for capturing a screenshot of a Blueprint graph.

    This renders the Blueprint graph to an image and returns it as base64-encoded data.
    The image can be used by AI to visually understand the node layout without needing
    to query detailed node metadata, significantly reducing context/token usage.

    Args:
        blueprint_name: Name or path of the Blueprint to capture
        graph_name: Name of the graph to capture (default: "EventGraph").
                   Use get_blueprint_metadata with fields=["graphs"] to discover available graphs.
        width: Screenshot width in pixels (default: 1280, range: 1-8192)
        height: Screenshot height in pixels (default: 720, range: 1-8192)
        format: Image format - "png" (default) or "jpg"

    Returns:
        Dictionary containing:
        - success: Whether the screenshot was captured
        - image_base64: Base64-encoded image data (viewable by AI)
        - width: Actual image width
        - height: Actual image height
        - format: Image format used
        - image_size_bytes: Size of the compressed image
        - blueprint_name: Name of the Blueprint
        - graph_name: Name of the captured graph
        - node_count: Number of nodes in the graph

    Examples:
        # Capture EventGraph
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="BP_MyActor"
        )

        # Capture a specific function graph
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="BP_MyActor",
            graph_name="ToggleInventory"
        )

        # Capture at higher resolution
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="WBP_InventorySlot",
            graph_name="HandleMouseDown",
            width=1920,
            height=1080
        )

        # Capture as JPEG (smaller file size)
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="BP_DialogueComponent",
            graph_name="EventGraph",
            format="jpg"
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "graph_name": graph_name,
        "width": width,
        "height": height,
        "format": format
    }

    return send_unreal_command("capture_blueprint_graph_screenshot", params)


# =============================================================================
# GAS (Gameplay Ability System) Pipeline Tools
# =============================================================================

def create_gameplay_ability(
    ctx: Context,
    name: str,
    parent_class: str = "BaseGameplayAbility",
    folder_path: str = "/Game/Abilities",
    ability_type: str = "instant",
    ability_tags: List[str] = None,
    cancel_abilities_with_tag: List[str] = None,
    block_abilities_with_tag: List[str] = None,
    activation_owned_tags: List[str] = None,
    activation_required_tags: List[str] = None,
    activation_blocked_tags: List[str] = None,
    source_required_tags: List[str] = None,
    source_blocked_tags: List[str] = None,
    target_required_tags: List[str] = None,
    target_blocked_tags: List[str] = None,
    cost_effect_class: str = None,
    cooldown_effect_class: str = None,
    cooldown_duration: float = None,
    replication_policy: str = "ReplicateYes",
    instancing_policy: str = "InstancedPerActor"
) -> Dict[str, Any]:
    """Implementation for creating a GameplayAbility Blueprint with GAS configuration.

    This is a high-level scaffolding tool that:
    1. Creates a Blueprint with the specified GAS ability parent class
    2. Configures common GAS properties (tags, costs, cooldowns, policies)
    3. Returns the created ability info for further customization

    Args:
        name: Name of the ability (e.g., "GA_Dash", "GA_FireBolt")
        parent_class: Parent ability class. Common options:
                     - "BaseGameplayAbility" (default, project's base class)
                     - "BaseDamageGameplayAbility" (for damage abilities)
                     - "BaseAOEDamageAbility" (for area damage)
                     - "BaseSummonAbility" (for summon mechanics)
                     - "BaseWeaponAbility" (for weapon-based abilities)
                     - Or any custom ability class path
        folder_path: Content folder path for the Blueprint
        ability_type: Ability behavior type:
                     - "instant": Immediate execution (default)
                     - "duration": Has active duration
                     - "passive": Always active when granted
                     - "toggle": Can be toggled on/off
        ability_tags: Tags identifying this ability (e.g., ["Ability.Movement.Dash"])
        cancel_abilities_with_tag: Tags of abilities this will cancel
        block_abilities_with_tag: Tags of abilities this will block
        activation_owned_tags: Tags applied while ability is active
        activation_required_tags: Tags required on owner to activate
        activation_blocked_tags: Tags on owner that prevent activation
        source_required_tags: Tags required on source (caster) to activate
        source_blocked_tags: Tags on source that prevent activation
        target_required_tags: Tags required on target to activate
        target_blocked_tags: Tags on target that prevent activation
        cost_effect_class: Path to GameplayEffect for ability cost
        cooldown_effect_class: Path to GameplayEffect for cooldown
        cooldown_duration: Cooldown time in seconds (if using default cooldown)
        replication_policy: Network replication policy:
                           - "ReplicateNo": No replication
                           - "ReplicateYes": Replicate (default)
        instancing_policy: Ability instancing (IMPORTANT for stateful abilities):
                          - "NonInstanced": Shared CDO, no per-actor state
                          - "InstancedPerActor": One instance per actor (default, recommended)
                          - "InstancedPerExecution": New instance per activation

    Returns:
        Dictionary with created ability info and any property setting results
    """
    results = {"success": True, "steps": []}

    # Step 1: Create the Blueprint
    create_result = send_unreal_command("create_blueprint", {
        "name": name,
        "parent_class": parent_class,
        "folder_path": folder_path
    })
    results["steps"].append({"action": "create_blueprint", "result": create_result})

    if not create_result.get("success") and not create_result.get("result", {}).get("already_exists"):
        results["success"] = False
        results["error"] = f"Failed to create ability Blueprint: {create_result.get('error', 'Unknown error')}"
        return results

    blueprint_name = name
    results["blueprint_name"] = blueprint_name
    results["blueprint_path"] = create_result.get("result", {}).get("path", f"{folder_path}/{name}")

    # Step 2: Set GAS properties via set_blueprint_property
    property_results = []

    # Set ability tags (as GameplayTagContainer)
    if ability_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "AbilityTags",
            "property_value": {"GameplayTags": ability_tags}
        })
        property_results.append({"property": "AbilityTags", "result": tag_result})

    if cancel_abilities_with_tag:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "CancelAbilitiesWithTag",
            "property_value": {"GameplayTags": cancel_abilities_with_tag}
        })
        property_results.append({"property": "CancelAbilitiesWithTag", "result": tag_result})

    if block_abilities_with_tag:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "BlockAbilitiesWithTag",
            "property_value": {"GameplayTags": block_abilities_with_tag}
        })
        property_results.append({"property": "BlockAbilitiesWithTag", "result": tag_result})

    if activation_owned_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "ActivationOwnedTags",
            "property_value": {"GameplayTags": activation_owned_tags}
        })
        property_results.append({"property": "ActivationOwnedTags", "result": tag_result})

    if activation_required_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "ActivationRequiredTags",
            "property_value": {"GameplayTags": activation_required_tags}
        })
        property_results.append({"property": "ActivationRequiredTags", "result": tag_result})

    if activation_blocked_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "ActivationBlockedTags",
            "property_value": {"GameplayTags": activation_blocked_tags}
        })
        property_results.append({"property": "ActivationBlockedTags", "result": tag_result})

    # Source/Target tag requirements
    if source_required_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "SourceRequiredTags",
            "property_value": {"GameplayTags": source_required_tags}
        })
        property_results.append({"property": "SourceRequiredTags", "result": tag_result})

    if source_blocked_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "SourceBlockedTags",
            "property_value": {"GameplayTags": source_blocked_tags}
        })
        property_results.append({"property": "SourceBlockedTags", "result": tag_result})

    if target_required_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "TargetRequiredTags",
            "property_value": {"GameplayTags": target_required_tags}
        })
        property_results.append({"property": "TargetRequiredTags", "result": tag_result})

    if target_blocked_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "TargetBlockedTags",
            "property_value": {"GameplayTags": target_blocked_tags}
        })
        property_results.append({"property": "TargetBlockedTags", "result": tag_result})

    # Set cost and cooldown effects
    if cost_effect_class:
        cost_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "CostGameplayEffectClass",
            "property_value": cost_effect_class
        })
        property_results.append({"property": "CostGameplayEffectClass", "result": cost_result})

    if cooldown_effect_class:
        cooldown_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "CooldownGameplayEffectClass",
            "property_value": cooldown_effect_class
        })
        property_results.append({"property": "CooldownGameplayEffectClass", "result": cooldown_result})

    if cooldown_duration is not None:
        duration_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "CooldownDuration",
            "property_value": cooldown_duration
        })
        property_results.append({"property": "CooldownDuration", "result": duration_result})

    # Set replication and instancing policies
    if replication_policy:
        rep_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "ReplicationPolicy",
            "property_value": replication_policy
        })
        property_results.append({"property": "ReplicationPolicy", "result": rep_result})

    if instancing_policy:
        inst_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "InstancingPolicy",
            "property_value": instancing_policy
        })
        property_results.append({"property": "InstancingPolicy", "result": inst_result})

    results["property_results"] = property_results

    # Step 3: Compile the Blueprint
    compile_result = send_unreal_command("compile_blueprint", {"blueprint_name": blueprint_name})
    results["steps"].append({"action": "compile_blueprint", "result": compile_result})

    return results


def create_gameplay_effect(
    ctx: Context,
    name: str,
    folder_path: str = "/Game/Effects",
    effect_type: str = "instant",
    duration_seconds: float = None,
    period_seconds: float = None,
    modifiers: List[Dict[str, Any]] = None,
    executions: List[str] = None,
    components: List[Dict[str, Any]] = None,
    gameplay_cues: List[Dict[str, Any]] = None,
    asset_tags: List[str] = None,
    granted_tags: List[str] = None,
    application_tag_requirements: List[str] = None,
    removal_tag_requirements: List[str] = None,
    ongoing_tag_requirements: List[str] = None,
    removal_tag_immunity: List[str] = None,
    stacking_type: str = "None",
    stack_limit: int = 1,
    stack_duration_refresh: bool = False,
    stack_period_reset: bool = False
) -> Dict[str, Any]:
    """Implementation for creating a GameplayEffect Blueprint.

    GameplayEffects are the backbone of GAS - they modify attributes, apply tags,
    and trigger gameplay cues.

    Args:
        name: Name of the effect (e.g., "GE_DashCooldown", "GE_FireDamage")
        folder_path: Content folder path for the Blueprint
        effect_type: Effect duration type:
                    - "instant": Applied immediately, no duration
                    - "duration": Has set duration
                    - "infinite": Lasts until explicitly removed
                    - "periodic": Applies periodically
        duration_seconds: Duration for "duration" or "periodic" types
        period_seconds: Period for "periodic" type
        modifiers: List of attribute modifiers, each containing:
                  - "attribute": Attribute path (e.g., "VitalAttributesSet.Health")
                  - "operation": "Add", "Multiply", "Divide", "Override"
                  - "magnitude": Numeric value or magnitude calculation
        executions: List of execution calculation class names
        components: List of GameplayEffect components to add, each containing:
                   - "class": Component class (e.g., "AbilitiesGameplayEffectComponent",
                              "AdditionalEffectsGameplayEffectComponent",
                              "RemoveOtherGameplayEffectComponent",
                              "ImmunityGameplayEffectComponent",
                              "TargetTagsGameplayEffectComponent")
                   - Additional properties specific to the component type
        gameplay_cues: List of gameplay cue configurations, each containing:
                      - "cue_tag": GameplayCue tag (e.g., "GameplayCue.Damage.Fire")
                      - "min_level": Optional minimum level to trigger (float)
                      - "max_level": Optional maximum level to trigger (float)
        asset_tags: Tags identifying this effect
        granted_tags: Tags applied to target while effect is active
        application_tag_requirements: Tags required on target to apply
        removal_tag_requirements: Tags that cause removal when applied
        ongoing_tag_requirements: Tags required for effect to remain active
        removal_tag_immunity: Tags that grant immunity to this effect's removal
        stacking_type: How effect stacks:
                      - "None": No stacking (default)
                      - "AggregateBySource": Stack per source
                      - "AggregateByTarget": Stack on target
        stack_limit: Maximum stack count
        stack_duration_refresh: Refresh duration on new stack
        stack_period_reset: Reset period on new stack

    Returns:
        Dictionary with created effect info
    """
    results = {"success": True, "steps": []}

    # Step 1: Create the Blueprint with GameplayEffect as parent
    create_result = send_unreal_command("create_blueprint", {
        "name": name,
        "parent_class": "GameplayEffect",
        "folder_path": folder_path
    })
    results["steps"].append({"action": "create_blueprint", "result": create_result})

    if not create_result.get("success") and not create_result.get("result", {}).get("already_exists"):
        results["success"] = False
        results["error"] = f"Failed to create effect Blueprint: {create_result.get('error', 'Unknown error')}"
        return results

    blueprint_name = name
    results["blueprint_name"] = blueprint_name
    results["blueprint_path"] = create_result.get("result", {}).get("path", f"{folder_path}/{name}")

    property_results = []

    # Set duration policy based on effect_type
    duration_policy_map = {
        "instant": "Instant",
        "duration": "HasDuration",
        "infinite": "Infinite",
        "periodic": "HasDuration"  # Periodic uses duration + period
    }
    if effect_type in duration_policy_map:
        policy_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "DurationPolicy",
            "property_value": duration_policy_map[effect_type]
        })
        property_results.append({"property": "DurationPolicy", "result": policy_result})

    # Set duration
    if duration_seconds is not None:
        duration_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "DurationMagnitude",
            "property_value": {"Value": duration_seconds}
        })
        property_results.append({"property": "DurationMagnitude", "result": duration_result})

    # Set period
    if period_seconds is not None:
        period_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "Period",
            "property_value": period_seconds
        })
        property_results.append({"property": "Period", "result": period_result})

    # Set modifiers
    if modifiers:
        modifier_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "Modifiers",
            "property_value": modifiers
        })
        property_results.append({"property": "Modifiers", "result": modifier_result})

    # Set executions
    if executions:
        exec_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "Executions",
            "property_value": executions
        })
        property_results.append({"property": "Executions", "result": exec_result})

    # Set tags
    if asset_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "AssetTags",
            "property_value": {"GameplayTags": asset_tags}
        })
        property_results.append({"property": "AssetTags", "result": tag_result})

    if granted_tags:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "GrantedTags",
            "property_value": {"GameplayTags": granted_tags}
        })
        property_results.append({"property": "GrantedTags", "result": tag_result})

    if application_tag_requirements:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "ApplicationTagRequirements",
            "property_value": {"RequireTags": {"GameplayTags": application_tag_requirements}}
        })
        property_results.append({"property": "ApplicationTagRequirements", "result": tag_result})

    if removal_tag_requirements:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "RemovalTagRequirements",
            "property_value": {"RequireTags": {"GameplayTags": removal_tag_requirements}}
        })
        property_results.append({"property": "RemovalTagRequirements", "result": tag_result})

    if ongoing_tag_requirements:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "OngoingTagRequirements",
            "property_value": {"RequireTags": {"GameplayTags": ongoing_tag_requirements}}
        })
        property_results.append({"property": "OngoingTagRequirements", "result": tag_result})

    if removal_tag_immunity:
        tag_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "GrantedApplicationImmunityTags",
            "property_value": {"RequireTags": {"GameplayTags": removal_tag_immunity}}
        })
        property_results.append({"property": "GrantedApplicationImmunityTags", "result": tag_result})

    # Set components (GameplayEffectComponents)
    if components:
        comp_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "GEComponents",
            "property_value": components
        })
        property_results.append({"property": "GEComponents", "result": comp_result})

    # Set gameplay cues
    if gameplay_cues:
        cue_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "GameplayCues",
            "property_value": gameplay_cues
        })
        property_results.append({"property": "GameplayCues", "result": cue_result})

    # Set stacking
    if stacking_type != "None":
        stack_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "StackingType",
            "property_value": stacking_type
        })
        property_results.append({"property": "StackingType", "result": stack_result})

        limit_result = send_unreal_command("set_blueprint_property", {
            "blueprint_name": blueprint_name,
            "property_name": "StackLimitCount",
            "property_value": stack_limit
        })
        property_results.append({"property": "StackLimitCount", "result": limit_result})

    results["property_results"] = property_results

    # Step 3: Compile the Blueprint
    compile_result = send_unreal_command("compile_blueprint", {"blueprint_name": blueprint_name})
    results["steps"].append({"action": "compile_blueprint", "result": compile_result})

    return results


def create_gameplay_cue(
    ctx: Context,
    name: str,
    cue_tag: str,
    folder_path: str = "/Game/Abilities/Cues",
    cue_type: str = "Notify",
    implement_on_execute: bool = True,
    implement_on_active: bool = False,
    implement_on_remove: bool = False,
    implement_while_active: bool = False
) -> Dict[str, Any]:
    """Implementation for creating a GameplayCue Blueprint.

    GameplayCues provide VFX/SFX feedback for GAS events. They're triggered by
    GameplayEffects or directly from abilities via cue tags.

    Args:
        name: Name of the cue (e.g., "GCN_Impact_Fire", "GCN_Buff_Haste")
        cue_tag: GameplayCue tag (e.g., "GameplayCue.Impact.Fire")
                 Must start with "GameplayCue."
        folder_path: Content folder path for the Blueprint
        cue_type: Type of cue:
                 - "Notify": Non-instanced, static functions (default, most common)
                 - "Actor": Instanced actor spawned for duration effects
                 - "Static": Pure static, no state
        implement_on_execute: Add OnExecute event (for instant cues)
        implement_on_active: Add OnActive event (when cue becomes active)
        implement_on_remove: Add OnRemove event (when cue is removed)
        implement_while_active: Add WhileActive event (called every tick while active)

    Returns:
        Dictionary with created cue info
    """
    results = {"success": True, "steps": []}

    # Validate cue tag format
    if not cue_tag.startswith("GameplayCue."):
        results["success"] = False
        results["error"] = f"cue_tag must start with 'GameplayCue.' but got: {cue_tag}"
        return results

    # Determine parent class based on cue_type
    parent_class_map = {
        "Notify": "GameplayCueNotify_Static",
        "Actor": "GameplayCueNotify_Actor",
        "Static": "GameplayCueNotify_Static"
    }
    parent_class = parent_class_map.get(cue_type, "GameplayCueNotify_Static")

    # Step 1: Create the Blueprint
    create_result = send_unreal_command("create_blueprint", {
        "name": name,
        "parent_class": parent_class,
        "folder_path": folder_path
    })
    results["steps"].append({"action": "create_blueprint", "result": create_result})

    if not create_result.get("success") and not create_result.get("result", {}).get("already_exists"):
        results["success"] = False
        results["error"] = f"Failed to create cue Blueprint: {create_result.get('error', 'Unknown error')}"
        return results

    blueprint_name = name
    results["blueprint_name"] = blueprint_name
    results["blueprint_path"] = create_result.get("result", {}).get("path", f"{folder_path}/{name}")
    results["cue_type"] = cue_type
    results["parent_class"] = parent_class

    property_results = []

    # Set the GameplayCue tag
    cue_tag_result = send_unreal_command("set_blueprint_property", {
        "blueprint_name": blueprint_name,
        "property_name": "GameplayCueTag",
        "property_value": cue_tag
    })
    property_results.append({"property": "GameplayCueTag", "result": cue_tag_result})

    results["property_results"] = property_results
    results["cue_tag"] = cue_tag

    # Note which event implementations were requested (user will implement in BP editor)
    results["requested_events"] = {
        "OnExecute": implement_on_execute,
        "OnActive": implement_on_active,
        "OnRemove": implement_on_remove,
        "WhileActive": implement_while_active
    }

    # Step 3: Compile the Blueprint
    compile_result = send_unreal_command("compile_blueprint", {"blueprint_name": blueprint_name})
    results["steps"].append({"action": "compile_blueprint", "result": compile_result})

    return results