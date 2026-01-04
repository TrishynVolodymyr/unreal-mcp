#!/usr/bin/env python3
"""
ComfyUI MCP Server - AI control of ComfyUI workflows.

This server provides tools for:
- Searching and inspecting available nodes
- Building workflows programmatically
- Executing workflows and tracking results
"""

import json
import sys
from pathlib import Path

# Add this directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from typing import Any, Dict, List

from mcp.server.fastmcp import FastMCP

from utils.config import config
from utils.connection import get_connection
from utils.workflow_state import get_workflow_state, new_workflow

mcp = FastMCP("comfyuiMCP")


# =============================================================================
# Tool 1: Search Available Nodes
# =============================================================================

@mcp.tool()
async def search_comfyui_nodes(
    query: str = "",
    category: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """
    Search available ComfyUI nodes.

    Args:
        query: Text to search in node names (case-insensitive). Leave empty to list all.
        category: Filter by category path (e.g., "loaders", "sampling", "conditioning").
                  Categories are hierarchical, separated by "/".
        max_results: Maximum number of results to return (default: 50)

    Returns:
        - success: Whether the search succeeded
        - count: Number of matching nodes
        - nodes: List of matching nodes with name, category, and brief description

    Example:
        search_comfyui_nodes(query="sampler")
        search_comfyui_nodes(category="loaders")
    """
    conn = get_connection()
    object_info = await conn.get_object_info()

    if object_info.get("success") is False:
        return object_info

    results = []
    query_lower = query.lower() if query else ""
    category_lower = category.lower() if category else ""

    for node_name, node_data in object_info.items():
        # Skip error responses
        if node_name in ["success", "error"]:
            continue

        # Get node category
        node_category = node_data.get("category", "")

        # Apply query filter
        if query_lower and query_lower not in node_name.lower():
            continue

        # Apply category filter
        if category_lower and category_lower not in node_category.lower():
            continue

        # Get input count for brief description
        input_required = node_data.get("input", {}).get("required", {})
        input_optional = node_data.get("input", {}).get("optional", {})
        output_count = len(node_data.get("output", []))

        results.append({
            "name": node_name,
            "category": node_category,
            "inputs": len(input_required) + len(input_optional),
            "outputs": output_count,
            "description": node_data.get("description", "")
        })

        if len(results) >= max_results:
            break

    return {
        "success": True,
        "count": len(results),
        "total_available": len(object_info) - 2 if "success" in object_info else len(object_info),
        "nodes": results
    }


# =============================================================================
# Tool 2: Get Node Information
# =============================================================================

@mcp.tool()
async def get_comfyui_node_info(node_type: str) -> Dict[str, Any]:
    """
    Get detailed information about a specific ComfyUI node type.

    Args:
        node_type: The node class name (e.g., "KSampler", "CheckpointLoaderSimple")

    Returns:
        - success: Whether the node was found
        - name: The node class name
        - category: Node category path
        - description: Node description
        - inputs: List of inputs with name, type, required, default_value, and options
        - outputs: List of outputs with name and type

    Example:
        get_comfyui_node_info("KSampler")
        get_comfyui_node_info("CheckpointLoaderSimple")
    """
    conn = get_connection()
    object_info = await conn.get_object_info()

    if object_info.get("success") is False:
        return object_info

    if node_type not in object_info:
        return {
            "success": False,
            "error": f"Node type '{node_type}' not found. Use search_comfyui_nodes to find available nodes."
        }

    node_data = object_info[node_type]

    # Parse inputs
    inputs = []
    input_required = node_data.get("input", {}).get("required", {})
    input_optional = node_data.get("input", {}).get("optional", {})

    for name, config in input_required.items():
        input_info = _parse_input_config(name, config, required=True)
        inputs.append(input_info)

    for name, config in input_optional.items():
        input_info = _parse_input_config(name, config, required=False)
        inputs.append(input_info)

    # Parse outputs
    outputs = []
    output_types = node_data.get("output", [])
    output_names = node_data.get("output_name", output_types)

    for i, (out_type, out_name) in enumerate(zip(output_types, output_names)):
        outputs.append({
            "index": i,
            "name": out_name,
            "type": out_type
        })

    return {
        "success": True,
        "name": node_type,
        "category": node_data.get("category", ""),
        "description": node_data.get("description", ""),
        "inputs": inputs,
        "outputs": outputs
    }


def _parse_input_config(name: str, config: Any, required: bool) -> Dict[str, Any]:
    """Parse a node input configuration into a standardized format."""
    result = {
        "name": name,
        "required": required
    }

    if not config:
        result["type"] = "unknown"
        return result

    # Config can be a list like ["INT", {"default": 20, "min": 1, "max": 100}]
    if isinstance(config, (list, tuple)):
        if len(config) >= 1:
            type_info = config[0]

            # Type can be a string or a list of options (combo)
            if isinstance(type_info, list):
                result["type"] = "COMBO"
                result["options"] = type_info
            else:
                result["type"] = str(type_info)

            # Parse additional config options
            if len(config) >= 2 and isinstance(config[1], dict):
                extra = config[1]
                if "default" in extra:
                    result["default"] = extra["default"]
                if "min" in extra:
                    result["min"] = extra["min"]
                if "max" in extra:
                    result["max"] = extra["max"]
                if "step" in extra:
                    result["step"] = extra["step"]
                if "multiline" in extra:
                    result["multiline"] = extra["multiline"]
    else:
        result["type"] = str(config)

    return result


# =============================================================================
# Tool 3: Create Node
# =============================================================================

@mcp.tool()
async def create_comfyui_node(
    node_type: str,
    inputs: Dict[str, Any] = None,
    position_x: float = 0,
    position_y: float = 0
) -> Dict[str, Any]:
    """
    Create a new node in the current workflow.

    Args:
        node_type: The node class name (e.g., "KSampler", "CheckpointLoaderSimple")
        inputs: Optional dict of initial input values (e.g., {"seed": 12345, "steps": 20})
        position_x: X position in the graph (for visualization)
        position_y: Y position in the graph (for visualization)

    Returns:
        - success: Whether the node was created
        - node_id: The generated node ID (use this for connections)
        - class_type: The node type
        - inputs: Current input configuration

    Example:
        create_comfyui_node("CheckpointLoaderSimple")
        create_comfyui_node("KSampler", inputs={"seed": 12345, "steps": 20})
    """
    # Validate node type exists
    conn = get_connection()
    object_info = await conn.get_object_info()

    if object_info.get("success") is False:
        return object_info

    if node_type not in object_info:
        return {
            "success": False,
            "error": f"Node type '{node_type}' not found. Use search_comfyui_nodes to find available nodes."
        }

    # Create the node
    workflow = get_workflow_state()
    node = workflow.add_node(
        class_type=node_type,
        inputs=inputs or {},
        position=[position_x, position_y]
    )

    return {
        "success": True,
        "node_id": node.node_id,
        "class_type": node.class_type,
        "inputs": node.inputs,
        "position": node.position,
        "message": f"Created {node_type} node with ID {node.node_id}"
    }


# =============================================================================
# Tool 4: Set Node Inputs
# =============================================================================

@mcp.tool()
async def set_comfyui_node_inputs(
    node_id: str,
    inputs: Dict[str, Any]
) -> Dict[str, Any]:
    """
    Set input values on a node.

    Args:
        node_id: The node ID (from create_comfyui_node)
        inputs: Dict of input values. For connections, use list format: ["source_node_id", output_index]

    Returns:
        - success: Whether inputs were set
        - node_id: The node ID
        - inputs: Updated input configuration

    Example:
        # Set scalar values
        set_comfyui_node_inputs("3", {"seed": 12345, "steps": 20, "cfg": 7.5})

        # Set a connection (connect node 1's output 0 to this node's "model" input)
        set_comfyui_node_inputs("3", {"model": ["1", 0]})
    """
    workflow = get_workflow_state()
    node = workflow.get_node(node_id)

    if not node:
        return {
            "success": False,
            "error": f"Node '{node_id}' not found in current workflow"
        }

    # Set each input
    for input_name, value in inputs.items():
        workflow.set_input(node_id, input_name, value)

    # Get updated node
    node = workflow.get_node(node_id)

    return {
        "success": True,
        "node_id": node_id,
        "class_type": node.class_type,
        "inputs": node.inputs
    }


# =============================================================================
# Tool 5: Connect Nodes
# =============================================================================

@mcp.tool()
async def connect_comfyui_nodes(
    connections: List[Dict[str, Any]]
) -> Dict[str, Any]:
    """
    Connect nodes together (batch operation).

    Args:
        connections: List of connection objects, each with:
            - source_node: Source node ID
            - source_output: Output index on source node (usually 0)
            - target_node: Target node ID
            - target_input: Input name on target node

    Returns:
        - success: Whether all connections succeeded
        - connected: List of successful connections
        - errors: List of failed connections with error messages

    Example:
        connect_comfyui_nodes([
            {"source_node": "1", "source_output": 0, "target_node": "3", "target_input": "model"},
            {"source_node": "1", "source_output": 1, "target_node": "2", "target_input": "clip"}
        ])
    """
    workflow = get_workflow_state()

    connected = []
    errors = []

    for conn in connections:
        source_node = str(conn.get("source_node", ""))
        source_output = conn.get("source_output", 0)
        target_node = str(conn.get("target_node", ""))
        target_input = conn.get("target_input", "")

        # Validate
        if not source_node or not target_node or not target_input:
            errors.append({
                "connection": conn,
                "error": "Missing required fields (source_node, source_output, target_node, target_input)"
            })
            continue

        if not workflow.get_node(source_node):
            errors.append({
                "connection": conn,
                "error": f"Source node '{source_node}' not found"
            })
            continue

        if not workflow.get_node(target_node):
            errors.append({
                "connection": conn,
                "error": f"Target node '{target_node}' not found"
            })
            continue

        # Create connection
        success = workflow.connect_nodes(
            source_node, source_output, target_node, target_input
        )

        if success:
            connected.append({
                "source_node": source_node,
                "source_output": source_output,
                "target_node": target_node,
                "target_input": target_input
            })
        else:
            errors.append({
                "connection": conn,
                "error": "Failed to create connection"
            })

    return {
        "success": len(errors) == 0,
        "connected_count": len(connected),
        "error_count": len(errors),
        "connected": connected,
        "errors": errors if errors else None
    }


# =============================================================================
# Tool 6: Get Workflow State
# =============================================================================

@mcp.tool()
async def get_comfyui_workflow_state(
    mode: str = "full"
) -> Dict[str, Any]:
    """
    Get current workflow state for visualization.

    Args:
        mode: Analysis mode:
            - "full": All nodes with their inputs/outputs (default)
            - "flow": Connection graph showing which pins connect to which
            - "orphans": Nodes with no connections (isolated nodes)
            - "summary": Statistics (node count, connection count, types used)

    Returns:
        Workflow state based on requested mode

    Example:
        get_comfyui_workflow_state()  # Full state
        get_comfyui_workflow_state("flow")  # Connection analysis
        get_comfyui_workflow_state("orphans")  # Find disconnected nodes
    """
    workflow = get_workflow_state()

    if mode == "full":
        return {
            "success": True,
            "workflow_name": workflow.name,
            "nodes": workflow.get_all_nodes(),
            "connections": workflow.get_connections()
        }

    elif mode == "flow":
        flow = workflow.get_flow_analysis()
        return {
            "success": True,
            "workflow_name": workflow.name,
            **flow
        }

    elif mode == "orphans":
        orphans = workflow.get_orphan_nodes()
        return {
            "success": True,
            "workflow_name": workflow.name,
            "orphan_count": len(orphans),
            "orphans": orphans
        }

    elif mode == "summary":
        return {
            "success": True,
            **workflow.get_summary()
        }

    else:
        return {
            "success": False,
            "error": f"Unknown mode '{mode}'. Valid modes: full, flow, orphans, summary"
        }


# =============================================================================
# Tool 7: Delete Node
# =============================================================================

@mcp.tool()
async def delete_comfyui_node(node_id: str) -> Dict[str, Any]:
    """
    Delete a node from the workflow.

    This also removes any connections to/from the deleted node.

    Args:
        node_id: The node ID to delete

    Returns:
        - success: Whether the node was deleted
        - deleted_connections: List of connections that were removed

    Example:
        delete_comfyui_node("3")
    """
    workflow = get_workflow_state()

    success, deleted_connections = workflow.remove_node(node_id)

    if not success:
        return {
            "success": False,
            "error": f"Node '{node_id}' not found in current workflow"
        }

    return {
        "success": True,
        "node_id": node_id,
        "deleted_connections": deleted_connections,
        "message": f"Deleted node {node_id}" + (
            f" and {len(deleted_connections)} connection(s)" if deleted_connections else ""
        )
    }


# =============================================================================
# Tool 8: New Workflow
# =============================================================================

@mcp.tool()
async def new_comfyui_workflow(name: str = "Untitled") -> Dict[str, Any]:
    """
    Create a new empty workflow (clears current state).

    Use this to start fresh for experiments. The previous workflow is discarded.

    Args:
        name: Optional name for the workflow

    Returns:
        - success: Whether the workflow was created
        - workflow_name: The workflow name

    Example:
        new_comfyui_workflow("My Test Workflow")
    """
    workflow = new_workflow(name)

    return {
        "success": True,
        "workflow_name": workflow.name,
        "message": f"Created new empty workflow '{name}'"
    }


# =============================================================================
# Tool 9: Save Workflow to File
# =============================================================================

@mcp.tool()
async def save_comfyui_workflow_to_file(
    file_path: str,
    format: str = "ui"
) -> Dict[str, Any]:
    """
    Save the current workflow to a JSON file that can be loaded in ComfyUI's UI.

    Args:
        file_path: Full path where to save the workflow (e.g., "C:/workflows/my_workflow.json")
            - If just a filename (e.g., "my_workflow.json"), saves to ComfyUI's workflows folder
            - Requires COMFYUI_PATH in .env for automatic workflow folder detection
        format: Export format:
            - "ui": ComfyUI UI format (default) - load via ComfyUI's Load button
            - "api": API prompt format - for programmatic use

    Returns:
        - success: Whether the file was saved
        - file_path: Path where the file was saved
        - node_count: Number of nodes in the workflow

    Example:
        save_comfyui_workflow_to_file("my_workflow.json")  # Saves to ComfyUI workflows folder
        save_comfyui_workflow_to_file("C:/custom/path/workflow.json")  # Custom path
    """
    workflow = get_workflow_state()

    if format == "api":
        workflow_json = workflow.get_workflow_json()
    else:
        # Fetch object_info to get correct widget order
        conn = get_connection()
        object_info = await conn.get_object_info()
        if object_info.get("success") is False:
            object_info = None  # Fall back to no widget values
        workflow_json = workflow.get_ui_workflow_json(object_info)

    if not workflow_json or (format == "api" and len(workflow_json) == 0):
        return {
            "success": False,
            "error": "Workflow is empty. Add nodes before saving."
        }

    try:
        path = Path(file_path)

        # If just a filename (no directory), save to ComfyUI workflows folder
        if path.parent == Path(".") or str(path.parent) == ".":
            workflows_dir = config.workflows_path
            if workflows_dir:
                path = workflows_dir / path.name
            else:
                return {
                    "success": False,
                    "error": "COMFYUI_PATH not configured in .env. Please set it or provide a full file path."
                }

        # Ensure .json extension
        if not path.suffix.lower() == ".json":
            path = path.with_suffix(".json")

        # Ensure parent directory exists
        path.parent.mkdir(parents=True, exist_ok=True)

        with open(path, 'w', encoding='utf-8') as f:
            json.dump(workflow_json, f, indent=2)

        node_count = len(workflow_json) if format == "api" else len(workflow_json.get("nodes", []))

        return {
            "success": True,
            "file_path": str(path.absolute()),
            "format": format,
            "node_count": node_count,
            "message": f"Saved workflow to {path.absolute()}"
        }
    except Exception as e:
        return {
            "success": False,
            "error": f"Failed to save workflow: {str(e)}"
        }


# =============================================================================
# Tool 10: Execute Workflow
# =============================================================================

@mcp.tool()
async def execute_comfyui_workflow() -> Dict[str, Any]:
    """
    Queue the current workflow for execution in ComfyUI.

    The workflow must have all required inputs connected.

    Returns:
        - success: Whether the workflow was queued
        - prompt_id: ID to track execution (use with get_execution_status)
        - workflow_json: The workflow that was submitted

    Example:
        execute_comfyui_workflow()
    """
    workflow = get_workflow_state()
    workflow_json = workflow.get_workflow_json()

    if not workflow_json:
        return {
            "success": False,
            "error": "Workflow is empty. Add nodes before executing."
        }

    conn = get_connection()
    result = await conn.queue_prompt(workflow_json)

    if result.get("error"):
        return {
            "success": False,
            "error": result.get("error"),
            "workflow_json": workflow_json
        }

    return {
        "success": True,
        "prompt_id": result.get("prompt_id"),
        "number": result.get("number"),
        "node_errors": result.get("node_errors"),
        "message": "Workflow queued for execution"
    }


# =============================================================================
# Bonus Tool: Get Execution Status
# =============================================================================

@mcp.tool()
async def get_comfyui_execution_status(prompt_id: str = None) -> Dict[str, Any]:
    """
    Get the status of a workflow execution.

    Args:
        prompt_id: The prompt ID from execute_comfyui_workflow.
                  If not provided, returns queue status.

    Returns:
        - success: Whether the request succeeded
        - status: Execution status (queued, running, completed, error)
        - outputs: Output data if completed

    Example:
        get_comfyui_execution_status("abc123")
    """
    conn = get_connection()

    if prompt_id:
        history = await conn.get_history(prompt_id)

        if history.get("success") is False:
            return history

        if prompt_id in history:
            prompt_data = history[prompt_id]
            outputs = prompt_data.get("outputs", {})

            return {
                "success": True,
                "prompt_id": prompt_id,
                "status": "completed" if outputs else "processing",
                "outputs": outputs
            }
        else:
            # Check if still in queue
            queue = await conn.get_queue()
            queue_running = queue.get("queue_running", [])
            queue_pending = queue.get("queue_pending", [])

            for item in queue_running:
                if len(item) > 1 and item[1] == prompt_id:
                    return {
                        "success": True,
                        "prompt_id": prompt_id,
                        "status": "running"
                    }

            for item in queue_pending:
                if len(item) > 1 and item[1] == prompt_id:
                    return {
                        "success": True,
                        "prompt_id": prompt_id,
                        "status": "queued"
                    }

            return {
                "success": True,
                "prompt_id": prompt_id,
                "status": "not_found",
                "message": "Prompt not found in history or queue"
            }
    else:
        # Return queue status
        queue = await conn.get_queue()

        if queue.get("success") is False:
            return queue

        return {
            "success": True,
            "running_count": len(queue.get("queue_running", [])),
            "pending_count": len(queue.get("queue_pending", [])),
            "queue_running": queue.get("queue_running", []),
            "queue_pending": queue.get("queue_pending", [])
        }


# =============================================================================
# Bonus Tool: Interrupt Execution
# =============================================================================

@mcp.tool()
async def interrupt_comfyui_execution() -> Dict[str, Any]:
    """
    Interrupt the currently running workflow execution.

    Returns:
        - success: Whether the interrupt was sent

    Example:
        interrupt_comfyui_execution()
    """
    conn = get_connection()
    result = await conn.interrupt()

    if result.get("success") is False:
        return result

    return {
        "success": True,
        "message": "Interrupt signal sent to ComfyUI"
    }


if __name__ == "__main__":
    mcp.run(transport='stdio')
