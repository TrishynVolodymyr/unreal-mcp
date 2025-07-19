#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

/**
 * Interface for all MCP commands that can be executed by the UnrealMCP system.
 * Provides a standardized way to execute commands, validate parameters, and get command metadata.
 */
class UNREALMCP_API IUnrealMCPCommand
{
public:
    virtual ~IUnrealMCPCommand() = default;

    /**
     * Execute the command with the provided parameters
     * @param Parameters JSON string containing command parameters
     * @return JSON string containing the command result
     */
    virtual FString Execute(const FString& Parameters) = 0;

    /**
     * Get the name/identifier of this command
     * @return Command name used for registration and lookup
     */
    virtual FString GetCommandName() const = 0;

    /**
     * Validate the provided parameters before execution
     * @param Parameters JSON string containing command parameters
     * @return True if parameters are valid, false otherwise
     */
    virtual bool ValidateParams(const FString& Parameters) const = 0;
};
