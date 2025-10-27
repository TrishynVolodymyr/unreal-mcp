#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles Blueprint action-related commands
 */
class UNREALMCP_API FUnrealMCPBlueprintActionCommandsHandler
{
public:
    /**
     * Handle a Blueprint action command
     * @param CommandType - The type of command to execute
     * @param Params - Parameters for the command
     * @return JSON object containing the result
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    /**
     * Get actions for a specific pin type
     */
    static TSharedPtr<FJsonObject> GetActionsForPin(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get actions for a specific class
     */
    static TSharedPtr<FJsonObject> GetActionsForClass(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get actions for a class hierarchy
     */
    static TSharedPtr<FJsonObject> GetActionsForClassHierarchy(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get node pin info
     */
    static TSharedPtr<FJsonObject> GetNodePinInfo(const TSharedPtr<FJsonObject>& Params);



    /**
     * Create a blueprint node by discovered action/function name
     */
    static TSharedPtr<FJsonObject> CreateNodeByActionName(const TSharedPtr<FJsonObject>& Params);
}; 
