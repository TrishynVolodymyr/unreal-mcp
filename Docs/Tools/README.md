# Unreal MCP Tools - Technical API Reference

Technical documentation for all MCP tool categories. This section provides command parameters, JSON schemas, and protocol details for developers.

**For natural language usage examples**, see the [user-friendly guides](../README.md).

---

## 📚 Tool Categories

| Category | File | Description |
|----------|------|-------------|
| **[Actor Tools](actor_tools.md)** | `actor_tools.md` | Actor spawning, deletion, and manipulation in the scene |
| **[Blueprint Tools](blueprint_tools.md)** | `blueprint_tools.md` | Blueprint class creation, components, variables, compilation |
| **[Blueprint Action Tools](blueprint_action_tools.md)** | `blueprint_action_tools.md` | Dynamic Blueprint action discovery via UE's action database |
| **[Editor Tools](editor_tools.md)** | `editor_tools.md` | Editor control, viewport management, scene queries |
| **[Node Tools](node_tools.md)** | `node_tools.md` | Blueprint visual scripting, node creation, connections |
| **[UMG Tools](umg_tools.md)** | `umg_tools.md` | Widget Blueprint creation, UI components, layouts |
| **[DataTable Tools](datatable_tools.md)** | `datatable_tools.md` | DataTable CRUD operations, struct management |
| **[Project Tools](project_tools.md)** | `project_tools.md` | Project organization, Enhanced Input System, structs |

---

## 🏗️ Architecture Overview

### Communication Protocol

```
Python MCP Tool (@mcp.tool())
    ↓
send_tcp_command(command_type, params)
    ↓
TCP Socket (localhost:55557)
    ↓
C++ UnrealMCP Plugin
    ↓
Command Dispatcher
    ↓
Service Layer (Business Logic)
    ↓
Unreal Engine API
```

### JSON Command Format

All commands follow this JSON structure:

```json
{
  "command": "command_name",
  "params": {
    "param1": "value1",
    "param2": "value2"
  }
}
```

### JSON Response Format

All responses follow this JSON structure:

```json
{
  "success": true,
  "message": "Operation completed successfully",
  "data": {
    "key": "value"
  }
}
```

**Error Response:**
```json
{
  "success": false,
  "message": "Error description here",
  "error": "Detailed error information"
}
```

---

## 🔧 Common Patterns

### Parameter Types

| Type | Python Type | C++ Type | Example |
|------|-------------|----------|---------|
| **String** | `str` | `FString` | `"BP_MyActor"` |
| **Integer** | `int` | `int32` | `42` |
| **Float** | `float` | `float` | `3.14` |
| **Boolean** | `bool` | `bool` | `true` |
| **Array** | `List` | `TArray` | `[1, 2, 3]` |
| **Vector** | `List[float]` | `FVector` | `[0.0, 0.0, 100.0]` |
| **Rotator** | `List[float]` | `FRotator` | `[0.0, 90.0, 0.0]` |
| **Object** | `Dict` | `TSharedPtr<FJsonObject>` | `{"key": "value"}` |

### Vector Representation

Unreal uses **Z-up, left-handed coordinate system**:

```json
{
  "position": [X, Y, Z],  // [Forward/Back, Right/Left, Up/Down]
  "rotation": [Pitch, Yaw, Roll],  // Degrees
  "scale": [X, Y, Z]
}
```

**Example:**
```json
{
  "position": [100.0, 0.0, 50.0],  // 100cm forward, 50cm up
  "rotation": [0.0, 45.0, 0.0],    // Rotated 45° on Z axis
  "scale": [1.0, 1.0, 1.0]         // Default scale
}
```

### Asset Path Format

Unreal asset paths follow this format:

```
/Game/FolderName/AssetName
```

**Examples:**
- Blueprint: `/Game/Blueprints/BP_Character`
- Widget: `/Game/UI/WBP_MainMenu`
- DataTable: `/Game/Data/DT_Items`
- Struct: `/Game/Structs/S_ItemData`

---

## 🔌 TCP Protocol Details

### Connection

- **Host**: `127.0.0.1` (localhost)
- **Port**: `55557`
- **Protocol**: TCP
- **Encoding**: UTF-8 JSON
- **Buffer Size**: 48KB (49152 bytes)

### Python Connection Example

```python
import socket
import json

def send_tcp_command(command_type: str, params: dict):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("127.0.0.1", 55557))

        # Send command
        command = {"command": command_type, "params": params}
        sock.sendall(json.dumps(command).encode('utf-8'))

        # Receive response
        response = sock.recv(49152).decode('utf-8')
        return json.loads(response)
```

### C++ Command Execution

Commands are registered in the dispatcher and routed to service classes:

```cpp
// Command Registration
Dispatcher->RegisterCommand(TEXT("create_blueprint"),
    MakeShared<FCreateBlueprintCommand>());

// Command Execution
TSharedPtr<FJsonObject> Result = Command->Execute(Params);
```

---

## 📖 JSON Schema Examples

### Create Blueprint

**Request:**
```json
{
  "command": "create_blueprint",
  "params": {
    "name": "BP_MyActor",
    "parent_class": "Actor",
    "folder_path": "/Game/Blueprints"
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Blueprint created successfully",
  "data": {
    "blueprint_path": "/Game/Blueprints/BP_MyActor",
    "parent_class": "Actor"
  }
}
```

### Spawn Actor

**Request:**
```json
{
  "command": "spawn_actor",
  "params": {
    "actor_class": "PointLight",
    "name": "MainLight",
    "position": [0.0, 0.0, 200.0],
    "rotation": [0.0, 0.0, 0.0],
    "scale": [1.0, 1.0, 1.0]
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Actor spawned successfully",
  "data": {
    "actor_name": "MainLight",
    "actor_class": "PointLight",
    "position": [0.0, 0.0, 200.0]
  }
}
```

### Create Node

**Request:**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_path": "/Game/BP_Character",
    "graph_name": "EventGraph",
    "action_name": "PrintString",
    "node_pos_x": 200,
    "node_pos_y": 100
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Node created successfully",
  "data": {
    "node_guid": "A1B2C3D4E5F6...",
    "node_title": "Print String",
    "pins": [
      {"name": "exec", "direction": "input"},
      {"name": "then", "direction": "output"},
      {"name": "InString", "direction": "input"}
    ]
  }
}
```

---

## 🛠️ Development Guidelines

### Adding New Commands

1. **Define Python Tool** (`*_tools/*.py`):
```python
@mcp.tool()
def my_new_command(ctx: Context, param1: str, param2: int) -> Dict[str, Any]:
    """Command description."""
    response = send_tcp_command("my_new_command", {
        "param1": param1,
        "param2": param2
    })
    return response
```

2. **Implement C++ Handler** (`Commands/*/MyNewCommand.cpp`):
```cpp
class FMyNewCommand : public IUnrealMCPCommand
{
public:
    virtual FString CommandName() const override
    {
        return TEXT("my_new_command");
    }

    virtual TSharedPtr<FJsonObject> Execute(
        const TSharedPtr<FJsonObject>& Params) override
    {
        // Implementation here
    }
};
```

3. **Register Command** (`Commands/UnrealMCPMainDispatcher.cpp`):
```cpp
Dispatcher->RegisterCommand(TEXT("my_new_command"),
    MakeShared<FMyNewCommand>());
```

### Synchronization Requirements

**Critical**: Python and C++ must stay synchronized:

✅ **Match these exactly:**
- Command name strings
- Parameter names
- Parameter types
- JSON schema structure
- Return value structure

❌ **Common mistakes:**
- Mismatched parameter names (`blueprint_path` vs `BlueprintPath`)
- Type mismatches (string vs integer)
- Missing required parameters
- Inconsistent JSON structure

---

## 📝 Documentation Types

### 1. Technical API Docs (This Section)

**Audience**: Developers extending the system

**Contents**:
- Command parameters and types
- JSON request/response schemas
- TCP protocol details
- C++ implementation patterns

**Files**: This directory (`Docs/Tools/*.md`)

### 2. Natural Language Guides

**Audience**: End users (game developers using AI assistants)

**Contents**:
- Natural language command examples
- Workflow tutorials
- Best practices
- Troubleshooting

**Files**: Parent directory (`Docs/*.md`)

---

## 🔗 Related Documentation

### User Guides
- **[Main Documentation](../README.md)** - Documentation index and learning paths
- **[Quick Start Guide](../Quick-Start-Guide.md)** - 15-minute tutorial
- **[Blueprint Tools Guide](../Blueprint-Tools.md)** - Natural language Blueprint examples
- **[UMG Tools Guide](../UMG-Tools.md)** - Natural language UI examples

### Developer Guides
- **[CLAUDE.md](../../CLAUDE.md)** - Architecture, development workflow, file size limits
- **[Architecture Guide](../../MCPGameProject/Plugins/UnrealMCP/Documentation/Architecture_Guide.md)** - C++ plugin architecture
- **[Python README](../../Python/README.md)** - Python server setup and structure

### API References
- **[Actor Tools API](actor_tools.md)** - Actor management commands
- **[Blueprint Tools API](blueprint_tools.md)** - Blueprint creation commands
- **[Node Tools API](node_tools.md)** - Visual scripting commands
- **[UMG Tools API](umg_tools.md)** - UI widget commands

---

## 🧪 Testing

### Direct TCP Testing

Test commands without MCP using Python scripts:

```bash
cd Python
source .venv/bin/activate  # or .venv\Scripts\activate on Windows
python scripts/test_blueprint.py
```

### MCP Testing

Test through AI assistant:
1. Ensure Unreal Editor is running
2. Use natural language commands with your AI assistant
3. Check Output Log in Unreal for detailed execution info

---

## 📊 Command Statistics

| Category | Commands | Most Used |
|----------|----------|-----------|
| Blueprint | 15+ | `create_blueprint`, `add_component`, `compile_blueprint` |
| Node | 12+ | `add_event_node`, `connect_nodes`, `create_node_by_action_name` |
| UMG | 10+ | `create_widget_blueprint`, `add_widget_component`, `set_widget_property` |
| Editor | 8+ | `spawn_actor`, `set_actor_transform`, `get_level_metadata` |
| DataTable | 7+ | `create_datatable`, `add_rows`, `get_datatable_rows` |
| Project | 9+ | `create_enhanced_input_action`, `create_folder`, `create_struct` |
| Blueprint Action | 5+ | `search_blueprint_actions`, `get_actions_for_class` |

**Total**: 66+ commands across 7 categories