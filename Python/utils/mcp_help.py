"""
MCP Help System - Provides introspection and documentation for MCP tools.

This module provides utilities to extract documentation from registered MCP tools
and return it in a structured format for AI assistants and users.
"""

import inspect
import re
from typing import Dict, List, Any, Optional, Callable


def parse_docstring(docstring: str) -> Dict[str, Any]:
    """
    Parse a docstring into structured sections.

    Extracts:
    - description: The main description text
    - args: Parameter documentation
    - returns: Return value documentation
    - examples: Usage examples
    - notes: Additional notes
    - sections: Any custom uppercase sections (e.g., TYPE RESOLUTION, SUPPORTED FRIENDLY NAMES)
    """
    if not docstring:
        return {"description": "", "args": {}, "returns": "", "examples": [], "notes": [], "sections": {}}

    lines = docstring.strip().split('\n')
    result = {
        "description": "",
        "args": {},
        "returns": "",
        "examples": [],
        "notes": [],
        "sections": {}
    }

    # State machine for parsing
    current_section = "description"
    current_arg = None
    current_content = []
    description_lines = []

    # Custom sections we want to capture (uppercase headers with colons)
    custom_section_pattern = re.compile(r'^(\s*)([A-Z][A-Z\s]+):?\s*$')
    # Arg pattern: "argname: description" or "argname (type): description"
    arg_pattern = re.compile(r'^\s+(\w+)(?:\s*\([^)]+\))?:\s*(.*)$')

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Check for standard sections
        if stripped in ('Args:', 'Arguments:', 'Parameters:'):
            current_section = "args"
            current_arg = None
            i += 1
            continue
        elif stripped in ('Returns:', 'Return:'):
            current_section = "returns"
            i += 1
            continue
        elif stripped in ('Examples:', 'Example:'):
            current_section = "examples"
            i += 1
            continue
        elif stripped in ('Note:', 'Notes:'):
            current_section = "notes"
            i += 1
            continue
        elif stripped in ('Raises:', 'Raise:'):
            current_section = "raises"
            i += 1
            continue

        # Check for custom uppercase sections (like TYPE RESOLUTION:)
        custom_match = custom_section_pattern.match(line)
        if custom_match and current_section == "description":
            section_name = custom_match.group(2).strip().rstrip(':')
            current_section = f"custom_{section_name}"
            result["sections"][section_name] = []
            i += 1
            continue

        # Process content based on current section
        if current_section == "description":
            if stripped:
                description_lines.append(stripped)

        elif current_section == "args":
            arg_match = arg_pattern.match(line)
            if arg_match:
                current_arg = arg_match.group(1)
                result["args"][current_arg] = arg_match.group(2).strip()
            elif current_arg and stripped and line.startswith('                '):
                # Continuation of previous arg description
                result["args"][current_arg] += " " + stripped

        elif current_section == "returns":
            if stripped:
                if result["returns"]:
                    result["returns"] += " " + stripped
                else:
                    result["returns"] = stripped

        elif current_section == "examples":
            if stripped:
                result["examples"].append(stripped)

        elif current_section == "notes":
            if stripped:
                result["notes"].append(stripped)

        elif current_section.startswith("custom_"):
            section_name = current_section[7:]  # Remove "custom_" prefix
            if stripped:
                result["sections"][section_name].append(stripped)

        i += 1

    result["description"] = " ".join(description_lines)

    # Convert section lists to strings for cleaner output
    for section_name, section_lines in result["sections"].items():
        result["sections"][section_name] = "\n".join(section_lines)

    return result


def get_function_signature(func: Callable) -> Dict[str, Any]:
    """
    Extract function signature information.

    Returns:
        Dict with 'parameters' containing name, type, required, default for each param
    """
    sig = inspect.signature(func)
    params = {}

    for name, param in sig.parameters.items():
        if name == 'ctx':  # Skip context parameter
            continue

        param_info = {
            "required": param.default is inspect.Parameter.empty,
        }

        # Get type annotation if available
        if param.annotation is not inspect.Parameter.empty:
            type_name = getattr(param.annotation, '__name__', str(param.annotation))
            # Clean up typing module types
            type_name = str(type_name).replace('typing.', '')
            param_info["type"] = type_name

        # Get default value if available
        if param.default is not inspect.Parameter.empty:
            param_info["default"] = param.default

        params[name] = param_info

    return {"parameters": params}


def get_tool_help(tool_func: Callable, include_full_docstring: bool = False) -> Dict[str, Any]:
    """
    Get comprehensive help for a single tool function.

    Args:
        tool_func: The tool function to document
        include_full_docstring: If True, include the raw docstring

    Returns:
        Dict with tool documentation
    """
    docstring = inspect.getdoc(tool_func) or ""
    parsed = parse_docstring(docstring)
    sig_info = get_function_signature(tool_func)

    # Merge parameter info from signature with docstring descriptions
    for param_name, param_info in sig_info["parameters"].items():
        if param_name in parsed["args"]:
            param_info["description"] = parsed["args"][param_name]

    result = {
        "name": tool_func.__name__,
        "description": parsed["description"],
        "parameters": sig_info["parameters"],
        "returns": parsed["returns"],
        "examples": parsed["examples"],
    }

    # Add custom sections if present
    if parsed["sections"]:
        result["sections"] = parsed["sections"]

    if parsed["notes"]:
        result["notes"] = parsed["notes"]

    if include_full_docstring:
        result["full_docstring"] = docstring

    return result


class MCPHelpRegistry:
    """
    Registry for MCP tools that provides help and documentation.
    """

    def __init__(self):
        self._tools: Dict[str, Callable] = {}
        self._categories: Dict[str, List[str]] = {}

    def register(self, func: Callable, category: str = "general") -> None:
        """Register a tool function."""
        self._tools[func.__name__] = func
        if category not in self._categories:
            self._categories[category] = []
        if func.__name__ not in self._categories[category]:
            self._categories[category].append(func.__name__)

    def get_tool_names(self) -> List[str]:
        """Get list of all registered tool names."""
        return list(self._tools.keys())

    def get_tools_by_category(self) -> Dict[str, List[str]]:
        """Get tools organized by category."""
        return self._categories.copy()

    def get_help(self, tool_name: str = None, include_full_docstring: bool = False) -> Dict[str, Any]:
        """
        Get help for a specific tool or list all tools.

        Args:
            tool_name: Name of tool to get help for, or None for list of all tools
            include_full_docstring: Include the complete raw docstring

        Returns:
            If tool_name is None: {"tools": [...], "categories": {...}}
            If tool_name specified: Full tool documentation
        """
        if tool_name is None:
            return {
                "tools": self.get_tool_names(),
                "categories": self.get_tools_by_category(),
                "usage": "Call get_mcp_help(tool_name='<name>') for detailed documentation"
            }

        if tool_name not in self._tools:
            return {
                "error": f"Tool '{tool_name}' not found",
                "available_tools": self.get_tool_names()
            }

        return get_tool_help(self._tools[tool_name], include_full_docstring)


# Global registry instance
_help_registry = MCPHelpRegistry()


def register_tool_for_help(func: Callable, category: str = "general") -> Callable:
    """
    Decorator/function to register a tool for the help system.

    Can be used as a decorator or called directly after tool registration.
    """
    _help_registry.register(func, category)
    return func


def get_mcp_help(tool_name: str = None, include_full_docstring: bool = False) -> Dict[str, Any]:
    """
    Get help for MCP tools.

    Args:
        tool_name: Specific tool name, or None to list all tools
        include_full_docstring: Include complete raw docstring in output

    Returns:
        Tool documentation or list of available tools
    """
    return _help_registry.get_help(tool_name, include_full_docstring)


def get_help_registry() -> MCPHelpRegistry:
    """Get the global help registry instance."""
    return _help_registry
