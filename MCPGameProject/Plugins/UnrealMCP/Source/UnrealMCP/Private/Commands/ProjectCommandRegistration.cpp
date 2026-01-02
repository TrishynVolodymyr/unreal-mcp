#include "Commands/ProjectCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/Project/CreateInputMappingCommand.h"
#include "Commands/Project/CreateFolderCommand.h"
#include "Commands/Project/CreateStructCommand.h"
#include "Commands/Project/CreateEnumCommand.h"
#include "Commands/Project/UpdateEnumCommand.h"
#include "Commands/Project/GetProjectDirCommand.h"
#include "Commands/Project/CreateEnhancedInputActionCommand.h"
#include "Commands/Project/CreateInputMappingContextCommand.h"
#include "Commands/Project/AddMappingToContextCommand.h"
#include "Commands/Project/UpdateStructCommand.h"
#include "Commands/Project/GetProjectMetadataCommand.h"
#include "Commands/Project/GetStructPinNamesCommand.h"
#include "Commands/Project/DuplicateAssetCommand.h"
#include "Commands/Project/CreateFontFaceCommand.h"
#include "Commands/Project/SetFontFacePropertiesCommand.h"
#include "Commands/Project/GetFontFaceMetadataCommand.h"
#include "Commands/Project/CreateOfflineFontCommand.h"
#include "Commands/Project/GetFontMetadataCommand.h"
#include "Commands/Project/CreateFontCommand.h"
#include "Commands/Project/SearchAssetsCommand.h"
#include "Services/IProjectService.h"

void FProjectCommandRegistration::RegisterCommands(FUnrealMCPCommandRegistry& Registry, TSharedPtr<IProjectService> ProjectService)
{
    if (!ProjectService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("ProjectService is null, cannot register project commands"));
        return;
    }

    // Register input mapping command
    Registry.RegisterCommand(MakeShared<FCreateInputMappingCommand>(ProjectService));
    
    // Register folder command
    Registry.RegisterCommand(MakeShared<FCreateFolderCommand>(ProjectService));
    
    // Register struct command
    Registry.RegisterCommand(MakeShared<FCreateStructCommand>(ProjectService));

    // Register enum commands
    Registry.RegisterCommand(MakeShared<FCreateEnumCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FUpdateEnumCommand>(ProjectService));

    // Register get project directory command
    Registry.RegisterCommand(MakeShared<FGetProjectDirCommand>(ProjectService));
    
    // Register Enhanced Input commands
    Registry.RegisterCommand(MakeShared<FCreateEnhancedInputActionCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FCreateInputMappingContextCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FAddMappingToContextCommand>(ProjectService));

    // Register struct commands
    Registry.RegisterCommand(MakeShared<FUpdateStructCommand>(ProjectService));

    // Register consolidated metadata command (replaces list_input_actions, list_input_mapping_contexts, show_struct_variables, list_folder_contents)
    Registry.RegisterCommand(MakeShared<FGetProjectMetadataCommand>(ProjectService));

    // Register struct pin names command for discovering struct field/pin names
    Registry.RegisterCommand(MakeShared<FGetStructPinNamesCommand>(ProjectService));

    // Register asset duplication command
    Registry.RegisterCommand(MakeShared<FDuplicateAssetCommand>(ProjectService));

    // Register asset search command (uses FAssetDiscoveryService singleton, no ProjectService needed)
    Registry.RegisterCommand(MakeShared<FSearchAssetsCommand>());

    // Register unified font command (recommended - consolidates all font creation methods)
    Registry.RegisterCommand(MakeShared<FCreateFontCommand>(ProjectService));

    // Register legacy font face commands (TTF-based) - kept for backwards compatibility
    Registry.RegisterCommand(MakeShared<FCreateFontFaceCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FSetFontFacePropertiesCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FGetFontFaceMetadataCommand>(ProjectService));

    // Register legacy offline font commands (SDF atlas-based) - kept for backwards compatibility
    Registry.RegisterCommand(MakeShared<FCreateOfflineFontCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FGetFontMetadataCommand>(ProjectService));

    UE_LOG(LogTemp, Log, TEXT("Registered project commands successfully"));
}
