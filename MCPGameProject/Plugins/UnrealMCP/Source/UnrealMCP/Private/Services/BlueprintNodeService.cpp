#include "Services/BlueprintNodeService.h"
#include "Services/BlueprintNodeCreationService.h"
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
    if (!Blueprint)
    {
        return false;
    }
    
    OutResults.Empty();
    OutResults.Reserve(Connections.Num());
    
    bool bAllSucceeded = true;
    
    // Find the target graph dynamically
    UEdGraph* SearchGraph = nullptr;
    
    // Try to find the specific graph by name in UbergraphPages
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetFName() == FName(*TargetGraph))
        {
            SearchGraph = Graph;
            break;
        }
    }
    
    // Also check function graphs
    if (!SearchGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetFName() == FName(*TargetGraph))
            {
                SearchGraph = Graph;
                break;
            }
        }
    }
    
    // Fallback to EventGraph if not found and TargetGraph is default
    if (!SearchGraph && TargetGraph == TEXT("EventGraph"))
    {
        SearchGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }
    
    if (!SearchGraph)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target graph '%s' not found in Blueprint '%s'"), *TargetGraph, *Blueprint->GetName());
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
        
        // Find the nodes by GUID or special identifiers for Entry/Exit nodes
        UEdGraphNode* SourceNode = FindNodeByIdOrType(SearchGraph, Connection.SourceNodeId);
        UEdGraphNode* TargetNode = FindNodeByIdOrType(SearchGraph, Connection.TargetNodeId);
        
        UE_LOG(LogTemp, Warning, TEXT("Connecting nodes: SourceID='%s' -> SourceNode=%s, TargetID='%s' -> TargetNode=%s"), 
            *Connection.SourceNodeId, 
            SourceNode ? *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"),
            *Connection.TargetNodeId,
            TargetNode ? *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"));
        
        if (!SourceNode || !TargetNode)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find nodes for connection: Source=%s, Target=%s"), 
                SourceNode ? TEXT("Found") : TEXT("NOT FOUND"),
                TargetNode ? TEXT("Found") : TEXT("NOT FOUND"));
            OutResults.Add(false);
            bAllSucceeded = false;
            continue;
        }
        
        // Try to connect the nodes with automatic cast node creation if needed
        UE_LOG(LogTemp, Warning, TEXT("Attempting connection: '%s'.'%s' -> '%s'.'%s'"), 
            *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
            *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);
            
        bool bConnectionSucceeded = ConnectNodesWithAutoCast(SearchGraph, SourceNode, Connection.SourcePin, TargetNode, Connection.TargetPin);
        OutResults.Add(bConnectionSucceeded);
        
        if (!bConnectionSucceeded)
        {
            UE_LOG(LogTemp, Error, TEXT("FINAL RESULT: Connection failed for '%s'.'%s' -> '%s'.'%s'"), 
                *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
                *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);
            bAllSucceeded = false;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FINAL RESULT: Connection succeeded for '%s'.'%s' -> '%s'.'%s'"), 
                *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
                *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);
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

bool FBlueprintNodeService::FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FBlueprintNodeInfo>& OutNodeInfos)
{
    if (!Blueprint)
    {
        return false;
    }
    
    OutNodeInfos.Empty();
    
    // If no specific filters, return all nodes from appropriate graphs
    if (NodeType.IsEmpty() && EventType.IsEmpty())
    {
        // If target graph is specified, search only in that graph
        if (!TargetGraph.IsEmpty())
        {
            UEdGraph* SearchGraph = nullptr;
            
            // Try to find the specific graph by name
            for (UEdGraph* Graph : Blueprint->UbergraphPages)
            {
                if (Graph && Graph->GetFName() == FName(*TargetGraph))
                {
                    SearchGraph = Graph;
                    break;
                }
            }
            
            // Also check function graphs
            for (UEdGraph* Graph : Blueprint->FunctionGraphs)
            {
                if (Graph && Graph->GetFName() == FName(*TargetGraph))
                {
                    SearchGraph = Graph;
                    break;
                }
            }
            
            if (!SearchGraph)
            {
                // Target graph not found
                return false;
            }
            
            for (UEdGraphNode* Node : SearchGraph->Nodes)
            {
                if (Node)
                {
                    FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                    
                    // Special handling for TypePromotion nodes to show correct titles
                    NodeTitle = GetCleanTypePromotionTitle(Node, NodeTitle);
                    
                    OutNodeInfos.Add(FBlueprintNodeInfo(Node->NodeGuid.ToString(), NodeTitle));
                }
            }
        }
        else
        {
            // No target graph specified, search in ALL graphs like UE does
            TArray<UEdGraph*> AllGraphs;
            Blueprint->GetAllGraphs(AllGraphs);
            for (UEdGraph* Graph : AllGraphs)
            {
                if (Graph)
                {
                    for (UEdGraphNode* Node : Graph->Nodes)
                    {
                        if (Node)
                        {
                            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                            
                            // Special handling for TypePromotion nodes to show correct titles
                            NodeTitle = GetCleanTypePromotionTitle(Node, NodeTitle);
                            
                            OutNodeInfos.Add(FBlueprintNodeInfo(Node->NodeGuid.ToString(), NodeTitle));
                        }
                    }
                }
            }
        }
        return true;
    }

    // For filtered searches, determine which graph to search in
    UEdGraph* SearchGraph = nullptr;
    
    if (!TargetGraph.IsEmpty())
    {
        // Try to find the specific graph by name
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph && Graph->GetFName() == FName(*TargetGraph))
            {
                SearchGraph = Graph;
                break;
            }
        }
        
        // Also check function graphs
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetFName() == FName(*TargetGraph))
            {
                SearchGraph = Graph;
                break;
            }
        }
        
        if (!SearchGraph)
        {
            // Target graph not found
            return false;
        }
    }
    else
    {
        // Default to event graph if no specific graph is requested
        SearchGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }
    
    if (!SearchGraph)
    {
        return false;
    }
    
    // Filter nodes by the exact requested type
    if (NodeType == TEXT("Event"))
    {
        // Look for event nodes
        for (UEdGraphNode* Node : SearchGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode)
            {
                // If specific event type is requested, filter by it
                if (!EventType.IsEmpty())
                {
                    if (EventNode->EventReference.GetMemberName() == FName(*EventType))
                    {
                        FString NodeTitle = EventNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                        OutNodeInfos.Add(FBlueprintNodeInfo(EventNode->NodeGuid.ToString(), NodeTitle));
                    }
                }
                else
                {
                    // Add all event nodes
                    FString NodeTitle = EventNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                    OutNodeInfos.Add(FBlueprintNodeInfo(EventNode->NodeGuid.ToString(), NodeTitle));
                }
            }
        }
    }
    else if (NodeType == TEXT("Function"))
    {
        // Look for function call nodes
        for (UEdGraphNode* Node : SearchGraph->Nodes)
        {
            UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node);
            if (FunctionNode)
            {
                FString NodeTitle = FunctionNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                OutNodeInfos.Add(FBlueprintNodeInfo(FunctionNode->NodeGuid.ToString(), NodeTitle));
            }
        }
    }
    else if (NodeType == TEXT("Variable"))
    {
        // Look for variable nodes
        for (UEdGraphNode* Node : SearchGraph->Nodes)
        {
            UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node);
            UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node);
            if (VarGetNode || VarSetNode)
            {
                FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                OutNodeInfos.Add(FBlueprintNodeInfo(Node->NodeGuid.ToString(), NodeTitle));
            }
        }
    }
    else
    {
        // Generic search by class name
        for (UEdGraphNode* Node : SearchGraph->Nodes)
        {
            if (Node)
            {
                FString NodeClassName = Node->GetClass()->GetName();
                if (NodeClassName.Contains(NodeType))
                {
                    FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                    OutNodeInfos.Add(FBlueprintNodeInfo(Node->NodeGuid.ToString(), NodeTitle));
                }
            }
        }
    }
    
    return true;
}

bool FBlueprintNodeService::GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames)
{
    if (!Blueprint)
    {
        return false;
    }
    
    OutGraphNames.Empty();
    
    // Add all Ubergraph pages (includes EventGraph and other main graphs)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph)
        {
            OutGraphNames.Add(Graph->GetFName().ToString());
        }
    }
    
    // Add all function graphs
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph)
        {
            OutGraphNames.Add(Graph->GetFName().ToString());
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

bool FBlueprintNodeService::ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    UE_LOG(LogTemp, Warning, TEXT("=== ConnectNodesWithAutoCast START ==="));
    UE_LOG(LogTemp, Warning, TEXT("Connecting: '%s'.%s -> '%s'.%s"), 
        SourceNode ? *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"), 
        *SourcePinName,
        TargetNode ? *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"), 
        *TargetPinName);
    
    if (!Graph || !SourceNode || !TargetNode)
    {
        UE_LOG(LogTemp, Error, TEXT("NULL parameters"));
        return false;
    }
    
    // Find the pins
    UEdGraphPin* SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (!SourcePin || !TargetPin)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find pins - Source '%s': %s, Target '%s': %s"), 
               *SourcePinName, SourcePin ? TEXT("FOUND") : TEXT("NOT FOUND"),
               *TargetPinName, TargetPin ? TEXT("FOUND") : TEXT("NOT FOUND"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Pin types - Source: %s, Target: %s"),
        *SourcePin->PinType.PinCategory.ToString(),
        *TargetPin->PinType.PinCategory.ToString());
    
    // Just try to connect - let Unreal handle all the validation and type conversion
    UE_LOG(LogTemp, Warning, TEXT("Attempting connection..."));
    SourcePin->MakeLinkTo(TargetPin);
    
    // Check if connection was made
    bool bConnectionExists = SourcePin->LinkedTo.Contains(TargetPin);
    UE_LOG(LogTemp, Warning, TEXT("Connection result: %s"), bConnectionExists ? TEXT("SUCCESS") : TEXT("FAILED"));
    
    if (bConnectionExists)
    {
        // Force wildcard pin type resolution for mathematical operators
        if (UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(SourceNode))
        {
            if (CallFuncNode->GetClass()->GetName().Contains(TEXT("PromotableOperator")))
            {
                // Reconstruct the node to lock in wildcard types
                CallFuncNode->ReconstructNode();
                UE_LOG(LogTemp, Log, TEXT("Reconstructed PromotableOperator node to lock wildcard types"));
            }
        }
        
        if (UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(TargetNode))
        {
            if (CallFuncNode->GetClass()->GetName().Contains(TEXT("PromotableOperator")))
            {
                // Reconstruct the node to lock in wildcard types
                CallFuncNode->ReconstructNode();
                UE_LOG(LogTemp, Log, TEXT("Reconstructed PromotableOperator target node to lock wildcard types"));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Connection rejected by Unreal Engine - incompatible pin types or other validation failed"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== ConnectNodesWithAutoCast END ==="));
    return bConnectionExists;
}

bool FBlueprintNodeService::ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType) const
{
    // Execution pins are always compatible with execution pins
    if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Exec && TargetPinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        return true;
    }
    
    // Exact type match
    if (SourcePinType.PinCategory == TargetPinType.PinCategory)
    {
        // For basic types, category match is sufficient
        if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Int ||
            SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
            SourcePinType.PinCategory == UEdGraphSchema_K2::PC_String ||
            SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
        {
            return true;
        }
        
        // For object types, check subcategory
        if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Object)
        {
            return SourcePinType.PinSubCategoryObject == TargetPinType.PinSubCategoryObject;
        }
        
        // For struct types, check subcategory
        if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
        {
            return SourcePinType.PinSubCategoryObject == TargetPinType.PinSubCategoryObject;
        }
    }
    
    // Some implicit conversions that don't need cast nodes
    // Int to Float
    if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Int && TargetPinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        return true;
    }
    
    return false;
}

bool FBlueprintNodeService::CreateCastNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    if (!Graph || !SourcePin || !TargetPin)
    {
        return false;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return false;
    }
    
    // Handle common cast scenarios
    
    // Integer to String cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int && 
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return CreateIntToStringCast(Graph, SourcePin, TargetPin);
    }
    
    // Float to String cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real && 
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return CreateFloatToStringCast(Graph, SourcePin, TargetPin);
    }
    
    // Boolean to String cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean && 
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return CreateBoolToStringCast(Graph, SourcePin, TargetPin);
    }
    
    // String to Int cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String && 
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
    {
        return CreateStringToIntCast(Graph, SourcePin, TargetPin);
    }
    
    // String to Float cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String && 
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        return CreateStringToFloatCast(Graph, SourcePin, TargetPin);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CreateCastNode: No cast implementation for %s to %s"), 
           *SourcePin->PinType.PinCategory.ToString(), 
           *TargetPin->PinType.PinCategory.ToString());
    
    return false;
}

bool FBlueprintNodeService::CreateIntToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_IntToString function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateIntToStringCast: Could not find KismetStringLibrary"));
        return false;
    }
    
    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_IntToString"));
    if (!ConvFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateIntToStringCast: Could not find Conv_IntToString function"));
        return false;
    }
    
    // Create the conversion node
    UK2Node_CallFunction* ConvNode = NewObject<UK2Node_CallFunction>(Graph);
    ConvNode->SetFromFunction(ConvFunction);
    
    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;
    
    ConvNode->NodePosX = CastPos.X;
    ConvNode->NodePosY = CastPos.Y;
    
    Graph->AddNode(ConvNode, true);
    ConvNode->PostPlacedNewNode();
    ConvNode->AllocateDefaultPins();
    
    // Find the input and output pins on the conversion node
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InInt"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);
    
    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);
    
    UE_LOG(LogTemp, Display, TEXT("CreateIntToStringCast: Successfully created Int to String cast node"));
    return true;
}

bool FBlueprintNodeService::CreateFloatToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_FloatToString function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }
    
    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_FloatToString"));
    if (!ConvFunction)
    {
        return false;
    }
    
    // Create the conversion node
    UK2Node_CallFunction* ConvNode = NewObject<UK2Node_CallFunction>(Graph);
    ConvNode->SetFromFunction(ConvFunction);
    
    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;
    
    ConvNode->NodePosX = CastPos.X;
    ConvNode->NodePosY = CastPos.Y;
    
    Graph->AddNode(ConvNode, true);
    ConvNode->PostPlacedNewNode();
    ConvNode->AllocateDefaultPins();
    
    // Find the input and output pins on the conversion node
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InFloat"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);
    
    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);
    
    UE_LOG(LogTemp, Display, TEXT("CreateFloatToStringCast: Successfully created Float to String cast node"));
    return true;
}

bool FBlueprintNodeService::CreateBoolToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_BoolToString function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }
    
    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_BoolToString"));
    if (!ConvFunction)
    {
        return false;
    }
    
    // Create the conversion node
    UK2Node_CallFunction* ConvNode = NewObject<UK2Node_CallFunction>(Graph);
    ConvNode->SetFromFunction(ConvFunction);
    
    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;
    
    ConvNode->NodePosX = CastPos.X;
    ConvNode->NodePosY = CastPos.Y;
    
    Graph->AddNode(ConvNode, true);
    ConvNode->PostPlacedNewNode();
    ConvNode->AllocateDefaultPins();
    
    // Find the input and output pins on the conversion node
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InBool"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);
    
    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);
    
    UE_LOG(LogTemp, Display, TEXT("CreateBoolToStringCast: Successfully created Bool to String cast node"));
    return true;
}

bool FBlueprintNodeService::CreateStringToIntCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_StringToInt function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }
    
    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_StringToInt"));
    if (!ConvFunction)
    {
        return false;
    }
    
    // Create the conversion node
    UK2Node_CallFunction* ConvNode = NewObject<UK2Node_CallFunction>(Graph);
    ConvNode->SetFromFunction(ConvFunction);
    
    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;
    
    ConvNode->NodePosX = CastPos.X;
    ConvNode->NodePosY = CastPos.Y;
    
    Graph->AddNode(ConvNode, true);
    ConvNode->PostPlacedNewNode();
    ConvNode->AllocateDefaultPins();
    
    // Find the input and output pins on the conversion node
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InString"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);
    
    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);
    
    UE_LOG(LogTemp, Display, TEXT("CreateStringToIntCast: Successfully created String to Int cast node"));
    return true;
}

bool FBlueprintNodeService::CreateStringToFloatCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_StringToFloat function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }
    
    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_StringToFloat"));
    if (!ConvFunction)
    {
        return false;
    }
    
    // Create the conversion node
    UK2Node_CallFunction* ConvNode = NewObject<UK2Node_CallFunction>(Graph);
    ConvNode->SetFromFunction(ConvFunction);
    
    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;
    
    ConvNode->NodePosX = CastPos.X;
    ConvNode->NodePosY = CastPos.Y;
    
    Graph->AddNode(ConvNode, true);
    ConvNode->PostPlacedNewNode();
    ConvNode->AllocateDefaultPins();
    
    // Find the input and output pins on the conversion node
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InString"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);
    
    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);
    
    UE_LOG(LogTemp, Display, TEXT("CreateStringToFloatCast: Successfully created String to Float cast node"));
    return true;
}

UEdGraphNode* FBlueprintNodeService::FindNodeByIdOrType(UEdGraph* Graph, const FString& NodeIdOrType) const
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FindNodeByIdOrType: Looking for '%s' in graph '%s' with %d nodes"), 
        *NodeIdOrType, *Graph->GetFName().ToString(), Graph->Nodes.Num());
    
    // First try to find by exact GUID
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && Node->NodeGuid.ToString() == NodeIdOrType)
        {
            return Node;
        }
    }
    
    // If not found by GUID, try to find by node title (for Entry/Exit nodes)
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            
            // Handle special cases for Entry/Exit nodes
            if (NodeIdOrType == TEXT("FunctionEntry") || NodeIdOrType == TEXT("CanInteract"))
            {
                // Look for function entry nodes
                if (NodeTitle.Contains(TEXT("CanInteract")) && !NodeTitle.Contains(TEXT("Return")))
                {
                    return Node;
                }
            }
            else if (NodeIdOrType == TEXT("FunctionResult") || NodeIdOrType == TEXT("Return Node"))
            {
                // Look for function result nodes
                if (NodeTitle.Contains(TEXT("Return")) && NodeTitle.Contains(TEXT("Node")))
                {
                    return Node;
                }
            }
            else if (NodeTitle == NodeIdOrType)
            {
                // Direct title match
                return Node;
            }
        }
    }
    
    return nullptr;
}

FString FBlueprintNodeService::GetCleanTypePromotionTitle(UEdGraphNode* Node, const FString& OriginalTitle) const
{
    if (!Node || Node->GetClass()->GetName() != TEXT("K2Node_PromotableOperator"))
    {
        return OriginalTitle;
    }
    
    FString CleanTitle = OriginalTitle;
    
    // Check if this is a Timespan operation that should be displayed as a generic operation
    if (OriginalTitle.Contains(TEXT("Timespan")))
    {
        // Map specific operations to user-friendly names
        if (OriginalTitle.Contains(TEXT("<=")))
        {
            CleanTitle = TEXT("Less Equal ( <= )");
        }
        else if (OriginalTitle.Contains(TEXT("<")))
        {
            CleanTitle = TEXT("Less ( < )");
        }
        else if (OriginalTitle.Contains(TEXT(">=")))
        {
            CleanTitle = TEXT("Greater Equal ( >= )");
        }
        else if (OriginalTitle.Contains(TEXT(">")))
        {
            CleanTitle = TEXT("Greater ( > )");
        }
        else if (OriginalTitle.Contains(TEXT("==")))
        {
            CleanTitle = TEXT("Equal ( == )");
        }
        else if (OriginalTitle.Contains(TEXT("!=")))
        {
            CleanTitle = TEXT("Not Equal ( != )");
        }
        else if (OriginalTitle.Contains(TEXT("+")))
        {
            CleanTitle = TEXT("Add ( + )");
        }
        else if (OriginalTitle.Contains(TEXT("-")))
        {
            CleanTitle = TEXT("Subtract ( - )");
        }
        else if (OriginalTitle.Contains(TEXT("*")))
        {
            CleanTitle = TEXT("Multiply ( * )");
        }
        else if (OriginalTitle.Contains(TEXT("/")))
        {
            CleanTitle = TEXT("Divide ( / )");
        }
    }
    
    return CleanTitle;
}
