# Quick Start Guide - Unreal MCP

Welcome! This guide will get you up and running with Unreal MCP in under 15 minutes. You'll learn how to control Unreal Engine 5.7 using natural language through AI assistants.

## What You'll Build

By the end of this guide, you'll use natural language to:
1. Create a Blueprint character
2. Add components and physics
3. Build visual scripting logic
4. Create a simple UI
5. Spawn the character in the scene

**No Unreal Engine expertise required** - just describe what you want!

## Prerequisites Checklist

Before starting, ensure you have:

- [ ] **Unreal Engine 5.7** installed ([Download](https://www.unrealengine.com/))
- [ ] **Python 3.10+** installed ([Download](https://www.python.org/downloads/))
- [ ] **Visual Studio 2022+** with C++ workload (Windows) or Xcode (macOS)
- [ ] **AI Assistant**: Claude Desktop, Cursor, or Windsurf
- [ ] **15 minutes** of your time

## Step 1: Setup the Project (5 minutes)

### 1.1 Clone or Download the Repository

```bash
git clone https://github.com/TrishynVolodymyr/unreal-mcp.git
cd unreal-mcp
```

Or download and extract the ZIP from GitHub.

### 1.2 Build the C++ Plugin

**Windows:**
```powershell
# Option A: Use the build script (recommended)
.\RebuildProject.bat

# Option B: Manual build
# 1. Right-click MCPGameProject.uproject
# 2. Select "Generate Visual Studio project files"
# 3. Open MCPGameProject.sln
# 4. Set configuration to "Development Editor"
# 5. Build Solution (Ctrl+Shift+B)
```

**macOS/Linux:**
```bash
# Generate project files
cd MCPGameProject
/path/to/UnrealEngine/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh MCPGameProject.uproject

# Build using Unreal Build Tool
/path/to/UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh MCPGameProjectEditor Mac Development
```

⏱️ **Build time:** 3-5 minutes on first build

### 1.3 Launch Unreal Editor

**Windows:**
```powershell
.\LaunchProject.bat
```

**macOS/Linux:**
```bash
open MCPGameProject/MCPGameProject.uproject
```

✅ **Success indicator:** Unreal Editor opens, and you see "MCP TCP Server started on 127.0.0.1:55557" in the Output Log (Window → Developer Tools → Output Log).

## Step 2: Setup Python MCP Servers (5 minutes)

### 2.1 Install uv (Python Package Manager)

```bash
# Windows (PowerShell)
powershell -c "irm https://astral.sh/uv/install.ps1 | iex"

# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh
```

### 2.2 Create Virtual Environment

```bash
cd Python
uv venv
```

### 2.3 Activate Virtual Environment

```powershell
# Windows
.venv\Scripts\activate

# macOS/Linux
source .venv/bin/activate
```

### 2.4 Install Dependencies

```bash
uv pip install -e .
```

⏱️ **Install time:** 30-60 seconds

## Step 3: Configure Your AI Assistant (3 minutes)

### 3.1 Choose Your Configuration Location

| AI Assistant | Configuration File |
|--------------|-------------------|
| **Claude Desktop** | `%USERPROFILE%\.config\claude-desktop\mcp.json` (Windows)<br>`~/.config/claude-desktop/mcp.json` (macOS/Linux) |
| **Cursor** | `.cursor/mcp.json` in your project root |
| **Windsurf** | `%USERPROFILE%\.config\windsurf\mcp.json` (Windows)<br>`~/.config/windsurf/mcp.json` (macOS/Linux) |

### 3.2 Add MCP Server Configuration

Create or edit the configuration file with the following content:

```json
{
  "mcpServers": {
    "blueprintMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "blueprint_mcp_server.py"]
    },
    "editorMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "editor_mcp_server.py"]
    },
    "umgMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "umg_mcp_server.py"]
    },
    "nodeMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "node_mcp_server.py"]
    },
    "datatableMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "datatable_mcp_server.py"]
    },
    "projectMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "project_mcp_server.py"]
    },
    "blueprintActionMCP": {
      "command": "uv",
      "args": ["--directory", "C:\\path\\to\\unreal-mcp\\Python", "run", "blueprint_action_mcp_server.py"]
    }
  }
}
```

**⚠️ Important:** Replace `C:\\path\\to\\unreal-mcp\\Python` with your actual path:
- Windows: Use double backslashes `C:\\Users\\YourName\\unreal-mcp\\Python`
- macOS/Linux: Use forward slashes `/home/username/unreal-mcp/Python`

### 3.3 Restart Your AI Assistant

Close and reopen your AI assistant application to load the new configuration.

✅ **Verification:** In Claude Desktop, click the 🔌 icon in the bottom right. You should see 7 MCP servers listed (blueprintMCP, editorMCP, umgMCP, nodeMCP, datatableMCP, projectMCP, blueprintActionMCP).

## Step 4: Your First Natural Language Commands (2 minutes)

Now the fun part! Open your AI assistant and try these commands:

### Test 1: Create a Blueprint

```
Create a Blueprint called BP_Player that inherits from Character
```

✅ **Expected:** AI creates the Blueprint, you'll see confirmation and the path `/Game/BP_Player`

### Test 2: Add Components

```
Add a static mesh component called Body to BP_Player
Add a camera component called PlayerCamera at position [0, 0, 100]
```

✅ **Expected:** Components are added to the Blueprint

### Test 3: Build Visual Logic

```
In BP_Player, add a BeginPlay event
Create a PrintString node that says "Player spawned!"
Connect the BeginPlay event to the PrintString node
```

✅ **Expected:** Visual scripting nodes are created and connected

### Test 4: Compile the Blueprint

```
Compile BP_Player
```

✅ **Expected:** Blueprint compiles successfully with no errors

### Test 5: Spawn in the Scene

```
Spawn an instance of BP_Player at position [0, 0, 100]
```

✅ **Expected:** Character appears in the Unreal Editor viewport, and "Player spawned!" prints to the Output Log when you press Play (Alt+P)

## 🎉 Success!

You've successfully:
- ✅ Built and launched the Unreal MCP system
- ✅ Configured your AI assistant with MCP servers
- ✅ Created a Blueprint using natural language
- ✅ Added components and visual scripting
- ✅ Spawned the character in the scene

## What's Next?

### Learn More Tools

Explore the comprehensive documentation for each tool category:

| Guide | What You'll Learn |
|-------|------------------|
| [Blueprint Tools](Blueprint-Tools.md) | Create complex Blueprints, manage components, variables, physics |
| [Node Tools](Node-Tools.md) | Build advanced visual scripting logic and event chains |
| [UMG Tools](UMG-Tools.md) | Design user interfaces, health bars, menus, HUDs |
| [Editor Tools](Editor-Tools.md) | Control the scene, manage actors, lighting, cameras |
| [DataTable Tools](DataTable-Tools.md) | Manage game data, items, characters, stats |
| [Project Tools](Project-Tools.md) | Organize projects, setup inputs, create structs |
| [Blueprint Action Tools](Blueprint-Action-Tools.md) | Discover and create nodes dynamically |

### Try Example Workflows

**Create a Health System:**
```
1. Create a Widget Blueprint called WBP_HealthBar
2. Add a progress bar and text block
3. Bind the progress bar to a health variable
4. Add the widget to the viewport
```

**Build a Pickup System:**
```
1. Create a Blueprint called BP_Pickup that inherits from Actor
2. Add a sphere collision component
3. Add a static mesh component
4. Create an OnComponentBeginOverlap event
5. When overlapped, print "Item picked up!" and destroy the actor
6. Spawn some pickups in the scene
```

**Setup Enhanced Input:**
```
1. Create an Input Action called IA_Jump with Digital value type
2. Create an Input Mapping Context called IMC_Player
3. Map the Space key to IA_Jump
4. List all input actions to verify
```

### Explore Advanced Features

- **Dynamic Node Discovery:** Use Blueprint Action tools to discover available functions for any class
- **Smart Node Replacement:** Replace nodes while preserving connections with automatic type casting
- **Widget Screenshots:** Capture UI screenshots for AI visual inspection
- **DataTable Automation:** Bulk operations on game data tables

## Troubleshooting

### Connection Failed

**Problem:** AI assistant can't connect to Unreal Engine

**Solutions:**
1. Ensure Unreal Editor is running with MCPGameProject open
2. Check Output Log for "MCP TCP Server started on 127.0.0.1:55557"
3. Verify no firewall is blocking localhost:55557
4. Restart Unreal Editor if the TCP server didn't start

### MCP Servers Not Found

**Problem:** AI assistant doesn't show the MCP servers

**Solutions:**
1. Verify the path in your MCP configuration file is correct
2. Ensure you used double backslashes on Windows (`C:\\path\\`)
3. Restart your AI assistant after changing configuration
4. Check that Python virtual environment was created correctly

### Build Errors

**Problem:** C++ plugin won't build

**Solutions:**
1. Verify Unreal Engine 5.7 is installed correctly
2. Ensure Visual Studio 2022 has C++ Desktop Development workload
3. Try cleaning: Delete `Binaries/`, `Intermediate/`, and `Saved/` folders, then rebuild
4. Run `RebuildProject.bat` which automatically cleans before building

### Python Import Errors

**Problem:** Dependencies not found when running MCP servers

**Solutions:**
1. Ensure virtual environment is activated (you should see `.venv` in your prompt)
2. Run `uv pip install -e .` again in the Python folder
3. Try deleting `.venv` folder and recreating: `uv venv` then `uv pip install -e .`

### Command Not Working

**Problem:** Natural language command doesn't execute

**Solutions:**
1. Check that the Blueprint/Actor/Asset exists with the exact name
2. Verify the Blueprint is compiled (compilation errors prevent operations)
3. Look at the AI assistant's response for error details
4. Check Unreal Editor Output Log for detailed error messages
5. Try a simpler version of the command first

## Getting Help

- **Documentation:** [Complete documentation index](README.md)
- **Architecture Guide:** [CLAUDE.md](../CLAUDE.md) for development details
- **Issues:** [GitHub Issues](https://github.com/TrishynVolodymyr/unreal-mcp/issues)
- **Examples:** Check `Python/scripts/` for Python test scripts

## Quick Reference Commands

### Blueprint Operations
```
Create a Blueprint called BP_MyActor based on Actor
Add a cube component called MyCube to BP_MyActor
Compile BP_MyActor
```

### Editor Operations
```
Spawn a point light at [0, 0, 100]
Move the MyActor to position [200, 0, 50]
Find all actors with "Player" in their name
```

### Node Graph Operations
```
Add a BeginPlay event to BP_MyActor
Add a PrintString node with text "Hello World"
Connect BeginPlay to PrintString
```

### UMG Operations
```
Create a Widget Blueprint called WBP_MyUI
Add a text block with content "Score: 0"
Add a button called StartButton
```

### Data Operations
```
Create a struct called S_ItemData with Name (string) and Value (integer)
Create a DataTable called DT_Items using S_ItemData
Add a row to DT_Items with Name "Sword" and Value 100
```

---

**Ready for more?** Dive into the [full documentation](README.md) to master Unreal MCP! 🚀
