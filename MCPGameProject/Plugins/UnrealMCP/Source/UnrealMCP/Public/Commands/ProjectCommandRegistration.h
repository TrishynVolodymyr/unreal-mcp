#pragma once

#include "CoreMinimal.h"

class FUnrealMCPCommandRegistry;
class IProjectService;

/**
 * Registration utility for Project commands
 */
class UNREALMCP_API FProjectCommandRegistration
{
public:
    /**
     * Register all project commands with the command registry
     * @param Registry The command registry to register commands with
     * @param ProjectService The project service instance to use
     */
    static void RegisterCommands(FUnrealMCPCommandRegistry& Registry, TSharedPtr<IProjectService> ProjectService);
};
