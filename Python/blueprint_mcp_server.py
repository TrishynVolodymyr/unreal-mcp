#!/usr/bin/env python3
"""Blueprint MCP Server for Unreal Engine Blueprint operations."""

from typing import Any, Dict, List

from fastmcp import FastMCP

from utils.async_tcp_utils import send_tcp_command

app = FastMCP("Blueprint MCP Server")


@app.tool()
async def create_blueprint(name: str, parent_class: str, folder_path: str = "") -> Dict[str, Any]:
    """
    Create a new Blueprint class.

    Args:
        name: Name of the new Blueprint class
        parent_class: Parent class for the Blueprint (e.g., "Actor", "Pawn")
        folder_path: Optional folder path where the blueprint should be created

    Note on renaming a project C++ class that existing BPs derive from:
    CoreRedirects in DefaultEngine.ini do not always relink Blueprints whose
    parent class was previously deleted — the asset can end up persisted with
    an engine-class fallback parent (e.g. ACharacter instead of your project
    base class). After such a rename, use `set_blueprint_parent_class` on each
    affected BP to do an explicit reparent + compile + save.
    """
    params = {"name": name, "parent_class": parent_class}
    if folder_path:
        params["folder_path"] = folder_path
    return await send_tcp_command("create_blueprint", params)


@app.tool()
async def set_blueprint_parent_class(
    blueprint_name: str,
    new_parent_class: str
) -> Dict[str, Any]:
    """
    Change the parent class of an existing Blueprint.

    Reparents `Blueprint->ParentClass` and recompiles. The asset is marked
    dirty and saved by the editor's normal flow. Mirrors the pattern used by
    `set_widget_parent_class` (umgMCP) but for regular UBlueprint.

    Args:
        blueprint_name: Simple name or full content path of the Blueprint
            (e.g., "BP_TopDownCharacter" or
            "/Game/TopDown/Blueprints/BP_TopDownCharacter")
        new_parent_class: Target parent class. Accepts:
            - Native class short name: "Character", "Actor"
            - Native class full path: "/Script/Engine.Character"
            - Project module short name: "PawnBase"
            - Project module full path: "/Script/<ProjectName>.PawnBase"
            - Blueprint asset path: "/Game/Blueprints/BP_Base"

    Returns:
        Dict containing:
        - success: Whether the reparent succeeded
        - changed: False if BP already had the requested parent (no-op)
        - blueprint_name: Name of the modified Blueprint
        - old_parent_class: Previous parent class name
        - new_parent_class: Resolved new parent class path

    Examples:
        # Reparent to a project C++ base class
        set_blueprint_parent_class(
            blueprint_name="/Game/TopDown/Blueprints/BP_TopDownCharacter",
            new_parent_class="PawnBase"
        )

        # Reparent to engine class (revert)
        set_blueprint_parent_class(
            blueprint_name="BP_TopDownCharacter",
            new_parent_class="Character"
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "new_parent_class": new_parent_class
    }
    return await send_tcp_command("set_blueprint_parent_class", params)


@app.tool()
async def compile_blueprint(blueprint_name: str) -> Dict[str, Any]:
    """
    Compile a Blueprint with enhanced error reporting.

    This tool provides detailed compilation error information including:
    - Node-level errors (missing connections, invalid types)
    - Graph-level errors (unreachable code, missing entry points)
    - Blueprint-level errors (missing parent class, invalid configurations)
    - Component-level errors (invalid classes, missing templates)

    Args:
        blueprint_name: Name of the target Blueprint

    Returns:
        Dictionary containing compilation results with detailed error information:
        - For successful compilation: success=True, compilation_time_seconds, status
        - For failed compilation: success=False, error message, compilation_errors array
        - For compilation with warnings: success=True, warnings array
    """
    params = {"blueprint_name": blueprint_name}
    return await send_tcp_command("compile_blueprint", params)


@app.tool()
async def add_blueprint_variable(
    blueprint_name: str,
    variable_name: str,
    variable_type: str,
    is_exposed: bool = False
) -> Dict[str, Any]:
    """
    Add a variable to a Blueprint.

    Args:
        blueprint_name: Name of the target Blueprint
        variable_name: Name of the variable
        variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
        is_exposed: Whether to expose the variable to the editor
    """
    params = {
        "blueprint_name": blueprint_name,
        "variable_name": variable_name,
        "variable_type": variable_type,
        "is_exposed": is_exposed
    }
    return await send_tcp_command("add_blueprint_variable", params)


@app.tool()
async def add_event_dispatcher(
    blueprint_name: str,
    dispatcher_name: str
) -> Dict[str, Any]:
    """
    Add an Event Dispatcher (multicast delegate) to a Blueprint.
    Event Dispatchers allow Blueprints to broadcast events that other Blueprints can listen to.
    Common use: custom button widgets exposing OnClicked, components broadcasting state changes.

    Args:
        blueprint_name: Name of the target Blueprint
        dispatcher_name: Name of the event dispatcher (e.g., "OnClicked", "OnItemSelected")
    """
    params = {
        "blueprint_name": blueprint_name,
        "dispatcher_name": dispatcher_name
    }
    return await send_tcp_command("add_event_dispatcher", params)


@app.tool()
async def delete_blueprint_variable(
    blueprint_name: str,
    variable_name: str
) -> Dict[str, Any]:
    """
    Delete a variable from a Blueprint.

    Args:
        blueprint_name: Name of the target Blueprint
        variable_name: Name of the variable to delete
    """
    params = {
        "blueprint_name": blueprint_name,
        "variable_name": variable_name
    }
    return await send_tcp_command("delete_blueprint_variable", params)



@app.tool()
async def add_component_to_blueprint(
    blueprint_name: str,
    component_type: str,
    component_name: str,
    location: List[float] = None,
    rotation: List[float] = None,
    scale: List[float] = None
) -> Dict[str, Any]:
    """
    Add a component to a Blueprint.

    Args:
        blueprint_name: Name of the target Blueprint
        component_type: Type of component to add
        component_name: Name for the new component
        location: [X, Y, Z] coordinates for component's position
        rotation: [Pitch, Yaw, Roll] values for component's rotation
        scale: [X, Y, Z] values for component's scale
    """
    params = {
        "blueprint_name": blueprint_name,
        "component_type": component_type,
        "component_name": component_name
    }
    if location:
        params["location"] = location
    if rotation:
        params["rotation"] = rotation
    if scale:
        params["scale"] = scale
    return await send_tcp_command("add_component_to_blueprint", params)


@app.tool()
async def modify_blueprint_component_properties(
    blueprint_name: str,
    component_name: str,
    properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Modify ANY properties on a Blueprint component.

    This is a universal tool for setting component properties including:
    - WidgetClass on WidgetComponent
    - StaticMesh on StaticMeshComponent
    - Material on any mesh component
    - LightColor, Intensity on LightComponent
    - Any other component-specific properties

    Args:
        blueprint_name: Name of the target Blueprint
        component_name: Name of the component to modify
        properties: Dict of property name -> value to set on the component.
                    Example: {"WidgetClass": "/Game/UI/WBP_Health", "DrawSize": [200, 50]}

    WARNING - DO NOT USE FOR COMPONENT EVENTS:
        For Blueprint component events (OnComponentBeginOverlap, OnComponentEndOverlap, etc.),
        DO NOT use this tool with bind_events. Instead use create_node_by_action_name:

        # CORRECT way to bind component overlap events:
        create_node_by_action_name(
            blueprint_name="BP_MyActor",
            function_name="ComponentBoundEvent",
            component_name="MySphereComponent",
            event_name="OnComponentBeginOverlap"
        )

        The bind_events parameter here is only for UMG widget events (OnClicked, etc.)
    """
    props = properties or {}
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "kwargs": json.dumps(props)
    }
    return await send_tcp_command("modify_blueprint_component_properties", params)


@app.tool()
async def create_custom_blueprint_function(
    blueprint_name: str,
    function_name: str,
    inputs: List[Dict[str, str]] = None,
    outputs: List[Dict[str, str]] = None,
    is_pure: bool = False,
    is_const: bool = False,
    access_specifier: str = "Public",
    category: str = "Default"
) -> Dict[str, Any]:
    """
    Create a custom user-defined function in a Blueprint.
    This will create a new function that appears in the Functions section of the Blueprint editor.

    Args:
        blueprint_name: Name of the target Blueprint
        function_name: Name of the custom function to create
        inputs: List of input parameters, each with 'name' and 'type' keys
        outputs: List of output parameters, each with 'name' and 'type' keys
        is_pure: Whether the function is pure (no execution pins)
        is_const: Whether the function is const
        access_specifier: Access level ("Public", "Protected", "Private")
        category: Category for organization in the functions list
    """
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name,
        "is_pure": is_pure,
        "is_const": is_const,
        "access_specifier": access_specifier,
        "category": category
    }
    if inputs:
        params["inputs"] = inputs
    if outputs:
        params["outputs"] = outputs
    return await send_tcp_command("create_custom_blueprint_function", params)


@app.tool()
async def set_blueprint_variable_value(
    blueprint_name: str,
    variable_name: str,
    value: Any
) -> Dict[str, Any]:
    """
    Set the default value of a Blueprint variable (CDO).

    This sets a property on the Blueprint's Class Default Object — i.e. it
    changes the default value of a *declared variable* for all instances of
    the Blueprint. It does NOT modify Blueprint asset-level metadata.

    For asset-level changes use the dedicated tools:
    - ParentClass: `set_blueprint_parent_class`
    - Implemented interfaces: `add_interface_to_blueprint`
    - Variable declaration: `add_blueprint_variable` / `delete_blueprint_variable`

    Args:
        blueprint_name: Name of the target Blueprint
        variable_name: Name of the variable to modify (must already be declared
                       on the Blueprint or one of its parents)
        value: New default value for the variable. Type must match the variable type:
            - For bool: True/False
            - For int/float: numeric values
            - For string: text values
            - For arrays: list of values
            - For structs: dictionary with field names as keys

    Returns:
        Dict containing success status and the property name that was set.

    Note: the underlying TCP command is `set_blueprint_property` for historical
    reasons; the parameter name on the C++ side is `property_name` but it must
    name a declared variable, not an arbitrary asset property.
    """
    params = {
        "blueprint_name": blueprint_name,
        "property_name": variable_name,
        "property_value": value
    }
    return await send_tcp_command("set_blueprint_property", params)


@app.tool()
async def get_blueprint_metadata(
    blueprint_name: str,
    fields: List[str] = None,
    graph_name: str = None,
    node_type: str = None,
    event_type: str = None,
    detail_level: str = None,
    component_name: str = None
) -> Dict[str, Any]:
    """
    Get comprehensive metadata about a Blueprint with selective field querying.

    This unified command replaces multiple separate commands:
    - list_blueprint_components (use fields=["components"])
    - get parent class info (use fields=["parent_class"])
    - get Blueprint variables (use fields=["variables"])
    - get Blueprint functions (use fields=["functions"])
    - find_blueprint_nodes (use fields=["graph_nodes"] with node_type/event_type filters)

    Args:
        blueprint_name: Name or path of the Blueprint
        fields: List of metadata fields to retrieve. If None or ["*"], returns all.
                Options:
                - "parent_class": Parent class name and path
                - "interfaces": Implemented interfaces and their functions
                - "variables": Blueprint variables with types and default values
                - "functions": Custom Blueprint functions
                - "components": All components with names and types (replaces list_blueprint_components)
                - "component_properties": Single component's property values (requires component_name parameter)
                - "graphs": Event graphs and function graphs
                - "status": Compilation status and error state
                - "metadata": Asset metadata and tags
                - "timelines": Timeline components
                - "asset_info": Asset path, size, and modification date
                - "orphaned_nodes": Detects disconnected nodes in all graphs
                - "graph_nodes": Detailed node info with pin connections and default values
        graph_name: Optional graph name filter for "graph_nodes" field.
                   When specified, only returns nodes from that specific graph.
        node_type: Optional node type filter for "graph_nodes" field.
                  Options: "Event", "Function", "Variable", "Comment", or any class name.
        event_type: Optional event type filter for "graph_nodes" field.
                   Options: "BeginPlay", "Tick", "EndPlay", "Destroyed", "Construct",
                   or any custom event name.
        detail_level: Optional detail level for "graph_nodes" field. Options:
                     - "summary": Node IDs and titles only (minimal output)
                     - "flow": Node IDs, titles, and exec pin connections only (DEFAULT)
                     - "full": Everything including all data pin connections and default values
        component_name: Required when using "component_properties" field. Specifies which
                       component to get properties for (e.g., "ProjectileMovement", "CollisionSphere").

    Returns:
        Dictionary containing requested metadata fields

    Example:
        get_blueprint_metadata(blueprint_name="BP_MyActor", fields=["components"])
    """
    params = {"blueprint_name": blueprint_name}
    if fields is not None:
        params["fields"] = fields
    if graph_name is not None:
        params["graph_name"] = graph_name
    if node_type is not None:
        params["node_type"] = node_type
    if event_type is not None:
        params["event_type"] = event_type
    if detail_level is not None:
        params["detail_level"] = detail_level
    if component_name is not None:
        params["component_name"] = component_name
    return await send_tcp_command("get_blueprint_metadata", params)


@app.tool()
async def add_interface_to_blueprint(
    blueprint_name: str,
    interface_name: str
) -> Dict[str, Any]:
    """
    Add an interface to a Blueprint.

    Args:
        blueprint_name: Name of the target Blueprint (e.g., "BP_MyActor")
        interface_name: Name or path of the interface to add (e.g., "/Game/Blueprints/BPI_MyInterface")
    """
    params = {
        "blueprint_name": blueprint_name,
        "interface_name": interface_name
    }
    return await send_tcp_command("add_interface_to_blueprint", params)


@app.tool()
async def create_blueprint_interface(
    name: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """
    Create a new Blueprint Interface asset.

    Args:
        name: Name of the interface (e.g., "BPI_Interactable")
        folder_path: Optional folder path for the interface (e.g., "/Game/Interfaces")
    """
    params = {"name": name}
    if folder_path:
        params["folder_path"] = folder_path
    return await send_tcp_command("create_blueprint_interface", params)


if __name__ == "__main__":
    app.run()
