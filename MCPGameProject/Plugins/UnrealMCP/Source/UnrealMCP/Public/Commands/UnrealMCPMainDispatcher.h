#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Main command dispatcher that routes commands through the registry system
 * Provides backward compatibility with the existing JSON-based command interface
 */
class UNREALMCP_API FUnrealMCPMainDispatcher
{
public:
    /**
     * Get the singleton instance of the main dispatcher
     * @return Reference to the singleton instance
     */
    static FUnrealMCPMainDispatcher& Get();
    
    /**
     * Handle a command using the registry system
     * Provides backward compatibility with existing JSON-based interface
     * @param CommandType - Type/name of the command to execute
     * @param Params - JSON object containing command parameters
     * @return JSON object containing the command result
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);
    
    /**
     * Handle a command using the registry system with string parameters
     * @param CommandType - Type/name of the command to execute
     * @param Parameters - JSON string containing command parameters
     * @return JSON string containing the command result
     */
    FString HandleCommandString(const FString& CommandType, const FString& Parameters);
    
    /**
     * Initialize the dispatcher and register all commands
     * This should be called during module startup
     */
    void Initialize();
    
    /**
     * Shutdown the dispatcher and unregister all commands
     * This should be called during module shutdown
     */
    void Shutdown();
    
    /**
     * Get list of all available commands
     * @return JSON object containing command list and help information
     */
    TSharedPtr<FJsonObject> GetAvailableCommands();

private:
    /** Private constructor for singleton pattern */
    FUnrealMCPMainDispatcher() = default;
    
    /** Whether the dispatcher has been initialized */
    bool bIsInitialized = false;
    
    /**
     * Convert JSON object to JSON string
     * @param JsonObject - JSON object to convert
     * @return JSON string representation
     */
    FString JsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject) const;
    
    /**
     * Convert JSON string to JSON object
     * @param JsonString - JSON string to convert
     * @return JSON object representation
     */
    TSharedPtr<FJsonObject> JsonStringToObject(const FString& JsonString) const;
    
    /**
     * Create error response JSON object
     * @param ErrorMessage - Error message
     * @return JSON object containing error response
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage) const;
    
    /**
     * Register all command types with the registry
     * This includes Blueprint, Project, DataTable, Editor, and UMG commands
     */
    void RegisterAllCommands();
};
