#include "Commands/EditorCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Services/EditorService.h"

// Include all editor command headers
#include "Commands/Editor/SpawnActorCommand.h"
#include "Commands/Editor/DeleteActorCommand.h"
#include "Commands/Editor/SpawnBlueprintActorCommand.h"
#include "Commands/Editor/SetActorTransformCommand.h"
#include "Commands/Editor/GetActorPropertiesCommand.h"
#include "Commands/Editor/SetActorPropertyCommand.h"
#include "Commands/Editor/SetLightPropertyCommand.h"
#include "Commands/Editor/GetLevelMetadataCommand.h"
#include "Commands/Editor/BatchDeleteActorsCommand.h"
#include "Commands/Editor/BatchSpawnActorsCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FEditorCommandRegistration::RegisteredCommands;

void FEditorCommandRegistration::RegisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Registering Editor commands..."));
    
    // Get the editor service instance
    IEditorService& EditorService = FEditorService::Get();
    
    // Register actor manipulation commands
    RegisterAndTrackCommand(MakeShared<FSpawnActorCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FDeleteActorCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FSpawnBlueprintActorCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FSetActorTransformCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FGetActorPropertiesCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FSetActorPropertyCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FSetLightPropertyCommand>(EditorService));

    // Register consolidated metadata command (replaces get_actors_in_level, find_actors_by_name)
    RegisterAndTrackCommand(MakeShared<FGetLevelMetadataCommand>(EditorService));

    // Register batch operations
    RegisterAndTrackCommand(MakeShared<FBatchDeleteActorsCommand>(EditorService));
    RegisterAndTrackCommand(MakeShared<FBatchSpawnActorsCommand>(EditorService));

    // Note: Additional editor commands are handled by legacy command system
    // and will be migrated to the new architecture in future iterations:
    // - SetActorTransformCommand, GetActorPropertiesCommand, etc.
    // RegisterAndTrackCommand(MakeShared<FSetActorPropertyCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FSetLightPropertyCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FFocusViewportCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FTakeScreenshotCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FFindAssetsByTypeCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FFindAssetsByNameCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FFindWidgetBlueprintsCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FFindBlueprintsCommand>(EditorService));
    // RegisterAndTrackCommand(MakeShared<FFindDataTablesCommand>(EditorService));
    
    UE_LOG(LogTemp, Log, TEXT("Registered %d Editor commands"), RegisteredCommands.Num());
}

void FEditorCommandRegistration::UnregisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Unregistering Editor commands..."));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
    {
        if (Command.IsValid())
        {
            Registry.UnregisterCommand(Command->GetCommandName());
        }
    }
    
    RegisteredCommands.Empty();
    UE_LOG(LogTemp, Log, TEXT("Unregistered all Editor commands"));
}

void FEditorCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Attempted to register invalid Editor command"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommands.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("Registered Editor command: %s"), *Command->GetCommandName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to register Editor command: %s"), *Command->GetCommandName());
    }
}
