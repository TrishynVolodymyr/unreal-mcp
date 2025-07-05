#include "Commands/UnrealMCPNodeCreators.h"
#include "Services/BlueprintNodeCreationService.h"

FString UnrealMCPNodeCreators::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    // Create service instance and delegate to it
    FBlueprintNodeCreationService NodeCreationService;
    return NodeCreationService.CreateNodeByActionName(BlueprintName, FunctionName, ClassName, NodePosition, JsonParams);
} 