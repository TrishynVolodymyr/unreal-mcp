---
description: 
globs: 
alwaysApply: true
---
Unreal MCP Project Architecture
Core Components
The Unreal MCP project consists of two essential components that must remain synchronized:
Python Server Component
Located in the Python/ directory
Handles MCP protocol communication
Provides tool modules for AI assistants
Various tools for Unreal Engine development like: blueprint_action_mcp_server.py, datatable_mcp_server.py, etc.
Unreal Engine Plugin is working with Unreal Engine 5.6 version
Located in MCPGameProject/Plugins/UnrealMCP/
Native C++ implementation within Unreal Engine
Executes commands received from the Python server
Interfaces with Unreal Editor subsystems
Synchronization Requirements
Critical: Both components must be kept in sync when adding new functionality.
When extending the system:
If adding a new tool in Python (Python/tools/), a corresponding implementation must be added to the Unreal plugin
If adding new features to the Unreal plugin, the Python interface must be updated to expose these capabilities
Function signatures, parameter naming, and data structures should match between both sides
Development Workflow
Plan the feature across both components
Implement the Unreal plugin side functionality
Add corresponding Python tool implementation
Test the end-to-end workflow through the MCP interface
Document new capabilities in both components
Common Extension Patterns
Most extensions follow this pattern:
Add a new function in a Python tool module (e.g., Python/tools/blueprint_tools.py)
Implement the corresponding C++ functionality in the Unreal plugin
Test through an AI assistant using natural language commands

This dual-component architecture allows for powerful natural language control of Unreal Engine while maintaining a clean separation of concerns.

After successfull additions you need to compile project running ./RebuildProject.bat
If compilation was successfull you need to run ./LaunchProject.bat