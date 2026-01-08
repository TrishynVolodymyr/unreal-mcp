#include "Commands/MaterialCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Services/MaterialService.h"

// Include all material command headers
#include "Commands/Material/CreateMaterialCommand.h"
#include "Commands/Material/CreateMaterialInstanceCommand.h"
#include "Commands/Material/GetMaterialMetadataCommand.h"
#include "Commands/Material/SetMaterialParameterCommand.h"
#include "Commands/Material/GetMaterialParameterCommand.h"
#include "Commands/Material/ApplyMaterialToActorCommand.h"

// Python MCP-compatible parameter commands
#include "Commands/Material/SetMaterialScalarParamCommand.h"
#include "Commands/Material/SetMaterialVectorParamCommand.h"
#include "Commands/Material/SetMaterialTextureParamCommand.h"
#include "Commands/Material/DuplicateMaterialInstanceCommand.h"
#include "Commands/Material/BatchSetMaterialParamsCommand.h"
#include "Commands/Material/GetMaterialInstanceMetadataCommand.h"
#include "Commands/Material/GetMaterialParametersCommand.h"

// Material expression command headers
#include "Commands/Material/AddMaterialExpressionCommand.h"
#include "Commands/Material/ConnectMaterialExpressionsCommand.h"
#include "Commands/Material/ConnectExpressionToMaterialOutputCommand.h"
#include "Commands/Material/GetMaterialExpressionMetadataCommand.h"
#include "Commands/Material/DeleteMaterialExpressionCommand.h"
#include "Commands/Material/SetMaterialExpressionPropertyCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FMaterialCommandRegistration::RegisteredCommands;

void FMaterialCommandRegistration::RegisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Registering Material commands..."));

    // Get the material service instance
    IMaterialService& MaterialService = FMaterialService::Get();

    // Register material manipulation commands
    RegisterAndTrackCommand(MakeShared<FCreateMaterialCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FCreateMaterialInstanceCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FGetMaterialMetadataCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FSetMaterialParameterCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FGetMaterialParameterCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FApplyMaterialToActorCommand>(MaterialService));

    // Register Python MCP-compatible parameter commands
    RegisterAndTrackCommand(MakeShared<FSetMaterialScalarParamCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FSetMaterialVectorParamCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FSetMaterialTextureParamCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FDuplicateMaterialInstanceCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FBatchSetMaterialParamsCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FGetMaterialInstanceMetadataCommand>(MaterialService));
    RegisterAndTrackCommand(MakeShared<FGetMaterialParametersCommand>(MaterialService));

    // Register material expression commands
    RegisterAndTrackCommand(MakeShared<FAddMaterialExpressionCommand>());
    RegisterAndTrackCommand(MakeShared<FConnectMaterialExpressionsCommand>());
    RegisterAndTrackCommand(MakeShared<FConnectExpressionToMaterialOutputCommand>());
    RegisterAndTrackCommand(MakeShared<FGetMaterialExpressionMetadataCommand>());
    RegisterAndTrackCommand(MakeShared<FDeleteMaterialExpressionCommand>());
    RegisterAndTrackCommand(MakeShared<FSetMaterialExpressionPropertyCommand>());

    UE_LOG(LogTemp, Log, TEXT("Registered %d Material commands"), RegisteredCommands.Num());
}

void FMaterialCommandRegistration::UnregisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Unregistering Material commands..."));

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

    for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
    {
        if (Command.IsValid())
        {
            Registry.UnregisterCommand(Command->GetCommandName());
        }
    }

    RegisteredCommands.Empty();
    UE_LOG(LogTemp, Log, TEXT("Unregistered all Material commands"));
}

void FMaterialCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Attempted to register invalid Material command"));
        return;
    }

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommands.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("Registered Material command: %s"), *Command->GetCommandName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to register Material command: %s"), *Command->GetCommandName());
    }
}
