"""
Blueprint Action Operations for Unreal MCP.

This module provides utility functions for discovering Blueprint actions using the FBlueprintActionDatabase.
"""

import logging
import json
from typing import Dict, Any, List
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def get_actions_for_pin(
    ctx: Context,
    pin_type: str,
    pin_subcategory: str = "",
    search_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """Implementation for getting actions for a specific pin type with search filtering."""
    params = {
        "pin_type": pin_type,
        "pin_subcategory": pin_subcategory,
        "search_filter": search_filter,
        "max_results": max_results
    }
    
    command_result = send_unreal_command("get_actions_for_pin", params)
    
    # Extract the actual result from the wrapper
    if "result" in command_result:
        result = command_result["result"]
    else:
        result = command_result
    
    # Check for duplicate function names and add warning if found
    if result.get("success") and "actions" in result:
        actions = result["actions"]
        function_names = {}
        
        # Count occurrences of each function name
        for action in actions:
            func_name = action.get("function_name", "")
            if func_name:
                if func_name not in function_names:
                    function_names[func_name] = []
                function_names[func_name].append(action.get("class_name", "Unknown"))
        
        # Find duplicates
        duplicates = {name: classes for name, classes in function_names.items() if len(classes) > 1}
        
        # Add warning message if duplicates exist - prepend to existing message
        if duplicates:
            warning_lines = [
                "IMPORTANT REMINDER: Multiple functions with the same name found!",
                "",
                "When using create_node_by_action_name() with these results,",
                "you MUST specify the 'class_name' parameter to avoid getting the wrong variant.",
                "",
                "Functions with duplicates:"
            ]
            
            for func_name, classes in duplicates.items():
                unique_classes = list(set(classes))  # Remove duplicates in class list
                warning_lines.append(f"  - '{func_name}' exists in: {', '.join(unique_classes)}")
            
            warning_lines.append("")
            warning_lines.append("Example:")
            first_dup = list(duplicates.keys())[0]
            first_class = list(set(duplicates[first_dup]))[0]
            warning_lines.append(f"  create_node_by_action_name(")
            warning_lines.append(f"      function_name=\"{first_dup}\",")
            warning_lines.append(f"      class_name=\"{first_class}\",  # ← REQUIRED!")
            warning_lines.append(f"      ...)")
            warning_lines.append("")
            warning_lines.append("=" * 70)
            warning_lines.append("")
            
            # Prepend warning to message
            original_message = result.get("message", "")
            result["message"] = "\n".join(warning_lines) + original_message
    
    return result

def get_actions_for_class(
    ctx: Context,
    class_name: str,
    search_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """Implementation for getting actions for a specific class with search filtering."""
    params = {
        "class_name": class_name,
        "search_filter": search_filter,
        "max_results": max_results
    }
    
    command_result = send_unreal_command("get_actions_for_class", params)
    
    # Extract the actual result from the wrapper
    if "result" in command_result:
        result = command_result["result"]
    else:
        result = command_result
    
    # Check for duplicate function names and add warning if found
    if result.get("success") and "actions" in result:
        actions = result["actions"]
        function_names = {}
        
        # Count occurrences of each function name
        for action in actions:
            func_name = action.get("function_name", "")
            if func_name:
                if func_name not in function_names:
                    function_names[func_name] = []
                function_names[func_name].append(action.get("class_name", "Unknown"))
        
        # Find duplicates
        duplicates = {name: classes for name, classes in function_names.items() if len(classes) > 1}
        
        # Add warning message if duplicates exist - prepend to existing message
        if duplicates:
            warning_lines = [
                "IMPORTANT REMINDER: Multiple functions with the same name found!",
                "",
                "When using create_node_by_action_name() with these results,",
                "you MUST specify the 'class_name' parameter to avoid getting the wrong variant.",
                "",
                "Functions with duplicates:"
            ]
            
            for func_name, classes in duplicates.items():
                unique_classes = list(set(classes))  # Remove duplicates in class list
                warning_lines.append(f"  - '{func_name}' exists in: {', '.join(unique_classes)}")
            
            warning_lines.append("")
            warning_lines.append("Example:")
            first_dup = list(duplicates.keys())[0]
            first_class = list(set(duplicates[first_dup]))[0]
            warning_lines.append(f"  create_node_by_action_name(")
            warning_lines.append(f"      function_name=\"{first_dup}\",")
            warning_lines.append(f"      class_name=\"{first_class}\",  # ← REQUIRED!")
            warning_lines.append(f"      ...)")
            warning_lines.append("")
            warning_lines.append("=" * 70)
            warning_lines.append("")
            
            # Prepend warning to message
            original_message = result.get("message", "")
            result["message"] = "\n".join(warning_lines) + original_message
    
    return result

def get_actions_for_class_hierarchy(
    ctx: Context,
    class_name: str,
    search_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """Implementation for getting actions for a class hierarchy with search filtering."""
    params = {
        "class_name": class_name,
        "search_filter": search_filter,
        "max_results": max_results
    }
    
    command_result = send_unreal_command("get_actions_for_class", params)
    
    # Extract the actual result from the wrapper
    if "result" in command_result:
        result = command_result["result"]
    else:
        result = command_result
    
    # Check for duplicate function names and add warning if found
    if result.get("success") and "actions" in result:
        actions = result["actions"]
        function_names = {}
        
        # Count occurrences of each function name
        for action in actions:
            func_name = action.get("function_name", "")
            if func_name:
                if func_name not in function_names:
                    function_names[func_name] = []
                function_names[func_name].append(action.get("class_name", "Unknown"))
        
        # Find duplicates
        duplicates = {name: classes for name, classes in function_names.items() if len(classes) > 1}
        
        # Add warning message if duplicates exist - prepend to existing message
        if duplicates:
            warning_lines = [
                "IMPORTANT REMINDER: Multiple functions with the same name found!",
                "",
                "When using create_node_by_action_name() with these results,",
                "you MUST specify the 'class_name' parameter to avoid getting the wrong variant.",
                "",
                "Functions with duplicates:"
            ]
            
            for func_name, classes in duplicates.items():
                unique_classes = list(set(classes))  # Remove duplicates in class list
                warning_lines.append(f"  - '{func_name}' exists in: {', '.join(unique_classes)}")
            
            warning_lines.append("")
            warning_lines.append("Example:")
            first_dup = list(duplicates.keys())[0]
            first_class = list(set(duplicates[first_dup]))[0]
            warning_lines.append(f"  create_node_by_action_name(")
            warning_lines.append(f"      function_name=\"{first_dup}\",")
            warning_lines.append(f"      class_name=\"{first_class}\",  # ← REQUIRED!")
            warning_lines.append(f"      ...)")
            warning_lines.append("")
            warning_lines.append("=" * 70)
            warning_lines.append("")
            
            # Prepend warning to message
            original_message = result.get("message", "")
            result["message"] = "\n".join(warning_lines) + original_message
    
    return result

def search_blueprint_actions(
    ctx: Context,
    search_query: str,
    category: str = "",
    max_results: int = 50,
    blueprint_name: str = None
) -> Dict[str, Any]:
    """Implementation for searching Blueprint actions using keywords."""
    params = {
        "search_query": search_query,
        "category": category,
        "max_results": max_results
    }
    if blueprint_name:
        params["blueprint_name"] = blueprint_name
    
    command_result = send_unreal_command("search_blueprint_actions", params)
    
    # Extract the actual result from the wrapper
    if "result" in command_result:
        result = command_result["result"]
    else:
        result = command_result
    
    # Check for duplicate function names and add warning if found
    if result.get("success") and "actions" in result:
        actions = result["actions"]
        function_names = {}
        
        # Count occurrences of each function name
        for action in actions:
            func_name = action.get("function_name", "")
            if func_name:
                if func_name not in function_names:
                    function_names[func_name] = []
                function_names[func_name].append(action.get("class_name", "Unknown"))
        
        # Find duplicates
        duplicates = {name: classes for name, classes in function_names.items() if len(classes) > 1}
        
        # Add warning message if duplicates exist - prepend to existing message
        if duplicates:
            warning_lines = [
                "IMPORTANT REMINDER: Multiple functions with the same name found!",
                "",
                "When using create_node_by_action_name() with these results,",
                "you MUST specify the 'class_name' parameter to avoid getting the wrong variant.",
                "",
                "Functions with duplicates:"
            ]
            
            for func_name, classes in duplicates.items():
                unique_classes = list(set(classes))  # Remove duplicates in class list
                warning_lines.append(f"  - '{func_name}' exists in: {', '.join(unique_classes)}")
            
            warning_lines.append("")
            warning_lines.append("Example:")
            first_dup = list(duplicates.keys())[0]
            first_class = list(set(duplicates[first_dup]))[0]
            warning_lines.append(f"  create_node_by_action_name(")
            warning_lines.append(f"      function_name=\"{first_dup}\",")
            warning_lines.append(f"      class_name=\"{first_class}\",  # ← REQUIRED!")
            warning_lines.append(f"      ...)")
            warning_lines.append("")
            warning_lines.append("=" * 70)
            warning_lines.append("")
            
            # Prepend warning to message
            original_message = result.get("message", "")
            warning_message = "\n".join(warning_lines) + original_message
            result["message"] = warning_message
    
    return result

def get_node_pin_info(
    ctx: Context,
    node_name: str,
    pin_name: str
) -> Dict[str, Any]:
    """Implementation for inspecting Blueprint node pin connection details and compatibility."""
    params = {
        "node_name": node_name,
        "pin_name": pin_name
    }
    return send_unreal_command("get_node_pin_info", params)


def create_node_by_action_name(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    class_name: str = "",
    node_position: List[float] = None,
    target_graph: str = "EventGraph",
    **kwargs
) -> Dict[str, Any]:
    """Implementation for creating a blueprint node by discovered action/function name."""
    
    # Flatten if kwargs is double-wrapped (i.e., kwargs={'kwargs': {...}})
    if (
        len(kwargs) == 1 and
        'kwargs' in kwargs and
        isinstance(kwargs['kwargs'], dict)
    ):
        kwargs = kwargs['kwargs']
    
    # --- PATCH START ---
    if function_name in ("Get", "Set") and "variable_name" in kwargs:
        function_name = f"{function_name} {kwargs['variable_name']}"
        kwargs.pop("variable_name")
    # --- PATCH END ---
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name
    }
    
    if class_name:
        params["class_name"] = class_name
        
    if node_position is not None:
        # Convert List[float] to JSON string that C++ can parse
        params["node_position"] = json.dumps(node_position)
    
    # Build json_params payload that will be passed through to the C++ side.  It carries
    # any extra keyword arguments **and** the optional target_graph so the service layer
    # can pick it up in one place.

    extra_params: Dict[str, Any] = {}

    # IMPORTANT: target_graph must be in json_params for C++ to see it
    # Handle both direct parameter and kwargs versions
    final_target_graph = target_graph
    if "target_graph" in kwargs:
        final_target_graph = kwargs.pop("target_graph")
    
    # Always include target_graph (defaults to "EventGraph")
    params["target_graph"] = final_target_graph  # For handler compatibility
    extra_params["target_graph"] = final_target_graph  # For C++ service layer

    # Merge any additional kwargs supplied by the caller
    if kwargs:
        extra_params.update(kwargs)

    # Always provide json_params (even if empty) so the handler code doesn't hit a
    # missing-field branch and the tool signature stays consistent.
    params["json_params"] = json.dumps(extra_params)

    return send_unreal_command("create_node_by_action_name", params)


def find_in_blueprints(
    ctx: Context,
    search_query: str,
    search_type: str = "all",
    path: str = "/Game",
    max_results: int = 50,
    case_sensitive: bool = False
) -> Dict[str, Any]:
    """
    Search for function/variable/event usages across all blueprints.

    Similar to Unreal's "Find in Blueprints" feature, this searches through
    all blueprint graphs to find nodes matching the search query.

    Args:
        ctx: MCP context
        search_query: Text to search for (function names, variable names, event names, etc.)
        search_type: Type of nodes to search ("all", "function", "variable", "event", "comment", "custom")
        path: Content path to search in (default: "/Game")
        max_results: Maximum number of results to return (default: 50, max: 500)
        case_sensitive: Whether to perform case-sensitive search (default: False)

    Returns:
        Dict containing:
            - success: Whether the search succeeded
            - search_query: The search query used
            - search_type: The type filter applied
            - blueprints_searched: Number of blueprints searched
            - match_count: Number of matches found
            - matches: Array of match objects with:
                - blueprint_path: Full path to the blueprint
                - blueprint_name: Name of the blueprint
                - graph_name: Name of the graph containing the match
                - node_id: Unique node ID
                - node_title: Node's display title
                - node_class: Node class name
                - match_context: The text that matched
            - by_blueprint: Results grouped by blueprint for easier consumption
    """
    params = {
        "search_query": search_query,
        "search_type": search_type,
        "path": path,
        "max_results": max_results,
        "case_sensitive": case_sensitive
    }

    command_result = send_unreal_command("find_in_blueprints", params)

    # Extract the actual result from the wrapper
    if "result" in command_result:
        result = command_result["result"]
    else:
        result = command_result

    return result