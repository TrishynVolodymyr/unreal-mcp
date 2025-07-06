#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Project-wide MCP commands
 */
class UNREALMCP_API FUnrealMCPProjectCommands
{
public:
    FUnrealMCPProjectCommands();

    // Handle project commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Specific project command handlers
    TSharedPtr<FJsonObject> HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateFolder(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateStruct(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleUpdateStruct(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleShowStructVariables(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListFolderContents(const TSharedPtr<FJsonObject>& Params);
    
    // Enhanced Input specific handlers
    TSharedPtr<FJsonObject> HandleCreateEnhancedInputAction(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddMappingToContext(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListInputActions(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListInputMappingContexts(const TSharedPtr<FJsonObject>& Params);
};