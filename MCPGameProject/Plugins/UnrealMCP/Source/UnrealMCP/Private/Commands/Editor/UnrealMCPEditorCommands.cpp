#include "Commands/Editor/UnrealMCPEditorCommands.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPMainDispatcher.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // All Editor commands now route through the new main dispatcher and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_actors_in_level"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_actors_by_name"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("spawn_actor"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("delete_actor"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_actor_transform"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_actor_properties"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_actor_property"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetLightProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_light_property"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("spawn_blueprint_actor"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("focus_viewport"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("take_screenshot"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindAssetsByType(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_assets_by_type"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindAssetsByName(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_assets_by_name"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindWidgetBlueprints(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_widget_blueprints"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindBlueprints(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_blueprints"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindDataTables(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Editor commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("find_data_tables"), Params);
}

