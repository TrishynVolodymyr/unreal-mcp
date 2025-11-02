#include "Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.h"
#include "Services/BlueprintAction/BlueprintPinSearchService.h"
#include "Services/BlueprintAction/BlueprintClassSearchService.h"
#include "Services/BlueprintAction/BlueprintNodePinInfoService.h"
#include "Services/BlueprintAction/BlueprintActionSearchService.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/Engine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

// Native Blueprint Action Menu includes
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionMenuItem.h"
#include "EdGraphSchema_K2_Actions.h"

// Blueprint Action Database includes
#include "K2Node.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintTypePromotion.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_MapForEach.h"
#include "K2Node_SetForEach.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"

// Enhanced Input includes
#include "InputAction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"

// Utility to convert CamelCase function names to Title Case (e.g., "GetActorLocation" -> "Get Actor Location")
static FString ConvertCamelCaseToTitleCase(const FString& InFunctionName)
{
    
}

// Phase 2 includes removed - using universal dynamic creation via Blueprint Action Database
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet/KismetMathLibrary.h"

// For pin type analysis
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"

// Additional includes for node creation
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/BlueprintNode/UnrealMCPNodeCreators.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

FString UUnrealMCPBlueprintActionCommands::GetActionsForPin(const FString& PinType, const FString& PinSubCategory, const FString& SearchFilter, int32 MaxResults)
{
    FBlueprintPinSearchService Service;
    return Service.GetActionsForPin(PinType, PinSubCategory, SearchFilter, MaxResults);
}

FString UUnrealMCPBlueprintActionCommands::GetActionsForClass(const FString& ClassName, const FString& SearchFilter, int32 MaxResults)
{
    FBlueprintClassSearchService Service;
    return Service.GetActionsForClass(ClassName, SearchFilter, MaxResults);
}

FString UUnrealMCPBlueprintActionCommands::GetActionsForClassHierarchy(const FString& ClassName, const FString& SearchFilter, int32 MaxResults)
{
    FBlueprintClassSearchService Service;
    return Service.GetActionsForClassHierarchy(ClassName, SearchFilter, MaxResults);
}

FString UUnrealMCPBlueprintActionCommands::GetNodePinInfo(const FString& NodeName, const FString& PinName)
{
    FBlueprintNodePinInfoService Service;
    return Service.GetNodePinInfo(NodeName, PinName);
}

FString UUnrealMCPBlueprintActionCommands::SearchBlueprintActions(const FString& SearchQuery, const FString& Category, int32 MaxResults, const FString& BlueprintName)
{
    FBlueprintActionSearchService Service;
    return Service.SearchBlueprintActions(SearchQuery, Category, MaxResults, BlueprintName);
}

FString UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    // Delegate to the extracted function in UnrealMCPNodeCreators
    return UnrealMCPNodeCreators::CreateNodeByActionName(BlueprintName, FunctionName, ClassName, NodePosition, JsonParams);
}



