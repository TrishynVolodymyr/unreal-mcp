#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Dom/JsonObject.h"
#include "Json.h"

/**
 * Service class for Blueprint node creation operations
 * This service handles all node creation functionality following the established services pattern
 */
class UNREALMCP_API FBlueprintNodeCreationService
{
public:
    FBlueprintNodeCreationService();

    /**
     * Create a Blueprint node by action/function name
     * @param BlueprintName - Name of the target Blueprint
     * @param FunctionName - Name of the function/action to create
     * @param ClassName - Optional class name for the function
     * @param NodePosition - Position in the graph
     * @param JsonParams - Additional parameters as JSON string
     * @return JSON string result
     */
    FString CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, 
                                  const FString& ClassName, const FString& NodePosition, 
                                  const FString& JsonParams);

private:
    // JSON and parameter handling
    bool ParseJsonParameters(const FString& JsonParams, TSharedPtr<FJsonObject>& OutParamsObject, TSharedPtr<FJsonObject>& OutResultObj);
    void ParseNodePosition(const FString& NodePosition, int32& OutPositionX, int32& OutPositionY);
    
    // Class and blueprint resolution
    UClass* FindTargetClass(const FString& ClassName);
    UBlueprint* FindBlueprintByName(const FString& BlueprintName);
    
    // Helper for logging and debugging
    void LogNodeCreationAttempt(const FString& FunctionName, const FString& BlueprintName, 
                               const FString& ClassName, int32 PositionX, int32 PositionY) const;
}; 
