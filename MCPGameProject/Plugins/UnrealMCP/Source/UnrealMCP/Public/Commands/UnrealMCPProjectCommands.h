#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class FUnrealMCPCommandRegistry;
class IProjectService;

/**
 * Handler class for Project-wide MCP commands
 * Refactored to use the new service layer architecture and command pattern
 */
class UNREALMCP_API FUnrealMCPProjectCommands
{
public:
    FUnrealMCPProjectCommands();
    virtual ~FUnrealMCPProjectCommands();

    // Handle project commands using the new architecture
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Service layer
    TSharedPtr<IProjectService> ProjectService;
    
    // Initialize the service and command registry
    void InitializeServices();
    void RegisterCommands();
    
    // Legacy command handlers (for backward compatibility during transition)
    TSharedPtr<FJsonObject> HandleLegacyCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleUpdateStruct(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleShowStructVariables(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListFolderContents(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateEnhancedInputAction(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddMappingToContext(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListInputActions(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListInputMappingContexts(const TSharedPtr<FJsonObject>& Params);
};
