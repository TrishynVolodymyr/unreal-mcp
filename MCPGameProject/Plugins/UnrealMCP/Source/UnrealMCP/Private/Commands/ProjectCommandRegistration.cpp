#include "Commands/ProjectCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/Project/CreateInputMappingCommand.h"
#include "Commands/Project/CreateFolderCommand.h"
#include "Commands/Project/CreateStructCommand.h"
#include "Commands/Project/GetProjectDirCommand.h"
#include "Commands/Project/CreateEnhancedInputActionCommand.h"
#include "Commands/Project/CreateInputMappingContextCommand.h"
#include "Commands/Project/AddMappingToContextCommand.h"
#include "Commands/Project/ListInputActionsCommand.h"
#include "Commands/Project/ListInputMappingContextsCommand.h"
#include "Commands/Project/UpdateStructCommand.h"
#include "Commands/Project/ShowStructVariablesCommand.h"
#include "Commands/Project/ListFolderContentsCommand.h"
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
    Registry.RegisterCommand(MakeShared<FListInputActionsCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FListInputMappingContextsCommand>(ProjectService));
    
    // Register remaining project commands
    Registry.RegisterCommand(MakeShared<FUpdateStructCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FShowStructVariablesCommand>(ProjectService));
    Registry.RegisterCommand(MakeShared<FListFolderContentsCommand>(ProjectService));

    UE_LOG(LogTemp, Log, TEXT("Registered project commands successfully"));
}
