#pragma once

#include "CoreMinimal.h"
#include "Services/IBlueprintActionService.h"

/**
 * Implementation of IBlueprintActionService
 * Handles Blueprint action discovery and node creation
 */
class UNREALMCP_API FBlueprintActionService : public IBlueprintActionService
{
public:
    FBlueprintActionService() = default;
    virtual ~FBlueprintActionService() = default;

    // IBlueprintActionService interface
    virtual FString GetActionsForPin(const FString& PinType, const FString& PinSubCategory, const FString& SearchFilter, int32 MaxResults) override;
    virtual FString GetActionsForClass(const FString& ClassName, const FString& SearchFilter, int32 MaxResults) override;
    virtual FString GetActionsForClassHierarchy(const FString& ClassName, const FString& SearchFilter, int32 MaxResults) override;
    virtual FString SearchBlueprintActions(const FString& SearchQuery, const FString& Category, int32 MaxResults, const FString& BlueprintName) override;
    virtual FString GetNodePinInfo(const FString& NodeName, const FString& PinName, const FString& ClassName = TEXT("")) override;
    virtual FString CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams, const FString& TargetGraph = TEXT("EventGraph")) override;
};