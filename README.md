<div align="center">

# Model Context Protocol for Unreal Engine
<span style="color: #555555">unreal-mcp</span>

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.10%2B-yellow)](https://www.python.org)
[![Status](https://img.shields.io/badge/Status-Experimental-red)](https://github.com/TrishynVolodymyr/unreal-mcp)

</div>

Control **Unreal Engine 5.7** through natural language using AI assistants (Claude Desktop, Cursor, Windsurf) via the Model Context Protocol (MCP). Build games, create Blueprints, design UI, and manage assets using conversational commands.

## ⚠️ Experimental Status

This project is currently in an **EXPERIMENTAL** state. The API, functionality, and implementation details are subject to significant changes. While we encourage testing and feedback, please be aware that:

- Breaking changes may occur without notice
- Features may be incomplete or unstable
- Documentation may be outdated or missing
- Production use is not recommended at this time

## 🎬 Video Demonstrations

These videos showcase systems built entirely through AI assistants using unreal-mcp:

| Demo | AI Assistant | Implementation Plan |
|------|--------------|---------------------|
| [Simple Dialogue System](https://www.youtube.com/watch?v=qRZYDYq8np4) | ChatGPT 5.1 Codex Max | [dialogue-plan](Docs/Implementation-Examples/dialogue-plan/) |
| [Branching Dialogue System](https://www.youtube.com/watch?v=s8949Xcvyow) | Claude Opus 4.5 | [dialogue-branching-plan](Docs/Implementation-Examples/dialogue-branching-plan/) |
| [Inventory System](https://www.youtube.com/watch?v=psqTS8jX-NI) | Claude Opus 4.5 | [inventory-plan](Docs/Implementation-Examples/inventory-plan/) |
| [Lootable Extension](https://www.youtube.com/watch?v=VRl7xGooVSw) | Claude Opus 4.5 | [loot-plan](Docs/Implementation-Examples/loot-plan/) |
| [Niagara Fire Embers VFX](https://youtu.be/oaGBUfUnzkw) | Claude Opus 4.5 | Multi-skill workflow: VFX Reference Transcriber extracted frames from reference video → Niagara VFX Architect created initial effect → Asset State Extractor captured current emitter state to readable markdown → state fed back to VFX Architect for iterative refinement |

## 🌟 What You Can Do

Control Unreal Engine using natural language across these major areas:

### 🎮 Blueprint Development
Create and manage Blueprint classes, components, variables, and game logic:
- Create Blueprint classes (Actor, Pawn, Character, custom parent classes)
- Add and configure components (StaticMesh, Camera, Light, Physics, etc.)
- Manage variables, properties, interfaces, and custom functions
- Configure physics simulation, Pawn properties, and component hierarchies
- Discover available Blueprint actions dynamically from Unreal's action database
- Inspect component hierarchies and Blueprint inheritance chains

### 🎯 Visual Scripting & Node Graphs
Build gameplay logic through Blueprint visual scripting:
- Create event nodes (BeginPlay, Tick, input actions, custom events)
- Add function calls, variable references, and arithmetic operations
- Connect nodes with execution and data flow pins
- Smart node replacement preserving connections with auto-casting
- Graph manipulation (disconnect, delete, replace nodes)
- Find and inspect nodes in Blueprint graphs

### 🖼️ UI Development (UMG)
Design user interfaces with Widget Blueprints:
- Create Widget Blueprints with complex layouts
- Add UI components (Text, Button, Image, Slider, Checkbox, ProgressBar, etc.)
- Configure nested hierarchies with containers (ScrollBox, Border, Overlay, etc.)
- Bind events and properties for dynamic UI behavior
- Set placement, alignment, size, and styling
- Capture widget screenshots for AI visual inspection
- Export enhanced JSON metadata for widget layouts

### 📊 Data Management
Structure and manage game data efficiently:
- Create DataTables with custom row structs
- Add, update, delete, and query rows
- Map properties using GUID-based automation
- List row names and struct fields
- Create and update custom structs

### 🎬 Scene & Editor Control
Manage actors and the editor environment:
- Spawn and delete actors (primitives, lights, cameras, Blueprint instances)
- Set transforms (position, rotation, scale)
- Configure light properties (intensity, color, shadows, attenuation)
- Find actors by name patterns and wildcards
- Control viewport camera focus and orientation
- Query and modify actor properties

### 📁 Project Organization
Organize assets, inputs, and project structure:
- Create and manage content browser folders
- **Enhanced Input System** (UE 5.5+): Create Input Actions and Mapping Contexts
- Configure key mappings with modifier support (Shift, Ctrl, Alt, Cmd)
- List and query Input Actions and Mapping Contexts
- Create and inspect custom structs
- Manage project folder structure

**All capabilities accessible through conversational AI commands** - no need to learn complex editor interfaces or remember specific API calls.

## 📖 Documentation

### 🚀 [Quick Start Guide](Docs/Quick-Start-Guide.md)
**New to Unreal MCP?** Follow our 15-minute quick start guide to get up and running! Perfect for first-time users.

### 📚 [Complete Documentation](Docs/README.md)
Comprehensive guides for all tool categories:

- **[Blueprint Tools](Docs/Blueprint-Tools.md)** - Creating and managing Blueprint classes, components, and variables
- **[Blueprint Action Tools](Docs/Blueprint-Action-Tools.md)** - Discovering available Blueprint actions and creating nodes dynamically
- **[Editor Tools](Docs/Editor-Tools.md)** - Controlling actors, transforms, and scene management
- **[Node Tools](Docs/Node-Tools.md)** - Building Blueprint visual scripting logic and event chains
- **[UMG Tools](Docs/UMG-Tools.md)** - Creating user interfaces and interactive UI elements
- **[DataTable Tools](Docs/DataTable-Tools.md)** - Managing structured game data and tables
- **[Project Tools](Docs/Project-Tools.md)** - Organizing projects, input systems, and structs

Each guide includes natural language usage examples, advanced patterns, and real-world workflows.

## 🏗️ Architecture

The project uses a **dual-component synchronized architecture** enabling natural language control of Unreal Engine:

```
AI Assistant (Claude/Cursor/Windsurf)
    ↓ [MCP Protocol]
Python MCP Servers (7 specialized FastMCP servers)
    ↓ [TCP/JSON on localhost:55557]
C++ Plugin (UnrealMCP EditorSubsystem)
    ↓ [Direct Unreal Engine API]
Unreal Engine 5.7 Editor
```

### Core Components

**C++ Plugin** ([MCPGameProject/Plugins/UnrealMCP](MCPGameProject/Plugins/UnrealMCP/))
- **EditorSubsystem** with TCP server on localhost:55557
- **Command Dispatcher** routing requests to specialized handlers
- **Service Layer** implementing business logic (Blueprint, UMG, Node, DataTable, Editor, Project)
- **Factories** for type-safe object creation (Components, Widgets)
- **Modular Architecture** with strict 1000-line file size limit for maintainability

**Python MCP Servers** ([Python/](Python/))
- **7 Specialized Servers**: blueprint, editor, umg, node, datatable, project, blueprint_action
- Each server has dedicated tool implementations in `*_tools/` folders
- Shared utilities for TCP communication, Blueprint operations, UMG, and more
- FastMCP-based implementation of Model Context Protocol
- JSON serialization for cross-language communication

**Sample Project** ([MCPGameProject/](MCPGameProject/))
- UE 5.7 Blank Project template with UnrealMCP plugin pre-configured
- Ready-to-use environment for testing and development
- Includes build and launch scripts ([RebuildProject.bat](RebuildProject.bat), [LaunchProject.bat](LaunchProject.bat))

## 📂 Project Structure

```
unreal-mcp/
├── MCPGameProject/                    # Sample UE 5.7 project
│   ├── Plugins/UnrealMCP/            # C++ Plugin
│   │   ├── Source/UnrealMCP/
│   │   │   ├── Private/
│   │   │   │   ├── Commands/         # Command handlers by category
│   │   │   │   ├── Services/         # Business logic layer
│   │   │   │   ├── Factories/        # Object creation
│   │   │   │   └── Utils/            # Utilities
│   │   │   └── Public/
│   │   └── UnrealMCP.uplugin         # Plugin manifest
│   └── MCPGameProject.uproject       # Unreal project file
│
├── Python/                            # Python MCP servers
│   ├── blueprint_mcp_server.py       # Blueprint tools server
│   ├── editor_mcp_server.py          # Editor tools server
│   ├── umg_mcp_server.py             # UMG tools server
│   ├── node_mcp_server.py            # Node tools server
│   ├── datatable_mcp_server.py       # DataTable tools server
│   ├── project_mcp_server.py         # Project tools server
│   ├── blueprint_action_mcp_server.py # Blueprint action discovery server
│   ├── *_tools/                      # Tool implementations
│   ├── utils/                        # Shared utilities
│   └── scripts/                      # Test scripts
│
├── Docs/                              # Documentation
│   ├── README.md                     # Documentation index
│   ├── Blueprint-Tools.md
│   ├── Blueprint-Action-Tools.md
│   ├── Editor-Tools.md
│   ├── Node-Tools.md
│   ├── UMG-Tools.md
│   ├── DataTable-Tools.md
│   └── Project-Tools.md
│
├── RebuildProject.bat                # Build C++ plugin script
├── LaunchProject.bat                 # Launch Unreal Editor script
└── CLAUDE.md                         # Developer guidance for AI assistants
```

## 🚀 Quick Start

> **📘 New to Unreal MCP?** Follow our comprehensive [**Quick Start Guide**](Docs/Quick-Start-Guide.md) for a detailed 15-minute walkthrough with step-by-step instructions, troubleshooting, and example commands!

### Quick Setup Overview

### Prerequisites

| Requirement | Version | Notes |
|------------|---------|-------|
| **Unreal Engine** | 5.7 | Required for the sample project and plugin |
| **Python** | 3.10+ | For running MCP servers |
| **Visual Studio** | 2022+ | For building C++ plugin (Windows) |
| **MCP Client** | Latest | Claude Desktop, Cursor, or Windsurf |
| **uv** (optional) | Latest | Fast Python package manager (recommended) |

### Option 1: Use the Sample Project (Recommended)

The fastest way to get started is using the included `MCPGameProject`:

1. **Generate Visual Studio Files**
   ```powershell
   # Right-click MCPGameProject.uproject
   # Select "Generate Visual Studio project files"
   ```

2. **Build the Project**
   ```powershell
   # Open MCPGameProject.sln in Visual Studio
   # Set configuration to "Development Editor"
   # Build Solution (Ctrl+Shift+B)

   # OR use the included script:
   .\RebuildProject.bat
   ```

3. **Launch Unreal Editor**
   ```powershell
   .\LaunchProject.bat
   ```

### Option 2: Add Plugin to Existing Project

To use UnrealMCP in your own UE 5.7 project:

1. **Copy the Plugin**
   ```powershell
   # Copy MCPGameProject/Plugins/UnrealMCP to YourProject/Plugins/
   ```

2. **Enable the Plugin**
   - Launch Unreal Editor
   - Edit → Plugins
   - Search for "UnrealMCP" in the Editor category
   - Check the Enabled checkbox
   - Restart when prompted

3. **Build the Plugin**
   - Right-click `YourProject.uproject`
   - Select "Generate Visual Studio project files"
   - Open the `.sln` file
   - Build in **Development Editor** configuration

### Python Server Setup

1. **Install uv (recommended)**
   ```bash
   # Windows (PowerShell)
   powershell -c "irm https://astral.sh/uv/install.ps1 | iex"

   # macOS/Linux
   curl -LsSf https://astral.sh/uv/install.sh | sh
   ```

2. **Create Virtual Environment**
   ```bash
   cd Python
   uv venv
   ```

3. **Activate Virtual Environment**
   ```powershell
   # Windows
   .venv\Scripts\activate

   # macOS/Linux
   source .venv/bin/activate
   ```

4. **Install Dependencies**
   ```bash
   uv pip install -e .
   ```

For more details, see [Python/README.md](Python/README.md).

### Configure Your MCP Client

The 7 MCP servers need to be configured in your AI assistant. Below is the configuration template - **adjust the path** to match your installation directory.

#### Configuration Template

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
    },
    "umgMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "umg_mcp_server.py"]
    },
    "nodeMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "node_mcp_server.py"]
    },
    "datatableMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "datatable_mcp_server.py"]
    },
    "projectMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "project_mcp_server.py"]
    },
    "blueprintActionMCP": {
      "command": "uv",
      "args": ["--directory", "/path/to/unreal-mcp/Python", "run", "blueprint_action_mcp_server.py"]
    }
  }
}
```

Replace `/path/to/unreal-mcp/Python` with your actual path:
- Windows: `E:\\code\\unreal-mcp\\Python`
- macOS/Linux: `/home/user/unreal-mcp/Python`

#### Configuration File Locations

| MCP Client | Configuration Path | Example |
|------------|-------------------|---------|
| **Claude Desktop** | `~/.config/claude-desktop/mcp.json` | Windows: `%USERPROFILE%\.config\claude-desktop\mcp.json` |
| **Cursor** | Project root `.cursor/mcp.json` | In your working directory |
| **Windsurf** | `~/.config/windsurf/mcp.json` | Windows: `%USERPROFILE%\.config\windsurf\mcp.json` |

A complete example configuration is provided in [mcp.json](mcp.json).

### Verify Setup

1. **Ensure Unreal Editor is Running**
   - Launch the project with `LaunchProject.bat` or manually
   - The TCP server starts automatically on localhost:55557

2. **Test with Your AI Assistant**
   ```
   "Create a new Blueprint called BP_TestActor"
   "Add a cube component to BP_TestActor"
   "Spawn an instance of BP_TestActor at position 0,0,100"
   ```

3. **Check Logs**
   - Python logs: Check console output when MCP servers start
   - C++ logs: Located in `MCPGameProject/Saved/Logs/`

## 🎯 Example Usage

Once configured, you can control Unreal Engine using natural language:

**Blueprint Creation**
```
"Create a Blueprint called BP_Enemy that inherits from Character"
"Add a sphere collision component and a static mesh component"
"Set physics to simulate with gravity enabled"
```

**Visual Scripting**
```
"Add a BeginPlay event to BP_Enemy"
"Create a function call to PrintString that says 'Enemy spawned!'"
"Connect BeginPlay to the PrintString node"
```

**UI Development**
```
"Create a Widget Blueprint called WBP_HealthBar"
"Add a progress bar and set its size to 300x30"
"Add a text block above it that says 'Health'"
```

**Data Management**
```
"Create a struct called S_ItemData with Name (string) and Value (integer) fields"
"Create a DataTable called DT_Items using S_ItemData"
"Add 3 rows to DT_Items for different game items"
```

See the [Documentation](Docs/README.md) for comprehensive examples and advanced workflows.

## 🆕 Recent Improvements

The project has undergone significant refactoring for improved maintainability and performance:

- **Service Layer Modularization**: Large service files split into focused, specialized classes (Blueprint, UMG, Node services)
- **Node Creation Strategy Pattern**: Separate creator classes for different node types with intelligent action database integration
- **Enhanced UMG Features**: Widget screenshot capture for AI visual inspection, enhanced JSON metadata export for layouts
- **Graph Manipulation**: Smart node replacement with automatic connection preservation and type casting
- **Strict File Size Limits**: Maximum 1000 lines per C++ file for better code organization
- **Blueprint Action Discovery**: Dynamic action database querying for intelligent node creation

## 🤝 Contributing

Contributions are welcome! This project follows specific architectural patterns:

**For New Features:**
1. Review [CLAUDE.md](CLAUDE.md) for architecture guidelines
2. Implement both Python MCP tool and C++ command handler
3. Maintain synchronization between Python and C++ interfaces
4. Follow the service layer pattern for business logic
5. Keep files under 1000 lines

**Key Development Guidelines:**
- Python and C++ must remain synchronized (function signatures, JSON schemas)
- Use the service layer for business logic, commands only for validation
- Follow the established command registration pattern
- Add comprehensive documentation and examples
- Test with all supported MCP clients

See [MCPGameProject/Plugins/UnrealMCP/Documentation/](MCPGameProject/Plugins/UnrealMCP/Documentation/) for detailed developer documentation.

## 📚 Additional Resources

- **[Complete Documentation](Docs/README.md)** - Tool guides and examples
- **[CLAUDE.md](CLAUDE.md)** - Developer guidance for AI assistants
- **[Architecture Guide](MCPGameProject/Plugins/UnrealMCP/Documentation/Architecture_Guide.md)** - C++ plugin architecture
- **[Coding Style Guide](MCPGameProject/Plugins/UnrealMCP/Documentation/Coding_Style_Guide.md)** - Code standards and conventions

## 🐛 Troubleshooting

**Connection Issues:**
- Ensure Unreal Editor is running before starting AI assistant
- Verify TCP server started (check logs: "MCP TCP Server started on 127.0.0.1:55557")
- Check firewall settings for localhost:55557

**Build Errors:**
- Clean and rebuild: `RebuildProject.bat`
- Verify Unreal Engine 5.7 is installed correctly
- Check Visual Studio 2022 is installed with C++ workload

**MCP Server Issues:**
- Verify Python virtual environment is activated
- Check all dependencies installed: `uv pip install -e .`
- Review Python console for error messages
- Ensure correct path in MCP client configuration

## 🙏 Credits & Acknowledgments

### Original Project
This is a fork and continuation of the original [unreal-mcp](https://github.com/chongdashu/unreal-mcp) project by [chongdashu](https://github.com/chongdashu). The original repository provided the foundation for this natural language control system for Unreal Engine.

**Note:** The original project indicated MIT License in its README. This fork formally adopts and continues under the MIT License with proper attribution to the original author.

### Contributors
- **[asseti6](https://github.com/asseti6)** - Animation Blueprint support, batch actor operations, enhanced Material Instance tools, Niagara parameter helpers, DataAsset tools, runtime help system

### Built With
- [FastMCP](https://github.com/anthropics/fastmcp) - Python MCP framework by Anthropic
- [Model Context Protocol](https://modelcontextprotocol.io/) - AI integration standard
- [Unreal Engine 5.7](https://www.unrealengine.com/) - Epic Games' game engine

## 📝 License

MIT License - See [LICENSE](LICENSE) for full details.

This project is open source and free to use. You can use, modify, and distribute this software freely with minimal restrictions. See the LICENSE file for complete terms.

---

**Ready to control Unreal Engine with AI?** Follow the [Quick Start Guide](#-quick-start-guide) above and start building with natural language!