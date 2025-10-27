#include "Commands/BlueprintNode/UnrealMCPBlueprintNodeCommands.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPMainDispatcher.h"

FUnrealMCPBlueprintNodeCommands::FUnrealMCPBlueprintNodeCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // All Blueprint Node commands now route through the new main dispatcher and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("connect_blueprint_nodes"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_blueprint_event_node"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_blueprint_function_node"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_blueprint_variable"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_blueprint_input_action_node"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_blueprint_nodes"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintCustomEventNode(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_blueprint_custom_event_node"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetVariableInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint Node commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_variable_info"), Params);
}

// REMOVED: Enhanced Input Action nodes now created via Blueprint Action system
// Use create_node_by_action_name with function_name="EnhancedInputAction {ActionName}" instead
