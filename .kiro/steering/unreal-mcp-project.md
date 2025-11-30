# Unreal MCP Project Context

## Product Overview
This is an experimental Model Context Protocol (MCP) integration for Unreal Engine 5.7 that enables AI assistants to control Unreal Engine through natural language commands.

## Tech Stack
- **Engine**: Unreal Engine 5.7
- **Plugin**: C++ UnrealMCP plugin (TCP server on port 55557)
- **Python**: 7 MCP servers using FastMCP library
- **Protocol**: Model Context Protocol for AI assistant integration
- **Clients**: Claude Desktop, Cursor, Windsurf, Kiro

## Architecture
- **MCPGameProject/**: Sample UE 5.7 project with UnrealMCP plugin
- **Python/**: 7 specialized MCP servers (*_mcp_server.py files)
- **Docs/**: Comprehensive documentation for each tool category

## Key Capabilities
- **Actor Management**: Create/delete/transform actors, lights, cameras
- **Blueprint Development**: Create Blueprints, add components, variables, physics
- **Node Graph**: Build Blueprint visual scripting logic and connections
- **UMG/UI**: Create widgets, layouts, event bindings
- **DataTables**: Manage structured game data with CRUD operations
- **Project Management**: Input systems, structs, folder organization
- **Editor Control**: Viewport control, scene management

## Development Guidelines
- Use natural language commands through AI assistants
- All operations performed via MCP tool calls
- Focus on minimal, functional implementations
- Prioritize working examples over complex features
- When python files are changed - you need to restart MCP servers
- When CPP files are changed - you must call ./RebuildProject.bat to compile it and then ./LaunchProject.bat to run it, after that changed/created MCP tool should be tested by calling in with launched UE project
- The best way to use UE API - is to google how things are done, official documentation or ue developers forums might help, but you better do it with using google and do not use inexisting URLs
- UE log file to check for UE runtime errors `MCPGameProject\Saved\Logs\MCPGameProject.log`
- Live Coding is active compilation error message means tha UE is running at the moment and you need to kill its process.