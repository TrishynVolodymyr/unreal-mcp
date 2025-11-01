"""
Blueprint Action Tools - Dynamically discover available Blueprint actions

This module provides tools for discovering Blueprint actions using the FBlueprintActionDatabase,
allowing dynamic exploration of what nodes and functions are available for different pin types,
classes, and class hierarchies.
"""

import logging
from typing import Dict, Any, List
from mcp.server.fastmcp import FastMCP, Context
from utils.blueprint_actions.blueprint_action_operations import (
    get_actions_for_pin as get_actions_for_pin_impl,
    get_actions_for_class as get_actions_for_class_impl,
    get_actions_for_class_hierarchy as get_actions_for_class_hierarchy_impl,
    search_blueprint_actions as search_blueprint_actions_impl,
    get_node_pin_info as inspect_node_pin_connection_impl,
    create_node_by_action_name as create_node_by_action_name_impl
)

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_action_tools(mcp: FastMCP):
    """Register blueprint action tools with the MCP server."""

    @mcp.tool()
    def get_actions_for_pin(
        ctx: Context,
        pin_type: str, 
        pin_subcategory: str = "",
        search_filter: str = "",
        max_results: int = 20
    ) -> Dict[str, Any]:
        """
        Get all available Blueprint actions for a specific pin type with search filtering.
        
        This tool uses Unreal's FBlueprintActionDatabase to dynamically discover what nodes/functions
        are available when connecting to a specific pin type, similar to what you see in the context
        menu when dragging from a pin in the Blueprint editor.
        
        Args:
            pin_type: The type of pin (object, int, float, bool, string, struct, etc.)
            pin_subcategory: The subcategory/class name for object pins (e.g., "PlayerController", "Pawn")
            search_filter: Optional search string to filter results (searches in name, keywords, category)
            max_results: Maximum number of results to return (default: 50)
        
        Returns:
            Dict containing:
                - success: Boolean indicating if the operation succeeded
                - actions: List of available actions with title, tooltip, category, keywords
                - pin_type: The pin type that was queried
                - pin_subcategory: The pin subcategory that was queried
                - action_count: Number of actions found
                - message: Status message
        
        Examples:
            # Get actions for a PlayerController object pin
            get_actions_for_pin(pin_type="object", pin_subcategory="PlayerController")
            
            # Get actions for a float pin
            get_actions_for_pin(pin_type="float")
            
            # Get actions for a Vector struct pin
            get_actions_for_pin(pin_type="struct", pin_subcategory="Vector")
        """
        return get_actions_for_pin_impl(ctx, pin_type, pin_subcategory, search_filter, max_results)

    @mcp.tool()
    def get_actions_for_class(
        ctx: Context,
        class_name: str,
        search_filter: str = "",
        max_results: int = 20
    ) -> Dict[str, Any]:
        """
        Get all available Blueprint actions for a specific class with search filtering.
        
        This tool uses Unreal's FBlueprintActionDatabase to discover what functions and nodes
        are available for a specific class. This includes all the BlueprintCallable functions,
        events, and other actions that can be used with instances of this class.
        
        Args:
            class_name: Name or path of the class to get actions for (e.g., "PlayerController", "Pawn", "Actor")
            search_filter: Optional search string to filter results (searches in name, keywords, category)
            max_results: Maximum number of results to return (default: 50)
        
        Returns:
            Dict containing:
                - success: Boolean indicating if the operation succeeded
                - actions: List of available actions with title, tooltip, category, keywords
                - class_name: The class name that was queried
                - action_count: Number of actions found
                - message: Status message
        
        Examples:
            # Get actions for PlayerController class
            get_actions_for_class(class_name="PlayerController")
            
            # Get actions for Actor class
            get_actions_for_class(class_name="Actor")
            
            # Get actions using full path
            get_actions_for_class(class_name="/Script/Engine.Pawn")
        """
        return get_actions_for_class_impl(ctx, class_name, search_filter, max_results)

    @mcp.tool()
    def get_actions_for_class_hierarchy(
        ctx: Context,
        class_name: str,
        search_filter: str = "",
        max_results: int = 20
    ) -> Dict[str, Any]:
        """
        Get all available Blueprint actions for a class and its entire inheritance hierarchy with search filtering.
        
        This tool uses Unreal's FBlueprintActionDatabase to discover what functions and nodes
        are available for a class and all of its parent classes. This gives you the complete
        picture of all available actions including inherited functionality.
        
        Args:
            class_name: Name or path of the class to get actions for (e.g., "PlayerController", "Pawn", "Actor")
            search_filter: Optional search string to filter results (searches in name, keywords, category)
            max_results: Maximum number of results to return (default: 50)
        
        Returns:
            Dict containing:
                - success: Boolean indicating if the operation succeeded
                - actions: List of available actions with title, tooltip, category, keywords
                - class_hierarchy: List of classes in the inheritance chain
                - category_counts: Count of actions per category
                - class_name: The class name that was queried
                - action_count: Number of actions found
                - message: Status message
        
        Examples:
            # Get actions for PlayerController and its parent classes
            get_actions_for_class_hierarchy(class_name="PlayerController")
            
            # Get comprehensive actions for Pawn class hierarchy
            get_actions_for_class_hierarchy(class_name="Pawn")
            
            # This will include actions from Actor, Object, etc.
            get_actions_for_class_hierarchy(class_name="StaticMeshActor")
        """
        return get_actions_for_class_hierarchy_impl(ctx, class_name, search_filter, max_results)

    @mcp.tool()
    def search_blueprint_actions(
        ctx: Context,
        search_query: str,
        category: str = "",
        max_results: int = 20,
        blueprint_name: str = None
    ) -> Dict[str, Any]:
        """
        Search for EXISTING Blueprint actions/nodes using keywords.
        
        IMPORTANT: This tool ONLY searches for existing Blueprint nodes and functions that are 
        already available in Unreal Engine's FBlueprintActionDatabase. It searches built-in 
        engine functionality like math operations, engine functions, and existing Blueprint nodes.
        
        DO NOT use this tool to search for:
        - Custom functions you want to create
        - Custom events you want to create  
        - Custom Blueprint logic that doesn't exist yet
        - Concepts like "custom function", "my function", etc.
        
        USE this tool to find:
        - Built-in math operations (add, multiply, etc.)
        - Engine functions (GetActorLocation, SetActorLocation, etc.)
        - Flow control nodes (Branch, Sequence, etc.)
        - Existing Blueprint nodes and functions
        - Enhanced Input Actions (IA_Jump, IA_Move, etc.) - search for "IA_" prefix
        
        If you want to CREATE custom functionality, use create_node_by_action_name directly
        with the appropriate node type (like "CustomEvent" for custom events).
        
        This tool is similar to typing in the search box in Unreal's Blueprint editor context menu.
        
        Args:
            search_query: Search string to find EXISTING actions (searches in name, keywords, category, tooltip)
            category: Optional category filter (Flow Control, Math, Utilities, etc.)
            max_results: Maximum number of results to return (default: 20)
            blueprint_name: Optional name of the Blueprint asset for local variable discovery
        
        Returns:
            Dict containing:
                - success: Boolean indicating if the operation succeeded
                - actions: List of matching EXISTING actions with title, tooltip, category, keywords
                - search_query: The search query that was used
                - category_filter: The category filter that was applied
                - action_count: Number of actions found
                - message: Status message
        
        Examples:
            # Search for existing math operations
            search_blueprint_actions(search_query="add")
            
            # Search for existing flow control nodes
            search_blueprint_actions(search_query="branch", category="Flow Control")
            
            # Search for existing print functions
            search_blueprint_actions(search_query="print")
            
            # Search for Enhanced Input Actions with IA_ prefix
            search_blueprint_actions(search_query="IA_")
            
            # Search for specific Enhanced Input Action
            search_blueprint_actions(search_query="IA_Jump")
            
            # Search for existing variable nodes in a Blueprint
            search_blueprint_actions(search_query="myvar", blueprint_name="BP_TestActor")
        """
        return search_blueprint_actions_impl(ctx, search_query, category, max_results, blueprint_name)

    @mcp.tool()
    def inspect_node_pin_connection(
        ctx: Context,
        node_name: str,
        pin_name: str
    ) -> Dict[str, Any]:
        """
        Inspect Blueprint node pin connection details including expected types and compatibility info.
        
        Args:
            node_name: Name of the Blueprint node (e.g., "Create Widget", "Get Controller", "Cast to PlayerController")
            pin_name: Name of the specific pin (e.g., "Owning Player", "Class", "Return Value", "Target")
        
        Returns:
            Dict containing:
                - success: Boolean indicating if the operation succeeded
                - node_name: The node name that was queried
                - pin_name: The pin name that was queried
                - pin_info: Detailed information about the pin including:
                    - pin_type: Type category (object, class, exec, etc.)
                    - expected_type: Specific type expected (PlayerController, Class<UserWidget>, etc.)
                    - description: Description of the pin's purpose
                    - is_required: Whether the pin must be connected
                    - is_input: Whether it's an input (true) or output (false) pin
                - message: Status message
                - available_pins: List of available pins if the node is known but pin is not found
        
        Examples:
            # Inspect the Class pin on Create Widget node
            inspect_node_pin_connection(node_name="Create Widget", pin_name="Class")
            
            # Inspect the Target pin on Get Controller node
            inspect_node_pin_connection(node_name="Get Controller", pin_name="Target")
            
            # Inspect the Owning Player pin on Create Widget node
            inspect_node_pin_connection(node_name="Create Widget", pin_name="Owning Player")
        """
        return inspect_node_pin_connection_impl(ctx, node_name, pin_name)

    @mcp.tool()
    def create_node_by_action_name(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        class_name: str = "",
        node_position: List[float] = None,
        target_graph: str = "EventGraph",
        **kwargs
    ) -> Dict[str, Any]:
        """
        Create a blueprint node by discovered action/function name.

        This allows you to create blueprint nodes using the function names discovered from
        the FBlueprintActionDatabase (via get_actions_for_pin, get_actions_for_class, etc.).
        
        **IMPORTANT: When to specify class_name**:
        - ALWAYS specify class_name when search returns multiple functions with the same name
        - Example: "GetOwner" exists in Actor, ActorComponent, CheatManager classes
        - Without class_name, the system may pick the wrong variant
        - Use get_actions_for_class() instead of search_blueprint_actions() to get context-specific results
        
        **WORKING NODE TYPES**:
        - Function calls (KismetMathLibrary, GameplayStatics, etc.)
        - For Each Loop (Map) - UK2Node_MapForEach
        - For Each Loop (Set) - UK2Node_SetForEach
        - Control flow nodes (Branch, Sequence, etc.)
        - Variable get/set nodes
        - Custom events
        - Cast nodes
        - Enhanced Input Action events (e.g., IA_Jump, IA_Move, IA_Interact)

        Args:
            blueprint_name: Name of the target Blueprint (e.g., "BP_MyActor")
            function_name: Name of the function to create a node for (from discovered actions).
                          For Enhanced Input Actions, use the action name (e.g., "IA_Interact")
            class_name: Class name for the function (REQUIRED when multiple classes have the same function).
                       Supports both short names like "ActorComponent" and full paths like "/Script/Engine.ActorComponent".
                       For Enhanced Input Actions, you can optionally use "EnhancedInputAction"
            node_position: Optional [X, Y] position in the graph (e.g., [100, 200])
            target_graph: Optional name of the specific graph to place the node in (e.g., "CanInteract", "GetSpeaker").
                         If not specified, defaults to "EventGraph". This allows creating nodes in custom functions.
                         Can be passed either as a direct parameter or as a keyword argument.
            **kwargs: Additional parameters for special nodes (e.g., target_type="PlayerController" for Cast nodes)

        Returns:
            Dict containing:
                - success: Boolean indicating if the node was created
                - node_id: Unique identifier for the created node (if successful)
                - node_type: Type of node that was created
                - pins: List of available pins on the created node
                - position: Position where the node was placed
                - message: Status message or error details

        Examples:
            # RECOMMENDED: Use get_actions_for_class to find the correct function for your Blueprint type
            actions = get_actions_for_class(class_name="ActorComponent", search_filter="GetOwner")
            # Then create with explicit class_name
            create_node_by_action_name(
                blueprint_name="BP_DialogueComponent",
                function_name="GetOwner",
                class_name="ActorComponent",  # Explicit class prevents wrong variant
                node_position=[100, 100]
            )
            
            # Create a working math function node (use SelectFloat, not Add_FloatFloat)
            create_node_by_action_name(
                blueprint_name="BP_Calculator",
                function_name="SelectFloat",
                class_name="KismetMathLibrary",
                node_position=[100, 200]
            )
            
            # Create a node in a specific function graph - Method 1 (direct parameter)
            create_node_by_action_name(
                blueprint_name="DialogueComponent",
                function_name="GetOwner",
                class_name="ActorComponent",  # Specify class to avoid ambiguity
                target_graph="CanInteract",
                node_position=[100, 100]
            )
            
            # Create a node in a specific function graph - Method 2 (keyword argument)
            create_node_by_action_name(
                blueprint_name="DialogueComponent",
                function_name="GetDistanceTo",
                node_position=[300, 100],
                target_graph="CanInteract"
            )
            
            # Create a comparison operator in a function
            create_node_by_action_name(
                blueprint_name="DialogueComponent",
                function_name="<",
                node_position=[500, 100],
                target_graph="CanInteract"
            )
            
            # Create a Map ForEach loop
            create_node_by_action_name(
                blueprint_name="BP_MyActor",
                function_name="For Each Loop (Map)",
                node_position=[300, 400]
            )
            
            # Create a Set ForEach loop
            create_node_by_action_name(
                blueprint_name="BP_MyActor", 
                function_name="For Each Loop (Set)",
                node_position=[500, 400]
            )
            
            # Create a custom event with specific name
            create_node_by_action_name(
                blueprint_name="BP_MyActor",
                function_name="CustomEvent",
                event_name="OnPlayerDied"
            )
            
            # Create a cast node with target type
            create_node_by_action_name(
                blueprint_name="BP_MyActor",
                function_name="Cast",
                target_type="PlayerController"
            )
            
            # Create Enhanced Input Action event node - Method 1 (just function_name)
            create_node_by_action_name(
                blueprint_name="BP_ThirdPersonCharacter",
                function_name="IA_Interact",
                node_position=[100, 200]
            )
            
            # Create Enhanced Input Action event node - Method 2 (with class_name)
            create_node_by_action_name(
                blueprint_name="BP_ThirdPersonCharacter",
                function_name="IA_Jump",
                class_name="EnhancedInputAction",
                node_position=[100, 300]
            )
            
            # Create without specifying class (will search common classes)
            create_node_by_action_name(
                blueprint_name="BP_MyActor",
                function_name="GetActorLocation"
            )
            
            # Find correct function names using search first:
            # search_blueprint_actions(search_query="float", category="Math") 
            # Then use the discovered function names
        """
        return create_node_by_action_name_impl(ctx, blueprint_name, function_name, class_name, node_position, target_graph=target_graph, **kwargs) 