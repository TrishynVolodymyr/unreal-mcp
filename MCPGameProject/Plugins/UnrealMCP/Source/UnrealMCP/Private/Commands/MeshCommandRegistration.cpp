#include "Commands/MeshCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"

#include "Commands/Mesh/GetStaticMeshMetadataCommand.h"
#include "Commands/Mesh/SetLodCountCommand.h"
#include "Commands/Mesh/ImportLodCommand.h"
#include "Commands/Mesh/SetLodScreenSizesCommand.h"
#include "Commands/Mesh/AutoGenerateLodsCommand.h"
#include "Commands/Mesh/SetMeshPropertiesCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FMeshCommandRegistration::RegisteredCommands;

void FMeshCommandRegistration::RegisterAllCommands()
{
	UE_LOG(LogTemp, Log, TEXT("Registering Mesh commands..."));

	RegisterAndTrackCommand(MakeShared<FGetStaticMeshMetadataCommand>());
	RegisterAndTrackCommand(MakeShared<FSetLodCountCommand>());
	RegisterAndTrackCommand(MakeShared<FImportLodCommand>());
	RegisterAndTrackCommand(MakeShared<FSetLodScreenSizesCommand>());
	RegisterAndTrackCommand(MakeShared<FAutoGenerateLodsCommand>());
	RegisterAndTrackCommand(MakeShared<FSetMeshPropertiesCommand>());

	UE_LOG(LogTemp, Log, TEXT("Registered %d Mesh commands"), RegisteredCommands.Num());
}

void FMeshCommandRegistration::UnregisterAllCommands()
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
}

void FMeshCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
	if (!Command.IsValid()) return;

	FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
	if (Registry.RegisterCommand(Command))
	{
		RegisteredCommands.Add(Command);
		UE_LOG(LogTemp, Log, TEXT("Registered Mesh command: %s"), *Command->GetCommandName());
	}
}
