#include "Services/BlueprintNodeService.h"
#include "Services/BlueprintNodeCreationService.h"
#include "Commands/UnrealMCPCommonUtils.h"
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

bool FBlueprintNodeService::ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, TArray<bool>& OutResults)
{
    if (!Blueprint)
    {
        return false;
    }
    
    OutResults.Empty();
    OutResults.Reserve(Connections.Num());
    
    bool bAllSucceeded = true;
    
    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return false;
    }
    
    for (const FBlueprintNodeConnectionParams& Connection : Connections)
    {
        FString ValidationError;
        if (!Connection.IsValid(ValidationError))
        {
            OutResults.Add(false);
            bAllSucceeded = false;
            continue;
        }
        
        // Find the nodes by GUID
        UEdGraphNode* SourceNode = nullptr;
        UEdGraphNode* TargetNode = nullptr;
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node->NodeGuid.ToString() == Connection.SourceNodeId)
            {
                SourceNode = Node;
            }
            else if (Node->NodeGuid.ToString() == Connection.TargetNodeId)
            {
                TargetNode = Node;
            }
        }
        
        if (!SourceNode || !TargetNode)
        {
            OutResults.Add(false);
            bAllSucceeded = false;
            continue;
        }
        
        // Connect the nodes using the common utils
        bool bConnectionSucceeded = FUnrealMCPCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, Connection.SourcePin, TargetNode, Connection.TargetPin);
        OutResults.Add(bConnectionSucceeded);
        
        if (!bConnectionSucceeded)
        {
            bAllSucceeded = false;
        }
    }
    
    if (bAllSucceeded)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }
    
    return bAllSucceeded;
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

bool FBlueprintNodeService::FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FString>& OutNodeIds)
{
    if (!Blueprint)
    {
        return false;
    }
    
    OutNodeIds.Empty();
    
    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return false;
    }
    
    // If no specific filters, return all nodes
    if (NodeType.IsEmpty() && EventType.IsEmpty())
    {
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node)
            {
                OutNodeIds.Add(Node->NodeGuid.ToString());
            }
        }
        return true;
    }
    
    // Filter nodes by the exact requested type
    if (NodeType == TEXT("Event"))
    {
        // Look for event nodes
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode)
            {
                // If specific event type is requested, filter by it
                if (!EventType.IsEmpty())
                {
                    if (EventNode->EventReference.GetMemberName() == FName(*EventType))
                    {
                        OutNodeIds.Add(EventNode->NodeGuid.ToString());
                    }
                }
                else
                {
                    // Add all event nodes
                    OutNodeIds.Add(EventNode->NodeGuid.ToString());
                }
            }
        }
    }
    else if (NodeType == TEXT("Function"))
    {
        // Look for function call nodes
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node);
            if (FunctionNode)
            {
                OutNodeIds.Add(FunctionNode->NodeGuid.ToString());
            }
        }
    }
    else if (NodeType == TEXT("Variable"))
    {
        // Look for variable nodes
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node);
            UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node);
            if (VarGetNode || VarSetNode)
            {
                OutNodeIds.Add(Node->NodeGuid.ToString());
            }
        }
    }
    else
    {
        // Generic search by class name
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node)
            {
                FString NodeClassName = Node->GetClass()->GetName();
                if (NodeClassName.Contains(NodeType))
                {
                    OutNodeIds.Add(Node->NodeGuid.ToString());
                }
            }
        }
    }
    
    return true;
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
    if (!Blueprint || VariableName.IsEmpty())
    {
        return false;
    }
    
    // Find the variable in the Blueprint
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName.ToString() == VariableName)
        {
            OutVariableType = Variable.VarType.PinCategory.ToString();
            
            // Create additional info object
            OutAdditionalInfo = MakeShared<FJsonObject>();
            OutAdditionalInfo->SetStringField(TEXT("variable_name"), VariableName);
            OutAdditionalInfo->SetStringField(TEXT("variable_type"), OutVariableType);
            OutAdditionalInfo->SetBoolField(TEXT("is_array"), Variable.VarType.IsArray());
            OutAdditionalInfo->SetBoolField(TEXT("is_reference"), Variable.VarType.bIsReference);
            
            if (Variable.VarType.PinSubCategoryObject.IsValid())
            {
                OutAdditionalInfo->SetStringField(TEXT("sub_category"), Variable.VarType.PinSubCategoryObject->GetName());
            }
            
            return true;
        }
    }
    
    return false;
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

UEdGraph* FBlueprintNodeService::FindGraphInBlueprint(UBlueprint* Blueprint, const FString& GraphName) const
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // If no graph name specified, return the main event graph
    if (GraphName.IsEmpty() || GraphName == TEXT("EventGraph"))
    {
        TArray<UEdGraph*> AllGraphs;
        Blueprint->GetAllGraphs(AllGraphs);
        
        for (UEdGraph* Graph : AllGraphs)
        {
            if (Graph && Graph->GetName() == TEXT("EventGraph"))
            {
                return Graph;
            }
        }
        
        // If no EventGraph found, return the first available graph
        if (AllGraphs.Num() > 0)
        {
            return AllGraphs[0];
        }
    }
    else
    {
        // Search for the specific graph by name
        TArray<UEdGraph*> AllGraphs;
        Blueprint->GetAllGraphs(AllGraphs);
        
        for (UEdGraph* Graph : AllGraphs)
        {
            if (Graph && Graph->GetName() == GraphName)
            {
                return Graph;
            }
        }
    }
    
    return nullptr;
}

FString FBlueprintNodeService::GenerateNodeId(UEdGraphNode* Node) const
{
    if (!Node)
    {
        return TEXT("");
    }
    
    // Generate a unique ID based on the node's memory address and class name
    return FString::Printf(TEXT("%s_%p"), *Node->GetClass()->GetName(), Node);
}

UEdGraphNode* FBlueprintNodeService::FindNodeById(UBlueprint* Blueprint, const FString& NodeId) const
{
    if (!Blueprint || NodeId.IsEmpty())
    {
        return nullptr;
    }
    
    // Search through all graphs in the Blueprint
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);
    
    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }
        
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node && GenerateNodeId(Node) == NodeId)
            {
                return Node;
            }
        }
    }
    
    return nullptr;
}

bool FBlueprintNodeService::ConnectPins(UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName) const
{
    if (!SourceNode || !TargetNode)
    {
        return false;
    }
    
    // Find the source pin
    UEdGraphPin* SourcePin = nullptr;
    for (UEdGraphPin* Pin : SourceNode->Pins)
    {
        if (Pin && Pin->PinName.ToString() == SourcePinName)
        {
            SourcePin = Pin;
            break;
        }
    }
    
    // Find the target pin
    UEdGraphPin* TargetPin = nullptr;
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        if (Pin && Pin->PinName.ToString() == TargetPinName)
        {
            TargetPin = Pin;
            break;
        }
    }
    
    if (!SourcePin || !TargetPin)
    {
        return false;
    }
    
    // Make the connection
    SourcePin->MakeLinkTo(TargetPin);
    
    return true;
}