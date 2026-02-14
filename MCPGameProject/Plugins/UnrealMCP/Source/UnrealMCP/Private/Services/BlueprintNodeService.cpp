#include "Services/BlueprintNodeService.h"
#include "Services/BlueprintNodeCreationService.h"
#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Services/BlueprintNode/BlueprintNodeQueryService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_InputAction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_PromotableOperator.h"
#include "KismetCompiler.h"

bool FBlueprintNodeConnectionParams::IsValid(FString& OutError) const
{
    if (SourceNodeId.IsEmpty())
    {
        OutError = TEXT("Source node ID is required");
        return false;
    }
    
    if (SourcePin.IsEmpty())
    {
        OutError = TEXT("Source pin name is required");
        return false;
    }
    
    if (TargetNodeId.IsEmpty())
    {
        OutError = TEXT("Target node ID is required");
        return false;
    }
    
    if (TargetPin.IsEmpty())
    {
        OutError = TEXT("Target pin name is required");
        return false;
    }
    
    return true;
}

bool FBlueprintNodeCreationParams::IsValid(FString& OutError) const
{
    if (BlueprintName.IsEmpty())
    {
        OutError = TEXT("Blueprint name is required");
        return false;
    }
    
    return true;
}

FBlueprintNodeService& FBlueprintNodeService::Get()
{
    static FBlueprintNodeService Instance;
    return Instance;
}

bool FBlueprintNodeService::ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& TargetGraph, TArray<bool>& OutResults)
{
    return FBlueprintNodeConnectionService::Get().ConnectBlueprintNodes(Blueprint, Connections, TargetGraph, OutResults);
}

bool FBlueprintNodeService::ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    return FBlueprintNodeConnectionService::Get().ConnectNodesWithAutoCast(Graph, SourceNode, SourcePinName, TargetNode, TargetPinName);
}

bool FBlueprintNodeService::AddInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId)
{
    if (!Blueprint || ActionName.IsEmpty())
    {
        return false;
    }
    
    // Use the existing BlueprintNodeCreationService as a dependency
    FBlueprintNodeCreationService CreationService;
    
    FString PositionStr = FString::Printf(TEXT("[%f, %f]"), Position.X, Position.Y);
    FString JsonParams = FString::Printf(TEXT("{\"action_name\": \"%s\"}"), *ActionName);
    
    // Call the creation service with the correct parameters for input action nodes
    // The creation service expects specific function names that it recognizes
    FString Result = CreationService.CreateNodeByActionName(Blueprint->GetName(), ActionName, TEXT(""), PositionStr, JsonParams);
    
    // Parse the result to extract node ID
    TSharedPtr<FJsonObject> ResultObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result);
    
    if (FJsonSerializer::Deserialize(Reader, ResultObj) && ResultObj.IsValid())
    {
        bool bSuccess = false;
        if (ResultObj->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
        {
            ResultObj->TryGetStringField(TEXT("node_id"), OutNodeId);
            return true;
        }
    }
    
    return false;
}

bool FBlueprintNodeService::FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FBlueprintNodeInfo>& OutNodeInfos)
{
    return FBlueprintNodeQueryService::Get().FindBlueprintNodes(Blueprint, NodeType, EventType, TargetGraph, OutNodeInfos);
}

bool FBlueprintNodeService::GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames)
{
    return FBlueprintNodeQueryService::Get().GetBlueprintGraphs(Blueprint, OutGraphNames);
}

bool FBlueprintNodeService::AddVariableNode(UBlueprint* Blueprint, const FString& VariableName, bool bIsGetter, const FVector2D& Position, FString& OutNodeId)
{
    if (!Blueprint || VariableName.IsEmpty())
    {
        return false;
    }
    
    // Use the existing BlueprintNodeCreationService as a dependency
    FBlueprintNodeCreationService CreationService;
    
    FString PositionStr = FString::Printf(TEXT("[%f, %f]"), Position.X, Position.Y);
    FString FunctionName = bIsGetter ? TEXT("Get") : TEXT("Set");
    FString JsonParams = FString::Printf(TEXT("{\"variable_name\": \"%s\"}"), *VariableName);
    
    // Call the creation service with the correct parameters for variable nodes
    FString Result = CreationService.CreateNodeByActionName(Blueprint->GetName(), FunctionName, TEXT(""), PositionStr, JsonParams);
    
    // Parse the result to extract node ID
    TSharedPtr<FJsonObject> ResultObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result);
    
    if (FJsonSerializer::Deserialize(Reader, ResultObj) && ResultObj.IsValid())
    {
        bool bSuccess = false;
        if (ResultObj->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
        {
            ResultObj->TryGetStringField(TEXT("node_id"), OutNodeId);
            return true;
        }
    }
    
    return false;
}

bool FBlueprintNodeService::GetVariableInfo(UBlueprint* Blueprint, const FString& VariableName, FString& OutVariableType, TSharedPtr<FJsonObject>& OutAdditionalInfo)
{
    return FBlueprintNodeQueryService::Get().GetVariableInfo(Blueprint, VariableName, OutVariableType, OutAdditionalInfo);
}

bool FBlueprintNodeService::AddEventNode(UBlueprint* Blueprint, const FString& EventType, const FVector2D& Position, FString& OutNodeId)
{
    if (!Blueprint || EventType.IsEmpty())
    {
        return false;
    }
    
    // Use the existing BlueprintNodeCreationService as a dependency
    FBlueprintNodeCreationService CreationService;
    
    FString PositionStr = FString::Printf(TEXT("[%f, %f]"), Position.X, Position.Y);
    FString JsonParams = FString::Printf(TEXT("{\"event_type\": \"%s\"}"), *EventType);
    
    // Call the creation service with the event type as the function name
    FString Result = CreationService.CreateNodeByActionName(Blueprint->GetName(), EventType, TEXT(""), PositionStr, JsonParams);
    
    // Parse the result to extract node ID
    TSharedPtr<FJsonObject> ResultObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result);
    
    if (FJsonSerializer::Deserialize(Reader, ResultObj) && ResultObj.IsValid())
    {
        bool bSuccess = false;
        if (ResultObj->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
        {
            ResultObj->TryGetStringField(TEXT("node_id"), OutNodeId);
            return true;
        }
    }
    
    return false;
}

bool FBlueprintNodeService::AddFunctionCallNode(UBlueprint* Blueprint, const FString& FunctionName, const FString& ClassName, const FVector2D& Position, FString& OutNodeId)
{
    if (!Blueprint || FunctionName.IsEmpty())
    {
        return false;
    }
    
    // Use the existing BlueprintNodeCreationService as a dependency
    FBlueprintNodeCreationService CreationService;
    
    FString PositionStr = FString::Printf(TEXT("[%f, %f]"), Position.X, Position.Y);
    FString JsonParams = FString::Printf(TEXT("{\"function_name\": \"%s\"}"), *FunctionName);
    
    // Call the creation service with the function name and class name
    FString Result = CreationService.CreateNodeByActionName(Blueprint->GetName(), FunctionName, ClassName, PositionStr, JsonParams);
    
    // Parse the result to extract node ID
    TSharedPtr<FJsonObject> ResultObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result);
    
    if (FJsonSerializer::Deserialize(Reader, ResultObj) && ResultObj.IsValid())
    {
        bool bSuccess = false;
        if (ResultObj->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
        {
            ResultObj->TryGetStringField(TEXT("node_id"), OutNodeId);
            return true;
        }
    }
    
    return false;
}

bool FBlueprintNodeService::AddCustomEventNode(UBlueprint* Blueprint, const FString& EventName, const FVector2D& Position, FString& OutNodeId)
{
    if (!Blueprint || EventName.IsEmpty())
    {
        return false;
    }
    
    // GUARD: If EventName is a standard override event, delegate to AddEventNode instead.
    // This prevents creating a useless CustomEvent named "ReceiveTick" when the user meant the built-in event.
    static const TArray<FString> StandardOverrideEvents = {
        TEXT("ReceiveTick"), TEXT("Tick"),
        TEXT("ReceiveBeginPlay"), TEXT("BeginPlay"),
        TEXT("ReceiveEndPlay"), TEXT("EndPlay"),
        TEXT("ReceiveActorBeginOverlap"), TEXT("ActorBeginOverlap"),
        TEXT("ReceiveActorEndOverlap"), TEXT("ActorEndOverlap"),
        TEXT("ReceiveHit"), TEXT("Hit"),
        TEXT("ReceiveDestroyed"), TEXT("Destroyed"),
        TEXT("ReceiveBeginDestroy"), TEXT("BeginDestroy")
    };

    for (const FString& OverrideEvent : StandardOverrideEvents)
    {
        if (EventName.Equals(OverrideEvent, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Warning, TEXT("AddCustomEventNode: '%s' is a standard override event — redirecting to AddEventNode"), *EventName);
            return AddEventNode(Blueprint, EventName, Position, OutNodeId);
        }
    }
    
    // Use the existing BlueprintNodeCreationService as a dependency
    FBlueprintNodeCreationService CreationService;
    
    FString PositionStr = FString::Printf(TEXT("[%f, %f]"), Position.X, Position.Y);
    FString JsonParams = FString::Printf(TEXT("{\"event_name\": \"%s\"}"), *EventName);
    
    // Call the creation service with CustomEvent as the function name
    FString Result = CreationService.CreateNodeByActionName(Blueprint->GetName(), TEXT("CustomEvent"), TEXT(""), PositionStr, JsonParams);
    
    // Parse the result to extract node ID
    TSharedPtr<FJsonObject> ResultObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result);
    
    if (FJsonSerializer::Deserialize(Reader, ResultObj) && ResultObj.IsValid())
    {
        bool bSuccess = false;
        if (ResultObj->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
        {
            ResultObj->TryGetStringField(TEXT("node_id"), OutNodeId);
            return true;
        }
    }
    
    return false;
}

// REMOVED: Enhanced Input Action nodes now created via Blueprint Action system
// Use create_node_by_action_name with function_name="EnhancedInputAction {ActionName}" instead

