# Unreal MCP - Python Servers

Python MCP servers for interacting with **Unreal Engine 5.7** using the Model Context Protocol (MCP).

## Overview

This directory contains **7 specialized FastMCP servers** that provide natural language control of Unreal Engine:

| Server | File | Purpose |
|--------|------|---------|
| **Blueprint** | `blueprint_mcp_server.py` | Blueprint class creation and management |
| **Editor** | `editor_mcp_server.py` | Scene management, actors, transforms |
| **UMG** | `umg_mcp_server.py` | UI widget creation and manipulation |
| **Node** | `node_mcp_server.py` | Blueprint visual scripting logic |
| **DataTable** | `datatable_mcp_server.py` | Structured game data management |
| **Project** | `project_mcp_server.py` | Project organization and input systems |
| **Blueprint Action** | `blueprint_action_mcp_server.py` | Dynamic node discovery |

Each server communicates with the C++ plugin via **TCP on localhost:55557**.

## Quick Setup

### 1. Install Python Requirements

**Python 3.10+** is required.

### 2. Install uv (Recommended)

```bash
# Windows (PowerShell)
powershell -c "irm https://astral.sh/uv/install.ps1 | iex"

# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh
```

### 3. Create Virtual Environment

```bash
cd Python
uv venv
```

### 4. Activate Virtual Environment

```bash
# Windows
.venv\Scripts\activate

# macOS/Linux
source .venv/bin/activate
```

### 5. Install Dependencies

```bash
uv pip install -e .
```

## Configuration

### MCP Client Setup

Configure your AI assistant to use these servers. See the [main README](../README.md#configure-your-mcp-client) for complete configuration examples.

**Quick example** (adjust paths for your installation):

```json
{
  "mcpServers": {
    "blueprintMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "blueprint_mcp_server.py"]
    },
    "editorMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "editor_mcp_server.py"]
    }
    // ... add all 7 servers
  }
}
```

## Project Structure

```
Python/
├── *_mcp_server.py          # 7 MCP server entry points
├── *_tools/                 # Tool implementations
│   ├── blueprint_tools.py
│   ├── editor_tools.py
│   ├── umg_tools.py
│   ├── node_tools.py
│   ├── datatable_tools.py
│   ├── project_tools.py
│   └── blueprint_action_tools.py
├── utils/                   # Shared utilities
│   ├── unreal_connection_utils.py  # TCP communication
│   ├── blueprints/          # Blueprint utilities
│   ├── nodes/               # Node utilities
│   ├── umg/                 # UMG utilities
│   ├── datatable/           # DataTable utilities
│   ├── editor/              # Editor utilities
│   └── project/             # Project utilities
├── scripts/                 # Test scripts (direct TCP)
└── pyproject.toml          # Python dependencies

```

## Testing Scripts

The `scripts/` folder contains Python scripts for **direct TCP testing** without MCP:

```bash
# Make sure virtual environment is activated
.venv\Scripts\activate  # Windows
source .venv/bin/activate  # macOS/Linux

# Run test scripts
python scripts/test_blueprint.py
python scripts/test_editor.py
# ... etc
```

These scripts bypass the MCP layer and connect directly to the C++ plugin's TCP server (localhost:55557).

## Development

### Adding New Tools

To add new functionality:

1. **Python Side**: Add `@mcp.tool()` decorated function in appropriate `*_tools/*.py` file
2. **C++ Side**: Implement corresponding command handler in C++ plugin
3. **Synchronization**: Ensure function signatures and JSON schemas match exactly

See [CLAUDE.md](../CLAUDE.md) for detailed development guidelines.

### Architecture

```
AI Assistant (Claude/Cursor/Windsurf)
    ↓ [MCP Protocol]
Python MCP Servers (7 servers)
    ↓ [TCP/JSON on localhost:55557]
C++ Plugin (UnrealMCP)
    ↓ [Direct Unreal Engine API]
Unreal Engine 5.7
```

Each Python tool sends JSON commands via TCP to the C++ plugin, which executes them and returns JSON responses.

## Troubleshooting

### Connection Failed

**Problem**: Cannot connect to Unreal Engine

**Solutions**:
- Ensure Unreal Editor is running with the MCPGameProject open
- Check Output Log (Window → Developer Tools → Output Log) for "MCP TCP Server started on 127.0.0.1:55557"
- Verify no firewall is blocking localhost:55557

### Import Errors

**Problem**: Module not found errors

**Solutions**:
- Ensure virtual environment is activated (you should see `.venv` in your terminal prompt)
- Reinstall dependencies: `uv pip install -e .`
- Try deleting `.venv` folder and recreating: `uv venv && uv pip install -e .`

### MCP Server Not Starting

**Problem**: AI assistant can't find the MCP servers

**Solutions**:
- Verify the path in your MCP configuration file is correct
- Use absolute paths (not relative)
- Windows: Use double backslashes `C:\\path\\to\\Python`
- macOS/Linux: Use forward slashes `/path/to/Python`
- Restart your AI assistant after changing configuration

## Documentation

- **[Main README](../README.md)** - Project overview and setup
- **[Quick Start Guide](../Docs/Quick-Start-Guide.md)** - 15-minute tutorial
- **[Complete Documentation](../Docs/README.md)** - All tool guides
- **[CLAUDE.md](../CLAUDE.md)** - Architecture and development guide

## Dependencies

Core dependencies (see `pyproject.toml` for full list):

- **FastMCP** (>=0.2.0) - MCP server framework
- **MCP** (>=1.4.1) - Model Context Protocol
- **Pydantic** (>=2.6.1) - Data validation
- **FastAPI** - HTTP API framework
- **Uvicorn** - ASGI server

All dependencies are automatically installed via `uv pip install -e .` 