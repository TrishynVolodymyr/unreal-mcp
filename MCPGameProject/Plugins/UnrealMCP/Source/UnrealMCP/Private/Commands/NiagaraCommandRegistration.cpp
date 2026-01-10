#include "Commands/NiagaraCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Services/NiagaraService.h"

// Include all Niagara command headers
// Feature 1: Core Asset Management
#include "Commands/Niagara/CreateNiagaraSystemCommand.h"
#include "Commands/Niagara/CreateNiagaraEmitterCommand.h"
#include "Commands/Niagara/AddEmitterToSystemCommand.h"
#include "Commands/Niagara/SetEmitterEnabledCommand.h"
#include "Commands/Niagara/RemoveEmitterFromSystemCommand.h"
#include "Commands/Niagara/SetEmitterPropertyCommand.h"
#include "Commands/Niagara/GetEmitterPropertiesCommand.h"
#include "Commands/Niagara/GetNiagaraMetadataCommand.h"
#include "Commands/Niagara/CompileNiagaraAssetCommand.h"

// Feature 2: Module System
#include "Commands/Niagara/SearchNiagaraModulesCommand.h"
#include "Commands/Niagara/AddModuleToEmitterCommand.h"
#include "Commands/Niagara/SetModuleInputCommand.h"
#include "Commands/Niagara/MoveModuleCommand.h"
#include "Commands/Niagara/SetModuleCurveInputCommand.h"
#include "Commands/Niagara/SetModuleColorCurveInputCommand.h"
#include "Commands/Niagara/SetModuleRandomInputCommand.h"
#include "Commands/Niagara/GetModuleInputsCommand.h"
#include "Commands/Niagara/GetEmitterModulesCommand.h"
#include "Commands/Niagara/RemoveModuleFromEmitterCommand.h"

// Feature 3: Parameters
#include "Commands/Niagara/AddNiagaraParameterCommand.h"
#include "Commands/Niagara/SetNiagaraParameterCommand.h"

// Python MCP-compatible typed parameter commands
#include "Commands/Niagara/SetNiagaraFloatParamCommand.h"
#include "Commands/Niagara/SetNiagaraVectorParamCommand.h"
#include "Commands/Niagara/SetNiagaraColorParamCommand.h"
#include "Commands/Niagara/GetNiagaraParametersCommand.h"
#include "Commands/Niagara/GetNiagaraSystemMetadataCommand.h"
#include "Commands/Niagara/CompileNiagaraSystemCommand.h"
#include "Commands/Niagara/DuplicateNiagaraSystemCommand.h"

// Feature 4: Data Interfaces
#include "Commands/Niagara/AddDataInterfaceCommand.h"
#include "Commands/Niagara/SetDataInterfacePropertyCommand.h"

// Feature 5: Renderers
#include "Commands/Niagara/AddRendererCommand.h"
#include "Commands/Niagara/SetRendererPropertyCommand.h"
#include "Commands/Niagara/GetRendererPropertiesCommand.h"

// Feature 6: Level Integration
#include "Commands/Niagara/SpawnNiagaraActorCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FNiagaraCommandRegistration::RegisteredCommands;

void FNiagaraCommandRegistration::RegisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Registering Niagara commands..."));

    // Get the Niagara service instance
    INiagaraService& NiagaraService = FNiagaraService::Get();

    // Register Feature 1: Core Asset Management commands
    RegisterAndTrackCommand(MakeShared<FCreateNiagaraSystemCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FCreateNiagaraEmitterCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FAddEmitterToSystemCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetEmitterEnabledCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FRemoveEmitterFromSystemCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetEmitterPropertyCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetEmitterPropertiesCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetNiagaraMetadataCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FCompileNiagaraAssetCommand>(NiagaraService));

    // Register Feature 2: Module System commands
    RegisterAndTrackCommand(MakeShared<FSearchNiagaraModulesCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FAddModuleToEmitterCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetModuleInputCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FMoveModuleCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetModuleCurveInputCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetModuleColorCurveInputCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetModuleRandomInputCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetModuleInputsCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetEmitterModulesCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FRemoveModuleFromEmitterCommand>(NiagaraService));

    // Register Feature 3: Parameter commands
    RegisterAndTrackCommand(MakeShared<FAddNiagaraParameterCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetNiagaraParameterCommand>(NiagaraService));

    // Register Python MCP-compatible typed parameter commands
    RegisterAndTrackCommand(MakeShared<FSetNiagaraFloatParamCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetNiagaraVectorParamCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetNiagaraColorParamCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetNiagaraParametersCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetNiagaraSystemMetadataCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FCompileNiagaraSystemCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FDuplicateNiagaraSystemCommand>(NiagaraService));

    // Register Feature 4: Data Interface commands
    RegisterAndTrackCommand(MakeShared<FAddDataInterfaceCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetDataInterfacePropertyCommand>(NiagaraService));

    // Register Feature 5: Renderer commands
    RegisterAndTrackCommand(MakeShared<FAddRendererCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FSetRendererPropertyCommand>(NiagaraService));
    RegisterAndTrackCommand(MakeShared<FGetRendererPropertiesCommand>(NiagaraService));

    // Register Feature 6: Level Integration commands
    RegisterAndTrackCommand(MakeShared<FSpawnNiagaraActorCommand>(NiagaraService));

    UE_LOG(LogTemp, Log, TEXT("Registered %d Niagara commands"), RegisteredCommands.Num());
}

void FNiagaraCommandRegistration::UnregisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Unregistering Niagara commands..."));

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

    for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
    {
        if (Command.IsValid())
        {
            Registry.UnregisterCommand(Command->GetCommandName());
        }
    }

    RegisteredCommands.Empty();
    UE_LOG(LogTemp, Log, TEXT("Unregistered all Niagara commands"));
}

void FNiagaraCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Attempted to register invalid Niagara command"));
        return;
    }

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommands.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("Registered Niagara command: %s"), *Command->GetCommandName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to register Niagara command: %s"), *Command->GetCommandName());
    }
}
