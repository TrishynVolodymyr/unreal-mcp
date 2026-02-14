#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration system for PCG (Procedural Content Generation) commands
 * Handles registration and cleanup of all PCG-related commands
 */
class UNREALMCP_API FPCGCommandRegistration
{
public:
	static void RegisterAllCommands();
	static void UnregisterAllCommands();

private:
	static TArray<TSharedPtr<class IUnrealMCPCommand>> RegisteredCommands;
	static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};
