# ComfyUI MCP Server

AI-powered control of ComfyUI workflows through the Model Context Protocol (MCP).

## Prerequisites

- ComfyUI running locally on `http://127.0.0.1:8188`
- Python 3.10+

## MCP Config

Add to your `.mcp.json`:

```json
"comfyuiMCP": {
  "command": "uv",
  "args": [
    "--directory",
    "E:\\code\\unreal-mcp\\Python\\ThirdParty\\comfyui",
    "run",
    "comfyui_mcp_server.py"
  ]
}
```

## Available Tools

### Node Discovery
- `search_comfyui_nodes` - Search available nodes by name or category
- `get_comfyui_node_info` - Get detailed info about a node (inputs, outputs, types)

### Workflow Building
- `new_comfyui_workflow` - Create a new empty workflow
- `create_comfyui_node` - Add a node to the workflow
- `set_comfyui_node_inputs` - Set input values on a node
- `connect_comfyui_nodes` - Connect nodes together (batch)
- `delete_comfyui_node` - Remove a node

### Workflow Analysis
- `get_comfyui_workflow_state` - View workflow (full, flow, orphans, summary modes)

### Execution
- `execute_comfyui_workflow` - Queue workflow for execution
- `get_comfyui_execution_status` - Check execution progress
- `interrupt_comfyui_execution` - Stop current execution

## Workflow State Modes

- **full**: All nodes with complete input/output details
- **flow**: Connection graph showing data flow (sources, sinks, connections)
- **orphans**: Nodes with no connections (isolated/unused nodes)
- **summary**: Statistics (node count, connection count, types used)
