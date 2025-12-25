# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Launch Commands

### Building the C++ Plugin
```powershell
# IMPORTANT: Always use this exact command format (not ./RebuildProject.bat or cmd /c)
powershell -Command "& {cd e:\code\unreal-mcp; .\RebuildProject.bat}"

# This script:
# 1. Terminates all UnrealEditor*.exe processes
# 2. Cleans Intermediate and Binaries folders
# 3. Calls UE 5.7's Build.bat for MCPGameProjectEditor Win64 Development
# 4. Targets: MCPGameProject\MCPGameProject.uproject
```

### Launching Unreal Editor
```powershell
# IMPORTANT: Always use this exact command format (not ./LaunchProject.bat or cmd /c)
powershell -Command "& {cd e:\code\unreal-mcp; .\LaunchProject.bat}"

# Uses: C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe
# Opens: E:\code\unreal-mcp\MCPGameProject\MCPGameProject.uproject
```

### Python MCP Server Setup
```bash
cd Python
uv venv
.venv\Scripts\activate  # Windows
uv pip install -e .

# Servers are being run by user manually via VSCode launch configurations.
```

## Architecture Overview

### Dual-Component System

This project enables **natural language control of Unreal Engine 5.7** through AI assistants via a synchronized dual-component architecture:

```
AI Assistant (Claude/Cursor/Windsurf)
    ↓ [MCP Protocol]
Python MCP Servers (7 FastMCP servers)
    ↓ [TCP/JSON on 127.0.0.1:55557]
C++ Plugin (UnrealMCP EditorSubsystem)
    ↓ [Direct API calls]
Unreal Engine 5.7 Editor
```

### Critical Synchronization Requirement

**Both Python and C++ sides MUST remain synchronized when adding functionality:**
- Function signatures, parameter names, types, and return values must match exactly
- TCP command strings in Python must correspond to registered dispatcher commands in C++
- JSON payloads between Python and C++ must maintain identical schemas

### Component Locations

**Python MCP Servers**: `Python/`
- 7 independent FastMCP servers (blueprint, editor, umg, node, datatable, project, blueprint_action)
- **MCP tools are defined in `*_mcp_server.py` files** (NOT in `*_tools/` folders)
- Shared utilities in `utils/` (blueprints/, nodes/, umg/, datatable/, editor/, project/)
- TCP communication via `utils/unreal_connection_utils.py` (connects to 127.0.0.1:55557)

**C++ Plugin**: `MCPGameProject/Plugins/UnrealMCP/`
- Main dispatcher: `Commands/UnrealMCPMainDispatcher.h/.cpp`
- Command categories in `Commands/*/` (Blueprint, BlueprintAction, BlueprintNode, GraphManipulation, Editor, UMG, DataTable, Project)
- Service layer in `Services/*/` (business logic separated from commands)
- Factories in `Factories/` (ComponentFactory, WidgetFactory for type-safe object creation)

### Module Initialization Flow

**C++ Plugin Startup** (`UnrealMCPModule::StartupModule`):
1. Initialize `FMCPLogger` (file logging enabled)
2. Initialize `FObjectPoolManager` (performance optimization)
3. Initialize `FComponentFactory::InitializeDefaultTypes()` (register component types)
4. Initialize `FWidgetFactory::InitializeDefaultWidgetTypes()` (register widget types)
5. Initialize `FUnrealMCPMainDispatcher::Initialize()` (register all commands)

**EditorSubsystem Startup** (`UUnrealMCPBridge::Initialize`):
1. Create TCP listener socket on 127.0.0.1:55557
2. Start `MCPServerRunnable` thread (async command loop)
3. Accept connections, read JSON commands (48KB buffer), execute via dispatcher, send responses

### Command Execution Pattern

**Adding a new tool requires changes in both components:**

**Python Side** (example: `Python/blueprint_mcp_server.py`):
```python
@app.tool()
async def create_blueprint(name: str, parent_class: str, folder_path: str = "") -> Dict[str, Any]:
    """Create a new Blueprint class."""
    params = {"name": name, "parent_class": parent_class}
    if folder_path:
        params["folder_path"] = folder_path
    return await send_tcp_command("create_blueprint", params)
```

**C++ Side** (example: `Commands/Blueprint/CreateBlueprintCommand.cpp`):
```cpp
class FCreateBlueprintCommand : public IUnrealMCPCommand
{
    FString CommandName() const override { return TEXT("create_blueprint"); }
    TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params) override;
};
// Register in FBlueprintCommandRegistration::RegisterAllBlueprintCommands()
```

**Command Registry System**:
- Commands implement `IUnrealMCPCommand` interface
- Registered via category-specific registration functions (e.g., `FBlueprintCommandRegistration::RegisterAllBlueprintCommands`)
- Dispatcher routes commands via `FUnrealMCPMainDispatcher::HandleCommand(CommandType, Params)`
- Returns JSON responses (48KB buffer for detailed errors/large payloads)

## Development Workflow

### MCP Implementation Plans: Mandatory Debugging Checkpoints (CRITICAL)

**⚠️ BEFORE creating ANY plan for UE functionality via MCP tools, this section MUST be followed.**

When implementing complex functionality (inventory systems, dialogue systems, game mechanics, etc.) through MCP tools:

1. **Every step in every phase MUST have a verification checkpoint**
   - Even "simple" data creation (structs, enums, DataTables) must be verified
   - Do NOT batch multiple steps without intermediate verification
   - Do NOT proceed to next step until current step is verified working

2. **Verification checkpoint requirements:**
   - After creating any asset (struct, enum, DataTable): Confirm it exists with correct properties via `get_*_metadata` tools
   - After completing a function's node logic: Compile Blueprint and check for errors
   - After completing a phase: User verification checkpoint before proceeding
   - Note: Do NOT compile after every single node - compile once when full function is complete

3. **Debug-first plan structure:**
   ```
   Step X.Y: [Action]
   - MCP Tool Call: [specific tool and params]
   - Verification: [how to confirm it worked]
   - Expected Result: [what success looks like]
   - Debug Point: [what to check if it fails]
   ```

4. **When errors occur:**
   - STOP immediately - do not continue building on broken foundation
   - Identify the exact step that failed
   - Fix that step and re-verify before continuing
   - Document any MCP tool limitations in `Docs/known-issues.md`

5. **Phase completion gates:**
   - ALL steps in current phase must be verified before starting next phase
   - No "we'll fix it later" - fix it now or document as blocked

**Rationale:** MCP-created Blueprint logic is difficult to debug after the fact. Each step builds on previous steps. One incorrect node connection early on cascades into complex debugging later. Verify early, verify often.

### Adding New Functionality

1. **Plan cross-component**: Design Python API signature and C++ implementation together
2. **Implement C++ command handler**:
   - Create command class in appropriate `Commands/*/` folder
   - Implement `IUnrealMCPCommand` interface (`CommandName()` and `Execute()`)
   - Register in category's registration function
   - Use service layer for business logic
3. **Create Python MCP tool**:
   - Add `@app.tool()` decorated function in the correct `*_mcp_server.py` file (see table below)
   - Use `await send_tcp_command(command_type, params)` to call C++ side
4. **Compile and test**:
   - Run `RebuildProject.bat` to compile C++ changes
   - Run `LaunchProject.bat` to start Unreal Editor
   - **Restart Python MCP servers** for Python changes to take effect
   - Test via AI assistant natural language or Python test scripts

### Service Layer Architecture

The C++ plugin uses a **modular service layer pattern** for clean separation of concerns:

**Service Organization**:
- **Services/Blueprint/** - Blueprint creation, properties, functions, caching
  - `BlueprintCreationService` - Blueprint class creation
  - `BlueprintPropertyService` - Property management
  - `BlueprintFunctionService` - Function operations
  - `BlueprintCacheService` - Asset caching

- **Services/BlueprintAction/** - Action database querying and discovery
  - `BlueprintActionDiscoveryService` - Action database searches
  - `BlueprintActionSearchService` - Action filtering
  - `BlueprintClassSearchService` - Class hierarchy exploration
  - `BlueprintPinSearchService` - Pin type searches
  - `BlueprintNodePinInfoService` - Pin metadata

- **Services/NodeCreation/** - Node creation strategies
  - `BlueprintActionDatabaseNodeCreator` - Database-driven creation
  - `ArithmeticNodeCreator` - Math operation nodes
  - `NativePropertyNodeCreator` - Property getter/setter nodes
  - `NodeCreationHelpers` - Shared utilities
  - `NodeResultBuilder` - Response formatting

- **Services/UMG/** - Widget operations (split into specialized files)

- **Commands/** - Thin handlers that validate params and call services

- **Factories/** - Type-safe object creation (ComponentFactory, WidgetFactory)

### File Size Guidelines (CRITICAL)

**⚠️ NEVER CREATE FILES LARGER THAN 1000 LINES**
- Target: 800 lines per file
- Absolute limit: 1000 lines
- See `.github/large-cpp-files.md` for existing problematic files
- When refactoring: Split into multiple smaller, focused files
- Recent refactoring examples:
  - `BlueprintService` → `BlueprintCreationService`, `BlueprintPropertyService`, `BlueprintFunctionService`, `BlueprintCacheService`
  - `BlueprintNodeCreationService` → `ControlFlowNodeCreator`, `EventAndVariableNodeCreator`
  - `UnrealMCPCommonUtils` → 6 specialized utility classes

**⚠️ ALWAYS CHECK FILE SIZE AFTER ADDING CODE**
- After adding new functions, check the file's line count
- If file exceeds 800 lines, proactively suggest splitting to user
- If file exceeds 1000 lines, MUST split before committing
- Split methodology: Extract functions as-is into new focused files (no refactoring during split)

## Unreal Engine Specifics

### Version & APIs
- **Using Unreal Engine 5.7** - Be cautious of deprecated/outdated APIs
- Reference full UE 5.7 source in `ues/UnrealEngine-5.7/` for API documentation
- Due to large folder size - use OS file search tools to locate specific classes/functions when needed

### Coordinate System
- **Z-up, left-handed coordinate system**
- X-axis (Red): Forward/backward (positive = forward)
- Y-axis (Green): Left/right (positive = right)
- Z-axis (Blue): Up/down (positive = up)

### Units
- Distance: Centimeters (cm)
- Mass: Kilograms (kg)
- Time: Seconds (s)
- Angles: Degrees (deg)
- Speed: Meters per second (m/s)
- Force: Newtons (N)

### TCP Configuration
```cpp
// C++: UnrealMCPBridge.cpp
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557
```

```python
# Python: utils/unreal_connection_utils.py
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557
```

## Python MCP Tool Guidelines

### Where to Modify MCP Tools (CRITICAL)

**⚠️ MCP tools are defined ONLY in `Python/*_mcp_server.py` files:**

| MCP Server File | Purpose | C++ Command Category |
|-----------------|---------|---------------------|
| `blueprint_mcp_server.py` | Blueprint creation, compilation, variables, components, interfaces, metadata | `Commands/Blueprint/` |
| `editor_mcp_server.py` | Actor spawning, transforms, viewport, light properties | `Commands/Editor/` |
| `umg_mcp_server.py` | Widget Blueprints, UI components, layouts, bindings | `Commands/UMG/` |
| `node_mcp_server.py` | Node connections, pin values, graph manipulation | `Commands/BlueprintNode/`, `Commands/GraphManipulation/` |
| `datatable_mcp_server.py` | DataTable creation, CRUD operations | `Commands/DataTable/` |
| `project_mcp_server.py` | Folders, input mappings, structs, enums | `Commands/Project/` |
| `blueprint_action_mcp_server.py` | Action discovery, node creation, pin inspection | `Commands/BlueprintAction/` |

**These `*_mcp_server.py` files contain the `@app.tool()` decorated functions that Claude/AI assistants actually call.**

**⚠️ DO NOT modify these folders for MCP tool changes:**
- `Python/blueprint_tools/` - Legacy/unused implementations
- `Python/utils/` - Shared utility functions (called BY MCP servers, not MCP tools themselves)
- `Python/*_tools/` - Helper modules (may be imported by MCP servers but are NOT the tool entry points)

**When adding/modifying MCP tool parameters:**
1. Identify the correct `*_mcp_server.py` file from the table above
2. Add/modify the `@app.tool()` decorated async function
3. Add parameter to the `params` dict passed to `send_tcp_command()`
4. Update the docstring with new parameter documentation
5. **Restart the Python MCP server** for changes to take effect (user runs via VSCode)

### Parameter Type Guidelines

**For functions decorated with `@app.tool()`:**
- Parameters should NOT use types: `Any`, `object`, `Optional[T]`, `Union[T]`
- For parameters with default values, use `x: T = None` NOT `x: T | None = None`
- Handle defaults within method body
- Always include comprehensive docstrings with valid input examples

## Key Extension Categories

- **Blueprint**: Class creation, compilation, components, variables, interfaces
- **BlueprintAction**: Dynamic node discovery, pin analysis, class hierarchy exploration
- **BlueprintNode**: Event nodes, function calls, pin connections, graph manipulation
- **GraphManipulation**: Replace nodes, disconnect pins, delete nodes (preserves connections)
- **Editor**: Actor spawning/management, transforms, viewport control, light properties
- **UMG**: Widget Blueprint creation, UI components, layouts, event/property bindings
- **DataTable**: Create tables, CRUD operations, struct-based data management
- **Project**: Folders, input mappings, Enhanced Input System (UE 5.5+), structs

## Blueprint Architecture Best Practices

**⚠️ All game logic should be implemented in custom functions, NOT in EventGraph:**
- EventGraph should only contain event bindings (BeginPlay, Tick, Input Actions) that call custom functions
- Complex logic belongs in dedicated custom functions for better organization and debugging
- This makes orphan detection more reliable (EventGraph has special entry points like Enhanced Input Actions)
- Easier to test, refactor, and maintain individual functions

**When working with orphan detection/cleanup:**
- Always scope operations to specific graphs (e.g., `target_graph="MyFunction"`)
- EventGraph should generally be excluded from automatic orphan deletion due to its special nature
- Development workflow typically focuses on one function at a time - tools should support this

**Return Node handling:**
- Every function has an auto-generated Return Node at (0,0) - this may appear orphaned if unused
- For non-void functions, always connect the final exec flow to the Return Node
- Pure functions don't have exec pins - they use data flow only

## Important Implementation Files

**C++ Entry Points:**
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPModule.cpp` - Module startup/shutdown
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPBridge.cpp` - TCP server subsystem
- `Commands/UnrealMCPMainDispatcher.h/.cpp` - Central command router
- `Commands/UnrealMCPCommandRegistry.cpp` - Command registration system

**Python Entry Points:**
- `Python/*_mcp_server.py` - 7 independent MCP servers
- `Python/utils/unreal_connection_utils.py` - TCP connection management

**Configuration:**
- MCP client configs (Claude Desktop: `%USERPROFILE%\.config\claude-desktop\mcp.json`)
- Python dependencies: `Python/pyproject.toml`
- Plugin manifest: `MCPGameProject/Plugins/UnrealMCP/UnrealMCP.uplugin`

## Recent Architecture Improvements

The codebase has undergone significant refactoring to improve modularity and maintainability:

1. **Service Layer Modularization**: Large service files split into focused, specialized classes
2. **Node Creation Strategy Pattern**: Separate creator classes for different node types
3. **Blueprint Action Services**: Dedicated services for action discovery, searching, and pin analysis
4. **Utility Class Decomposition**: Common utilities split into domain-specific modules
5. **Command Registration Pattern**: Centralized registry with category-specific registration functions

When adding new features, follow these established patterns for consistency and maintainability.

## Known Issues and Future Improvements

**Check `Docs/known-issues.md` for:**
- MCP tool limitations discovered during development
- Feature requests for future implementation
- Workarounds for known problems

**When encountering MCP limitations:**
1. Document the issue in `Docs/known-issues.md`
2. Include: what failed, expected behavior, any workarounds
3. Tag with affected tool names for searchability
