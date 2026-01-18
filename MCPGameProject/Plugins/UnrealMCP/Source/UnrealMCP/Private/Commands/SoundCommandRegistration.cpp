#include "Commands/SoundCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Services/SoundService.h"

// Include Phase 1 Sound command headers
#include "Commands/Sound/ImportSoundFileCommand.h"
#include "Commands/Sound/GetSoundWaveMetadataCommand.h"
#include "Commands/Sound/SpawnAmbientSoundCommand.h"
#include "Commands/Sound/CreateSoundAttenuationCommand.h"

// Include Phase 2 Sound Cue command headers
#include "Commands/Sound/CreateSoundCueCommand.h"
#include "Commands/Sound/GetSoundCueMetadataCommand.h"
#include "Commands/Sound/AddSoundCueNodeCommand.h"
#include "Commands/Sound/ConnectSoundCueNodesCommand.h"
#include "Commands/Sound/SetSoundCueNodePropertyCommand.h"
#include "Commands/Sound/RemoveSoundCueNodeCommand.h"
#include "Commands/Sound/CompileSoundCueCommand.h"

// Include Phase 3 MetaSound command headers
#include "Commands/Sound/CreateMetaSoundSourceCommand.h"
#include "Commands/Sound/GetMetaSoundMetadataCommand.h"
#include "Commands/Sound/AddMetaSoundNodeCommand.h"
#include "Commands/Sound/ConnectMetaSoundNodesCommand.h"
#include "Commands/Sound/SetMetaSoundInputCommand.h"
#include "Commands/Sound/AddMetaSoundInputCommand.h"
#include "Commands/Sound/AddMetaSoundOutputCommand.h"
#include "Commands/Sound/CompileMetaSoundCommand.h"
#include "Commands/Sound/SearchMetaSoundPaletteCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FSoundCommandRegistration::RegisteredCommands;

void FSoundCommandRegistration::RegisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Registering Sound commands..."));

    // Get the Sound service instance
    ISoundService& SoundService = FSoundService::Get();

    // Register Phase 1: Sound Wave and Audio Component commands
    RegisterAndTrackCommand(MakeShared<FImportSoundFileCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FGetSoundWaveMetadataCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FSpawnAmbientSoundCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FCreateSoundAttenuationCommand>(SoundService));

    // Register Phase 2: Sound Cue commands
    RegisterAndTrackCommand(MakeShared<FCreateSoundCueCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FGetSoundCueMetadataCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FAddSoundCueNodeCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FConnectSoundCueNodesCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FSetSoundCueNodePropertyCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FRemoveSoundCueNodeCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FCompileSoundCueCommand>(SoundService));

    // Register Phase 3: MetaSound commands
    RegisterAndTrackCommand(MakeShared<FCreateMetaSoundSourceCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FGetMetaSoundMetadataCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FAddMetaSoundNodeCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FConnectMetaSoundNodesCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FSetMetaSoundInputCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FAddMetaSoundInputCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FAddMetaSoundOutputCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FCompileMetaSoundCommand>(SoundService));
    RegisterAndTrackCommand(MakeShared<FSearchMetaSoundPaletteCommand>(SoundService));

    // TODO: Register Phase 4 Music System commands

    UE_LOG(LogTemp, Log, TEXT("Registered %d Sound commands"), RegisteredCommands.Num());
}

void FSoundCommandRegistration::UnregisterAllCommands()
{
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

    for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
    {
        if (Command.IsValid())
        {
            Registry.UnregisterCommand(Command->GetCommandName());
        }
    }

    RegisteredCommands.Empty();
    UE_LOG(LogTemp, Log, TEXT("Unregistered all Sound commands"));
}

void FSoundCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to register invalid Sound command"));
        return;
    }

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommands.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("Registered Sound command: %s"), *Command->GetCommandName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to register Sound command: %s"), *Command->GetCommandName());
    }
}
