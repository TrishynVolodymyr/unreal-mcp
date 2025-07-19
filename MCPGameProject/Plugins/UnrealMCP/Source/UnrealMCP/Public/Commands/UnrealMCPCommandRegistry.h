#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registry for all MCP commands
 * Provides centralized command registration, discovery, and execution
 */
class UNREALMCP_API FUnrealMCPCommandRegistry
{
public:
    /**
     * Get the singleton instance of the command registry
     * @return Reference to the singleton instance
     */
    static FUnrealMCPCommandRegistry& Get();
    
    /**
     * Register a command with the registry
     * @param Command - Shared pointer to the command to register
     * @return true if command was registered successfully
     */
    bool RegisterCommand(TSharedPtr<IUnrealMCPCommand> Command);
    
    /**
     * Unregister a command from the registry
     * @param CommandName - Name of the command to unregister
     * @return true if command was unregistered successfully
     */
    bool UnregisterCommand(const FString& CommandName);
    
    /**
     * Execute a command by name
     * @param CommandName - Name of the command to execute
     * @param Parameters - JSON parameters for the command
     * @return JSON response from the command
     */
    FString ExecuteCommand(const FString& CommandName, const FString& Parameters);
    
    /**
     * Check if a command is registered
     * @param CommandName - Name of the command to check
     * @return true if command is registered
     */
    bool IsCommandRegistered(const FString& CommandName) const;
    
    /**
     * Get all registered command names
     * @return Array of registered command names
     */
    TArray<FString> GetRegisteredCommandNames() const;
    
    /**
     * Get command help information
     * @param CommandName - Name of the command
     * @return Help information as JSON string
     */
    FString GetCommandHelp(const FString& CommandName) const;
    
    /**
     * Get all commands help information
     * @return Help information for all commands as JSON string
     */
    FString GetAllCommandsHelp() const;
    
    /**
     * Clear all registered commands
     */
    void ClearRegistry();

private:
    /** Private constructor for singleton pattern */
    FUnrealMCPCommandRegistry() = default;
    
    /** Map of command names to command instances */
    TMap<FString, TSharedPtr<IUnrealMCPCommand>> RegisteredCommands;
    
    /** Critical section for thread safety */
    mutable FCriticalSection RegistryLock;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON error response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
    
    /**
     * Create success response JSON for registry operations
     * @param Message - Success message
     * @return JSON success response string
     */
    FString CreateSuccessResponse(const FString& Message) const;
};
