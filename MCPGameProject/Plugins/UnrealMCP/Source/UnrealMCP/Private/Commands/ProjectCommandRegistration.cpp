#include "Commands/ProjectCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/Project/CreateInputMappingCommand.h"
#include "Commands/Project/CreateFolderCommand.h"
#include "Commands/Project/CreateStructCommand.h"
#include "Commands/Project/GetProjectDirCommand.h"
#include "Commands/Project/CreateEnhancedInputActionCommand.h"
#include "Commands/Project/CreateInputMappingContextCommand.h"
#include "Commands/Project/AddMappingToContextCommand.h"
#include "Commands/Project/UpdateStructCommand.h"
#include "Commands/Project/GetProjectMetadataCommand.h"
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

    UE_LOG(LogTemp, Log, TEXT("Registered project commands successfully"));
}
