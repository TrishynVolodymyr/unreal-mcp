#include "Commands/PCGCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"

// PCG command headers
#include "Commands/PCG/CreatePCGGraphCommand.h"
#include "Commands/PCG/GetPCGGraphMetadataCommand.h"
#include "Commands/PCG/SearchPCGPaletteCommand.h"
#include "Commands/PCG/AddPCGNodeCommand.h"
#include "Commands/PCG/ConnectPCGNodesCommand.h"
#include "Commands/PCG/SetPCGNodePropertyCommand.h"
#include "Commands/PCG/RemovePCGNodeCommand.h"
#include "Commands/PCG/SpawnPCGActorCommand.h"
#include "Commands/PCG/ExecutePCGGraphCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FPCGCommandRegistration::RegisteredCommands;

void FPCGCommandRegistration::RegisterAllCommands()
{
	UE_LOG(LogTemp, Log, TEXT("Registering PCG commands..."));

	RegisterAndTrackCommand(MakeShared<FCreatePCGGraphCommand>());
	RegisterAndTrackCommand(MakeShared<FGetPCGGraphMetadataCommand>());
	RegisterAndTrackCommand(MakeShared<FSearchPCGPaletteCommand>());
	RegisterAndTrackCommand(MakeShared<FAddPCGNodeCommand>());
	RegisterAndTrackCommand(MakeShared<FConnectPCGNodesCommand>());
	RegisterAndTrackCommand(MakeShared<FSetPCGNodePropertyCommand>());
	RegisterAndTrackCommand(MakeShared<FRemovePCGNodeCommand>());
	RegisterAndTrackCommand(MakeShared<FSpawnPCGActorCommand>());
	RegisterAndTrackCommand(MakeShared<FExecutePCGGraphCommand>());

	UE_LOG(LogTemp, Log, TEXT("Registered %d PCG commands"), RegisteredCommands.Num());
}

void FPCGCommandRegistration::UnregisterAllCommands()
{
	UE_LOG(LogTemp, Log, TEXT("Unregistering PCG commands..."));

	FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

	for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
	{
		if (Command.IsValid())
		{
			Registry.UnregisterCommand(Command->GetCommandName());
		}
	}

	RegisteredCommands.Empty();
	UE_LOG(LogTemp, Log, TEXT("Unregistered all PCG commands"));
}

void FPCGCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
	if (!Command.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Attempted to register invalid PCG command"));
		return;
	}

	FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

	if (Registry.RegisterCommand(Command))
	{
		RegisteredCommands.Add(Command);
		UE_LOG(LogTemp, Log, TEXT("Registered PCG command: %s"), *Command->GetCommandName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to register PCG command: %s"), *Command->GetCommandName());
	}
}
