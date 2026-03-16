#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class FMeshCommandRegistration
{
public:
	static void RegisterAllCommands();
	static void UnregisterAllCommands();

private:
	static TArray<TSharedPtr<IUnrealMCPCommand>> RegisteredCommands;
	static void RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command);
};
