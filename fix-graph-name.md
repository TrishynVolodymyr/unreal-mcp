# Task: Add graph_name/target_graph support to BlueprintNode commands

## Problem
`add_blueprint_variable_node`, `add_blueprint_function_node`, and `add_blueprint_event_node` always add nodes to EventGraph. They don't support a `graph_name` parameter to target specific function graphs (e.g., "UpdateValueDisplay").

The underlying `BlueprintNodeCreationService::CreateNodeByActionName()` already supports a `TargetGraph` parameter (defaults to "EventGraph"), but the commands and service layer don't pass it through.

## Files to modify

### 1. IBlueprintNodeService.h
Location: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/Services/IBlueprintNodeService.h`
- Add `const FString& TargetGraph = TEXT("EventGraph")` parameter to:
  - `AddVariableNode()`
  - `AddEventNode()` 
  - `AddFunctionNode()`

### 2. BlueprintNodeService.h
Location: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/Services/BlueprintNodeService.h`
- Update override declarations to match new interface signatures

### 3. BlueprintNodeService.cpp
Location: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/BlueprintNodeService.cpp`
- `AddVariableNode()` (line ~125): Pass TargetGraph to `CreationService.CreateNodeByActionName()` as the last parameter
- `AddEventNode()` (line ~165): Same
- `AddFunctionNode()` (find it): Same

### 4. AddBlueprintVariableNodeCommand.cpp
Location: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintNode/AddBlueprintVariableNodeCommand.cpp`
- Parse optional `graph_name` from JSON parameters (default to empty string or "EventGraph")
- Pass it to `BlueprintNodeService.AddVariableNode()`

### 5. AddBlueprintFunctionNodeCommand.cpp
Location: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintNode/AddBlueprintFunctionNodeCommand.cpp`
- Same: parse `graph_name`, pass to service

### 6. AddBlueprintEventNodeCommand.cpp
Location: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintNode/AddBlueprintEventNodeCommand.cpp`
- Same: parse `graph_name`, pass to service

## Pattern reference
Look at `CreateNodeByActionNameCommand.cpp` - it already parses `target_graph` and passes it through. Follow the same pattern but use parameter name `graph_name` in JSON for consistency with other commands (they already silently accept but ignore it).

## Important
- Keep backward compatibility (graph_name is optional, defaults to EventGraph)
- Don't change any other behavior
- This is C++ Unreal Engine code
