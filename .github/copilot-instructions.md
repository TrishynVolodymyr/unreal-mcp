---
description: Unreal engine MCP project for AI usage, architecture and development guidelines
applyTo: **
---

# Unreal MCP Project Architecture

## 🎯 Core Dual-Component System

The Unreal MCP project enables **natural language control of Unreal Engine 5.7** through a synchronized dual-component architecture:

### 📚 Unreal Engine Source Code Reference (`ues/UnrealEngine-5.7/`)
- **Complete UE 5.7 Source**: Available as documentation and reference for plugin development
- **Engine API Reference**: Use for understanding internal systems, subsystems, and implementation patterns
- **Class Hierarchies**: Reference for proper inheritance and interface implementation
- **Best Practices**: Study engine code for C++ conventions, memory management, and performance patterns
- **Debugging Aid**: Helpful for understanding engine behavior and troubleshooting integration issues
- **⚠️ ATTENTION**: Due to big folder size - use OS file search tools to locate specific classes or functions when needed

### 🐍 Python MCP Server Component (`Python/`)
- **FastMCP-based servers**: Individual MCP servers per domain (blueprint_mcp_server.py, datatable_mcp_server.py, etc.)
- **TCP Communication**: Uses TCP sockets (127.0.0.1:55557) via `send_tcp_command()` for Unreal Engine communication
- **Tool Modules**: Organized in domain-specific folders (`blueprint_tools/`, `umg_tools/`, `editor_tools/`, etc.)
- **Environment**: Uses `uv` for dependency management with Python 3.10+ requirement

### ⚙️ Unreal Engine Plugin (`MCPGameProject/Plugins/UnrealMCP/`)
- **C++ Implementation**: Editor plugin with modular dispatcher system (`FUnrealMCPMainDispatcher`)
- **Component Factories**: `FComponentFactory` and `FWidgetFactory` for dynamic object creation
- **Subsystem Integration**: Direct interface with Unreal Editor subsystems for Blueprint, UMG, DataTable operations
- **TCP Server**: Listens for commands from Python MCP servers

## 🔄 Critical Synchronization Requirements

**ESSENTIAL**: Both components must remain synchronized when extending functionality:

1. **Function Signatures**: Parameter names, types, and return values must match exactly between Python tools and C++ handlers
2. **Command Types**: TCP command strings in Python must correspond to registered dispatcher commands in C++
3. **Data Structures**: JSON payloads between Python and C++ must maintain identical schemas

## 🚨 CRITICAL AI ASSISTANT RULES

### File Size Management
**⚠️ NEVER CREATE FILES LARGER THAN 1000 LINES!**

- **Maximum file size**: 800 lines (target), 1000 lines (absolute limit)
- **Why**: AI performance degrades significantly with large files
- **Large files list**: See `.github/large-cpp-files.md` for existing problematic files
- **When refactoring**: Split into multiple smaller, focused files

### Efficient File Operations
**Reading Large Files:**
- Use `grep_search` to find specific functions instead of reading entire files
- Use `read_file` with specific line ranges, not entire files
- Read only what you need for the current task

**Editing Files:**
- Use `multi_replace_string_in_file` for multiple edits in one call
- Never make sequential edits when parallel edits are possible
- Include 3-5 lines of context before/after for unambiguous matching

**Creating New Files:**
- Keep new files under 800 lines
- Split functionality into logical, focused modules
- Create separate service classes for new domains

## 🛠️ Development Workflow Patterns

### Adding New Tool Functionality
1. **Plan cross-component**: Design Python API and C++ implementation together
2. **Implement C++ side first**: Add command handler to dispatcher in `MCPGameProject/Plugins/UnrealMCP/`
3. **Create Python MCP tool**: Add corresponding function in appropriate `Python/*_mcp_server.py`
4. **Register TCP command**: Ensure command type mapping exists between both sides
5. **Test via AI assistant**: Validate natural language interaction works end-to-end

### Build & Launch Workflow
```powershell
# Compile project (kills any running Unreal Editor instances)
.\RebuildProject.bat

# Launch Unreal Editor (checks if already running)
.\LaunchProject.bat
```

**Build Details**: 
- `RebuildProject.bat` uses UE 5.7's Build.bat for MCPGameProjectEditor Win64 Development
- Automatically terminates existing UnrealEditor.exe processes before compilation
- `LaunchProject.bat` prevents duplicate editor instances

### MCP Server Architecture
Each domain has its own MCP server following this pattern:
```python
# Example: blueprint_mcp_server.py
TCP_HOST = "127.0.0.1"
TCP_PORT = 55557

async def send_tcp_command(command_type: str, params: Dict[str, Any]):
    # Standard TCP communication pattern used across all servers
```

## 📦 Extension Categories
- **Blueprint Tools**: Class creation, compilation, component management, variable handling
- **Blueprint Action Tools**: Dynamic node discovery, pin analysis, class hierarchy exploration
- **Editor Tools**: Actor manipulation, viewport control, scene management
- **UMG Tools**: Widget creation, UI layout, event binding
- **DataTable Tools**: Struct-based data management, row operations
- **Project Tools**: Content organization, input systems, Enhanced Input (UE 5.5+)
- **Node Tools**: Blueprint graph construction, event chains, visual scripting logic

## 📍 Key Implementation Files
- **Python Entry Points**: `Python/*_mcp_server.py` files (blueprint, datatable, editor, etc.)
- **C++ Module**: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPModule.cpp`
- **Command Dispatcher**: `Commands/UnrealMCPMainDispatcher.h/.cpp`
- **Factories**: `Factories/ComponentFactory.h` and `Factories/WidgetFactory.h`
- **Large Files List**: `.github/large-cpp-files.md` - files requiring special attention

## ✅ Best Practices Summary

1. **Keep files small** - under 800 lines
2. **Use efficient tools** - grep_search, multi_replace, targeted reads
3. **Maintain sync** - Python ↔️ C++ command matching
4. **Test incrementally** - validate after each major change
5. **Modular design** - separate concerns into focused files

This architecture enables powerful AI-driven Unreal Engine workflows while maintaining clean separation of concerns and optimal AI performance.