# Unreal Engine MCP Documentation

Welcome! This is your comprehensive guide to controlling **Unreal Engine 5.7** through natural language using AI assistants via the Model Context Protocol (MCP).

## 🚀 Quick Start

**New to Unreal MCP?** Start here:

### [📘 Quick Start Guide](Quick-Start-Guide.md)
Get up and running in 15 minutes! This guide walks you through:
- Installing and configuring the system
- Connecting your AI assistant
- Creating your first Blueprint with natural language
- Building visual scripting logic
- Spawning characters in the scene

**Perfect for:** First-time users, testing the system, learning the basics

---

## 📚 Core Tool Documentation

Comprehensive guides for each tool category. Each guide includes natural language examples, advanced patterns, and real-world workflows.

### 🎬 [Animation Tools](Animation-Tools.md)
Create and manage Animation Blueprints, state machines, and animation variables
- Create Animation Blueprints for skeletons
- Build state machines with states and transitions
- Add animation variables (Float, Bool, Vector, etc.)
- Configure animation slots for montage playback
- Connect AnimGraph nodes to output pose
- Link animation layers for modular animation logic

### 🎮 [Blueprint Tools](Blueprint-Tools.md)
Create and manage Blueprint classes, components, variables, and game logic
- Create Blueprints with custom parent classes
- Add and configure components (meshes, lights, cameras, physics)
- Manage variables, properties, and interfaces
- Configure physics simulation and collision
- Compile and test Blueprints

### 🔍 [Blueprint Action Tools](Blueprint-Action-Tools.md)
Dynamically discover available Blueprint actions using Unreal's action database
- Search for available actions by pin type
- Discover class-specific functionality
- Explore inheritance hierarchies
- Create nodes dynamically based on action discovery
- Query pin requirements and types

### 🎯 [Node Tools](Node-Tools.md)
Build Blueprint visual scripting logic, event chains, and node connections
- Create event nodes (BeginPlay, Tick, custom events)
- Add function calls and variable references
- Connect nodes with execution and data flow
- Smart node replacement with connection preservation
- Graph manipulation (disconnect, delete, replace)

### 🖼️ [UMG Tools](UMG-Tools.md)
Design user interfaces, widgets, layouts, and interactive UI elements
- Create Widget Blueprints
- Add UI components (text, buttons, images, progress bars)
- Build nested layouts with containers
- Bind events and properties for dynamic UI
- Capture widget screenshots for AI inspection
- Export enhanced JSON metadata

### 🎬 [Editor Tools](Editor-Tools.md)
Control the Unreal Engine editor, manage actors, transforms, and scene objects
- Spawn and delete actors (primitives, lights, cameras)
- Transform actors (position, rotation, scale)
- Find actors by name patterns
- Configure light properties
- Control viewport camera
- Query and modify actor properties

### 🎨 [Material Tools](Material-Tools.md)
Create and manage Material Instances and their parameters
- Create Material Instances from parent materials
- Set scalar parameters (Metallic, Roughness, etc.)
- Set vector/color parameters (BaseColor, EmissiveColor)
- Set texture parameters (Diffuse, Normal, Roughness maps)
- Batch set multiple parameters in one operation
- Query available parameters from materials
- Duplicate Material Instances for variants

### ✨ [Niagara Tools](Niagara-Tools.md)
Create and manage Niagara particle systems, emitters, and VFX
- Create Niagara Systems for visual effects
- Add emitters from templates (Sprite, Ribbon, Mesh, Light)
- Configure float, vector, and color parameters
- Add renderers with custom materials
- Enable/disable emitters dynamically
- Duplicate systems for effect variants
- Compile and validate systems

### 📊 [DataTable Tools](DataTable-Tools.md)
Manage structured game data, create tables, and perform CRUD operations
- Create DataTables with custom structs
- Add, update, delete, and query rows
- Bulk operations on multiple rows
- Map properties using GUID automation
- List row names and struct fields

### 🌿 [PCG Tools](PCG-Tools.md)
Create and execute Procedural Content Generation graphs for procedural world-building
- Create PCG Graph assets with optional templates
- Add, connect, and configure PCG nodes (195+ node types)
- Search the PCG palette for available node types
- Set node properties using UE reflection
- Spawn actors with PCG components and assigned graphs
- Execute PCG generation on demand
- Inspect full graph structure: nodes, pins, connections, settings

### 📁 [Project Tools](Project-Tools.md)
Organize projects, manage input systems, create structs, and handle project structure
- Create and organize content folders
- Enhanced Input System (UE 5.5+)
  - Create Input Actions with value types
  - Create Input Mapping Contexts
  - Map keys with modifiers (Shift, Ctrl, Alt)
- Create and update custom structs
- Manage project folder structure

---

## 🎓 Learning Paths

### For Beginners
1. Start with the [Quick Start Guide](Quick-Start-Guide.md)
2. Learn [Blueprint Tools](Blueprint-Tools.md) to create game objects
3. Explore [Editor Tools](Editor-Tools.md) to manage your scene
4. Try [Node Tools](Node-Tools.md) for gameplay logic

### For Intermediate Users
1. Master [UMG Tools](UMG-Tools.md) for user interfaces
2. Use [DataTable Tools](DataTable-Tools.md) for game data
3. Explore [Blueprint Action Tools](Blueprint-Action-Tools.md) for advanced node discovery
4. Dive into [Project Tools](Project-Tools.md) for project organization

### For Advanced Users
- Combine multiple tool categories for complex workflows
- Use Blueprint Action discovery for dynamic node creation
- Implement smart node replacement for refactoring
- Build complete game systems using integrated workflows

### For VFX & Materials
1. Learn [Material Tools](Material-Tools.md) for material instances and parameters
2. Master [Niagara Tools](Niagara-Tools.md) for particle effects
3. Combine materials with Niagara for polished VFX

### For Procedural Content
1. Start with [PCG Tools](PCG-Tools.md) for graph creation
2. Search node types to discover available PCG building blocks
3. Build procedural pipelines (sample → filter → spawn)
4. Execute graphs in-editor for rapid iteration

### For Animation
1. Start with [Animation Tools](Animation-Tools.md) for Animation Blueprints
2. Create state machines for locomotion and actions
3. Add variables to drive animation logic
4. Configure slots for montage playback

---

## 💡 How to Use This Documentation

### Natural Language First
All examples show **natural language commands** you can use with your AI assistant. No need to learn specific syntax or APIs - just describe what you want!

**Example:**
```
"Create a Blueprint called BP_Enemy that inherits from Character"
"Add a sphere collision component called TriggerZone"
"Enable physics simulation with gravity"
```

### Progressive Examples
Each guide follows a progression:
1. **Basic Operations** - Simple, single-step tasks
2. **Common Patterns** - Frequently used combinations
3. **Advanced Workflows** - Complex, multi-step scenarios
4. **Real-World Examples** - Complete game systems

### Cross-Tool Integration
Many workflows combine multiple tools:

**Example: Complete Character with UI**
1. Use **Blueprint Tools** to create the character class
2. Use **Node Tools** to add behavior logic
3. Use **UMG Tools** to create a health bar
4. Use **Editor Tools** to spawn and test

---

## 📖 Additional Resources

### Optimization
- **[Context Optimization Guide](Context-Optimization.md)** - Reduce AI context usage by disabling unused MCP servers and skills

### Technical Documentation
- **[Tools](Tools/README.md)** - Technical implementation details and API reference
- **[Architecture Guide](../MCPGameProject/Plugins/UnrealMCP/Documentation/Architecture_Guide.md)** - C++ plugin architecture
- **[CLAUDE.md](../CLAUDE.md)** - Developer guidance for contributing

### Reference Materials
- **[Main README](../README.md)** - Project overview and setup
- **[Python README](../Python/README.md)** - Python server setup and testing
- **Example Scripts** - `Python/scripts/` directory for Python test examples

---

## 🎯 Common Use Cases

### Game Development
- **Character Creation**: Blueprints → Components → Physics → Logic → UI
- **Level Design**: Editor → Actors → Lights → Camera → Testing
- **UI Systems**: UMG → Widgets → Events → Bindings → Display
- **Data Management**: Structs → DataTables → CRUD → Game Logic

### Prototyping
- Quickly test gameplay ideas using natural language
- Iterate on designs without deep Unreal knowledge
- Experiment with different approaches rapidly

### Learning Unreal Engine
- Learn by doing with immediate feedback
- Understand concepts through natural language
- Explore Unreal's capabilities conversationally

### Automation
- Batch create and configure assets
- Standardize project setups
- Automate repetitive tasks

---

## 🤝 Getting Help

### Troubleshooting
Each tool guide includes common issues and solutions. Also see:
- [Quick Start Troubleshooting](Quick-Start-Guide.md#troubleshooting)
- [Main README Troubleshooting](../README.md#-troubleshooting)

### Community
- **GitHub Issues**: [Report bugs or request features](https://github.com/TrishynVolodymyr/unreal-mcp/issues)
- **Discussions**: Share workflows and ask questions

### Contributing
See [CLAUDE.md](../CLAUDE.md) for:
- Architecture guidelines
- Development workflow
- Adding new features
- Synchronization requirements

---

**Happy building with Unreal MCP!** 🎮 Start with the [Quick Start Guide](Quick-Start-Guide.md) to begin your journey.

