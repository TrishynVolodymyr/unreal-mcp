<div align="center">

# Model Context Protocol for Unreal Engine
<span style="color: #555555">unreal-mcp</span>

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.12%2B-yellow)](https://www.python.org)
[![Status](https://img.shields.io/badge/Status-Experimental-red)](https://github.com/chongdashu/unreal-mcp)

</div>

This project enables AI assistant clients like Cursor, Windsurf and Claude Desktop to control Unreal Engine through natural language using the Model Context Protocol (MCP).

## ⚠️ Experimental Status

This project is currently in an **EXPERIMENTAL** state. The API, functionality, and implementation details are subject to significant changes. While we encourage testing and feedback, please be aware that:

- Breaking changes may occur without notice
- Features may be incomplete or unstable
- Documentation may be outdated or missing
- Production use is not recommended at this time

## 🌟 Overview

The Unreal MCP integration provides comprehensive tools for controlling Unreal Engine through natural language:

| Category | Capabilities |
|----------|-------------|
| **Actor Management** | • Create and delete actors (cubes, spheres, lights, cameras, etc.)<br>• Set actor transforms (position, rotation, scale)<br>• Query actor properties and find actors by name or pattern<br>• List all actors in the current level<br>• Set and query light properties (intensity, color, attenuation, shadows, source size)<br>• Spawn actors from Blueprint classes with custom transforms |
| **Blueprint Development** | • Create new Blueprint classes with custom parent classes (Actor, Pawn, etc.)<br>• Add and configure components (StaticMesh, Camera, Light, etc.)<br>• Set component properties and static mesh assets<br>• Configure physics properties (simulation, gravity, mass, damping)<br>• Set Pawn-specific properties (auto possess, rotation control, damageability)<br>• Compile Blueprints<br>• Set Blueprint class default properties<br>• Add variables of any type (Boolean, Integer, Float, Vector, Struct, Array, Delegate, Blueprint references)<br>• Add interfaces to Blueprints<br>• Create Blueprint Interfaces<br>• Add custom event nodes to Blueprints<br>• Call BlueprintCallable functions by name<br>• **List all components (including inherited) in a Blueprint class for inspection and automation** |
| **Blueprint Node Graph** | • Add event nodes for standard events (BeginPlay, Tick) and input actions<br>• Add custom event nodes<br>• Create function call nodes with target components and parameters<br>• Connect nodes with proper pin linkages for execution and data flow<br>• Add variables with various types (Boolean, Integer, Float, Vector, Struct, etc.)<br>• Create component references and self references in the graph<br>• Find and identify nodes in the Blueprint graph by type/event<br>• Get variable type information for automation<br>• Build complete gameplay logic chains through the Blueprint visual scripting system |
| **UMG/UI Development** | • Create UMG Widget Blueprints for building user interfaces<br>• Add and customize UI components (text, buttons, images, checkboxes, sliders, etc.)<br>• Add any widget component type to a widget<br>• Create complex layouts with scrollboxes, borders, containers, and nested hierarchies<br>• Set up event bindings and property bindings for dynamic UI<br>• Add widgets to the viewport with z-ordering control<br>• Set and query widget component properties (text, color, brush, etc.)<br>• Change widget placement, size, and alignment<br>• Check for component existence<br>• Get hierarchical layout information for all components in a widget<br>• Get container dimensions for layout automation |
| **DataTable Management** | • Create new DataTables with custom row structs<br>• Add, update, and delete rows (single or multiple) in DataTables<br>• Query all rows or specific rows<br>• Get all row names and struct field names<br>• Automate row property mapping using GUID-based field names |
| **Project Management** | • Create and organize content browser folders for asset management<br>• Create project folders for non-content files (logs, intermediate, etc.)<br>• Set up input mappings for keyboard, mouse, and gamepad controls<br>• **Enhanced Input System**: Create Input Action assets with value types (Digital, Analog, Axis2D, Axis3D)<br>• **Enhanced Input System**: Create Input Mapping Context assets for organized input handling<br>• **Enhanced Input System**: Add key mappings between contexts and actions with modifier support (Shift, Ctrl, Alt, Cmd)<br>• **Enhanced Input System**: List and query Enhanced Input Actions and Mapping Contexts with metadata<br>• **Enhanced Input System**: Full integration with UE 5.5+ Enhanced Input architecture<br>• Create, update, and inspect Unreal structs<br>• List folder contents for project and content folders |
| **Editor Control** | • Focus viewport on specific actors or locations with custom distance<br>• Control viewport camera orientation with precise angle settings<br>• Find actors in the scene using name pattern matching and wildcards<br>• Access and modify actor properties through the editor interface<br>• Create and configure different light types (Point, Spot, Directional)<br>• Adjust light properties (intensity, color, attenuation, shadows, source size)<br>• Spawn Blueprint actors with custom logic and components |

All these capabilities are accessible through natural language commands via AI assistants, making it easy to automate and control Unreal Engine workflows.

## 🎮 Enhanced Input System Integration

The Unreal MCP integration includes comprehensive support for Unreal Engine 5.5+'s Enhanced Input system, providing modern input handling capabilities through natural language commands.

### 🛠️ Enhanced Input Features

| Feature | Description | Example Usage |
|---------|-------------|---------------|
| **Input Action Creation** | Create Enhanced Input Action assets with configurable value types | "Create a digital input action called 'Jump' for jumping mechanics" |
| **Input Mapping Context** | Create Input Mapping Context assets for organized input handling | "Create an input mapping context called 'PlayerContext' for player controls" |
| **Key Mapping Management** | Add key bindings between contexts and actions with modifier support | "Map the Space key to the Jump action in the PlayerContext" |
| **Asset Discovery** | List and query all Enhanced Input assets with metadata | "Show me all Enhanced Input Actions in the project" |
| **Blueprint Integration** | Add Enhanced Input Action event nodes to Blueprint graphs | "Add an Enhanced Input Action node for the Jump action to my player Blueprint" |

### 🔧 Available Input Value Types

- **Digital**: Simple on/off input (buttons, keys)
- **Analog**: Single-axis input with continuous values (triggers, mouse wheel)
- **Axis2D**: Two-dimensional input (mouse movement, gamepad sticks)
- **Axis3D**: Three-dimensional input (3D controllers, VR inputs)

### 🎯 Key Features

- **Automatic Asset Naming**: Follows UE conventions with `IA_` and `IMC_` prefixes
- **Modifier Key Support**: Full support for Shift, Ctrl, Alt, and Cmd modifiers
- **Asset Persistence**: Proper UE 5.5 asset creation workflow ensuring assets are saved correctly
- **Metadata Extraction**: Complete asset information including mappings, value types, and descriptions
- **Blueprint Node Support**: Create Enhanced Input Action event nodes for Blueprint scripting
- **Path Management**: Automatic directory creation and organized asset placement

### 🔍 Example Natural Language Commands

```
"Create a digital Enhanced Input Action called 'Interact' for player interactions"

"Create an input mapping context called 'MainGameplay' for core game controls"

"Map the E key to the Interact action in the MainGameplay context"

"Add the F key with Shift modifier to the Jump action"

"Show me all Enhanced Input Actions in the /Game/Input directory"

"List all Input Mapping Contexts and their current key mappings"

"Add an Enhanced Input Action event node for the Jump action to my PlayerBlueprint"
```

### ⚙️ Technical Implementation

The Enhanced Input integration includes:

#### **C++ Plugin Components**:
- `HandleCreateEnhancedInputAction`: Creates Input Action assets with proper UE 5.5 workflow
- `HandleCreateInputMappingContext`: Creates Input Mapping Context assets with package management
- `HandleAddMappingToContext`: Manages key mappings using FEnhancedActionKeyMapping system
- `HandleListInputActions`: Asset discovery with metadata extraction and filtering
- `HandleListInputMappingContexts`: Context enumeration with complete mapping details

#### **Python MCP Tools**:
- `create_enhanced_input_action`: Python wrapper for Input Action creation
- `create_input_mapping_context`: Python wrapper for Mapping Context creation
- `add_mapping_to_context`: Key mapping management with modifier support
- `list_input_actions`: Asset listing with path filtering capabilities
- `list_input_mapping_contexts`: Context discovery with mapping enumeration

#### **Asset Management**:
- Proper package creation and asset registry integration
- Asset persistence using target packages instead of transient packages
- Automatic asset naming with UE conventions (`IA_`, `IMC_` prefixes)
- Directory creation and path management for organized asset structure

### 🔄 Backward Compatibility

The Enhanced Input tools work alongside existing legacy input mapping functionality, allowing projects to:
- Migrate gradually from legacy input to Enhanced Input
- Use both systems simultaneously during transition
- Maintain existing legacy input mappings while adding Enhanced Input features

This integration provides a complete Enhanced Input toolset that modernizes input handling capabilities while preserving compatibility with existing input systems.

## 🧩 Components

### Sample Project (MCPGameProject) `MCPGameProject`
- Based off the Blank Project, but with the UnrealMCP plugin added.

### Plugin (UnrealMCP) `MCPGameProject/Plugins/UnrealMCP`
- Native TCP server for MCP communication
- Integrates with Unreal Editor subsystems
- Implements actor manipulation tools
- Handles command execution and response handling

### Python MCP Server `Python/unreal_mcp_server.py`
- Implemented in `unreal_mcp_server.py`
- Manages TCP socket connections to the C++ plugin (port 55557)
- Handles command serialization and response parsing
- Provides error handling and connection management
- Loads and registers tool modules from the `tools` directory
- Uses the FastMCP library to implement the Model Context Protocol

## 📂 Directory Structure

- **MCPGameProject/** - Example Unreal project
  - **Plugins/UnrealMCP/** - C++ plugin source
    - **Source/UnrealMCP/** - Plugin source code
    - **UnrealMCP.uplugin** - Plugin definition

- **Python/** - Python server and tools
  - **tools/** - Tool modules for actor, editor, blueprint, and UMG operations
  - **scripts/** - Example scripts and demos

- **Docs/** - Comprehensive documentation
  - See [Docs/README.md](Docs/README.md) for documentation index

## 🚀 Quick Start Guide

### Prerequisites
- Unreal Engine 5.5+
- Python 3.12+
- MCP Client (e.g., Claude Desktop, Cursor, Windsurf)

### Sample project

For getting started quickly, feel free to use the starter project in `MCPGameProject`. This is a UE 5.5 Blank Starter Project with the `UnrealMCP.uplugin` already configured. 

1. **Prepare the project**
   - Right-click your .uproject file
   - Generate Visual Studio project files
2. **Build the project (including the plugin)**
   - Open solution (`.sln`)
   - Choose `Development Editor` as your target.
   - Build

### Plugin
Otherwise, if you want to use the plugin in your existing project:

1. **Copy the plugin to your project**
   - Copy `MCPGameProject/Plugins/UnrealMCP` to your project's Plugins folder

2. **Enable the plugin**
   - Edit > Plugins
   - Find "UnrealMCP" in Editor category
   - Enable the plugin
   - Restart editor when prompted

3. **Build the plugin**
   - Right-click your .uproject file
   - Generate Visual Studio project files
   - Open solution (`.sln)
   - Build with your target platform and output settings

### Python Server Setup

See [Python/README.md](Python/README.md) for detailed Python setup instructions, including:
- Setting up your Python environment
- Running the MCP server
- Using direct or server-based connections

### Configuring your MCP Client

Use the following JSON for your mcp configuration based on your MCP client.

```json
{
  "mcpServers": {
    "blueprintMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "E:\\code\\unreal-mcp\\Python",
        "run",
        "blueprint_mcp_server.py"
      ]
    },
    "editorMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "E:\\code\\unreal-mcp\\Python",
        "run",
        "editor_mcp_server.py"
      ]
    },
    "umgMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "E:\\code\\unreal-mcp\\Python",
        "run",
        "umg_mcp_server.py"
      ]
    },
    "nodeMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "E:\\code\\unreal-mcp\\Python",
        "run",
        "node_mcp_server.py"
      ]
    },
    "datatableMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "E:\\code\\unreal-mcp\\Python",
        "run",
        "datatable_mcp_server.py"
      ]
    },
    "projectMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "E:\\code\\unreal-mcp\\Python",
        "run",
        "project_mcp_server.py"
      ]
    }
  }
}
```

An example is found in `mcp.json`

### MCP Configuration Locations

Depending on which MCP client you're using, the configuration file location will differ:

| MCP Client | Configuration File Location | Notes |
|------------|------------------------------|-------|
| Claude Desktop | `~/.config/claude-desktop/mcp.json` | On Windows: `%USERPROFILE%\.config\claude-desktop\mcp.json` |
| Cursor | `.cursor/mcp.json` | Located in your project root directory |
| Windsurf | `~/.config/windsurf/mcp.json` | On Windows: `%USERPROFILE%\.config\windsurf\mcp.json` |

Each client uses the same JSON format as shown in the example above. 
Simply place the configuration in the appropriate location for your MCP client.


## License
MIT

## Questions

For questions, you can reach me on X/Twitter: [@chongdashu](https://www.x.com/chongdashu)