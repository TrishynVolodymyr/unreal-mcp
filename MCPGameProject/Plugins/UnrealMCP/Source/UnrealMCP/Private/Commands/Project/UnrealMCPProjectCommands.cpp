#include "Commands/Project/UnrealMCPProjectCommands.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPMainDispatcher.h"

FUnrealMCPProjectCommands::FUnrealMCPProjectCommands()
{
}

FUnrealMCPProjectCommands::~FUnrealMCPProjectCommands()
{
}

void FUnrealMCPProjectCommands::InitializeServices()
{
    // Services are now initialized by the main dispatcher
}

void FUnrealMCPProjectCommands::RegisterCommands()
{
    // Commands are now registered by the main dispatcher
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // All Project commands now route through the new main dispatcher and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleLegacyCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleUpdateStruct(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("update_struct"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleShowStructVariables(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("show_struct_variables"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListFolderContents(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("list_folder_contents"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateEnhancedInputAction(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_enhanced_input_action"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_input_mapping_context"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleAddMappingToContext(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_mapping_to_context"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListInputActions(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("list_input_actions"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListInputMappingContexts(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Project commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("list_input_mapping_contexts"), Params);
}

