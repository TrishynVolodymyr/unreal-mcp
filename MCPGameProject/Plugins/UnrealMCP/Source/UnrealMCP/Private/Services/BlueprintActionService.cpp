#include "Services/BlueprintActionService.h"
#include "Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.h"

FString FBlueprintActionService::GetActionsForPin(const FString& PinType, const FString& PinSubCategory, const FString& SearchFilter, int32 MaxResults)
{
    return UUnrealMCPBlueprintActionCommands::GetActionsForPin(PinType, PinSubCategory, SearchFilter, MaxResults);
}

FString FBlueprintActionService::GetActionsForClass(const FString& ClassName, const FString& SearchFilter, int32 MaxResults)
{
    return UUnrealMCPBlueprintActionCommands::GetActionsForClass(ClassName, SearchFilter, MaxResults);
}

FString FBlueprintActionService::GetActionsForClassHierarchy(const FString& ClassName, const FString& SearchFilter, int32 MaxResults)
{
    return UUnrealMCPBlueprintActionCommands::GetActionsForClassHierarchy(ClassName, SearchFilter, MaxResults);
}

FString FBlueprintActionService::SearchBlueprintActions(const FString& SearchQuery, const FString& Category, int32 MaxResults, const FString& BlueprintName)
{
    return UUnrealMCPBlueprintActionCommands::SearchBlueprintActions(SearchQuery, Category, MaxResults, BlueprintName);
}

FString FBlueprintActionService::GetNodePinInfo(const FString& NodeName, const FString& PinName, const FString& ClassName)
{
    return UUnrealMCPBlueprintActionCommands::GetNodePinInfo(NodeName, PinName, ClassName);
}

FString FBlueprintActionService::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    UE_LOG(LogTemp, Warning, TEXT("*** SERVICE LAYER PROOF ***"));
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintActionService::CreateNodeByActionName called!"));
    UE_LOG(LogTemp, Warning, TEXT("This proves the NEW ARCHITECTURE service layer is being used!"));
    UE_LOG(LogTemp, Warning, TEXT("Parameters: Blueprint=%s, Function=%s, Class=%s"), *BlueprintName, *FunctionName, *ClassName);
    
    FString Result = UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(BlueprintName, FunctionName, ClassName, NodePosition, JsonParams);
    
    UE_LOG(LogTemp, Warning, TEXT("Service layer call completed, returning to command layer"));
    UE_LOG(LogTemp, Warning, TEXT("*** END SERVICE LAYER PROOF ***"));
    
    return Result;
}
