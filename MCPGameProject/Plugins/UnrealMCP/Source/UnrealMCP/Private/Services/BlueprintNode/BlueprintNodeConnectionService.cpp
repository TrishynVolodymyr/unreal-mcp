#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Services/BlueprintNode/BlueprintCastNodeService.h"
#include "Services/IBlueprintNodeService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Utils/GraphUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_PromotableOperator.h"

namespace
{
// Helper function to generate safe Node IDs for connection service
static FString GetSafeNodeId(UEdGraphNode* Node, const FString& NodeTitle)
{
    if (!Node)
    {
        return TEXT("InvalidNode");
    }
    return FGraphUtils::GetReliableNodeId(Node);
}
} // anonymous namespace

FBlueprintNodeConnectionService& FBlueprintNodeConnectionService::Get()
{
    static FBlueprintNodeConnectionService Instance;
    return Instance;
}

FPinConnectionResponse FBlueprintNodeConnectionService::CanConnectPins(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    if (!SourcePin || !TargetPin)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pin(s) - one or both pins are null"));
    }

    UEdGraphNode* SourceNode = SourcePin->GetOwningNode();
    UEdGraphNode* TargetNode = TargetPin->GetOwningNode();

    if (!SourceNode || !TargetNode)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid node(s) - pin has no owning node"));
    }

    UEdGraph* Graph = SourceNode->GetGraph();
    if (!Graph)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("No graph found for source node"));
    }

    const UEdGraphSchema* Schema = Graph->GetSchema();
    if (!Schema)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("No schema found for graph"));
    }

    return Schema->CanCreateConnection(SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CanConnectPinsByName(UEdGraphNode* SourceNode, const FString& SourcePinName,
                                                           UEdGraphNode* TargetNode, const FString& TargetPinName,
                                                           FPinConnectionResponse& OutResponse)
{
    if (!SourceNode || !TargetNode)
    {
        OutResponse = FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid node(s) - one or both nodes are null"));
        return false;
    }

    // Find the source pin
    UEdGraphPin* SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
    if (!SourcePin)
    {
        for (UEdGraphPin* Pin : SourceNode->Pins)
        {
            if (Pin && Pin->PinName.ToString() == SourcePinName)
            {
                SourcePin = Pin;
                break;
            }
        }
    }

    // Find the target pin
    UEdGraphPin* TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);
    if (!TargetPin)
    {
        for (UEdGraphPin* Pin : TargetNode->Pins)
        {
            if (Pin && Pin->PinName.ToString() == TargetPinName)
            {
                TargetPin = Pin;
                break;
            }
        }
    }

    if (!SourcePin)
    {
        OutResponse = FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
            FString::Printf(TEXT("Source pin '%s' not found on node"), *SourcePinName));
        return false;
    }

    if (!TargetPin)
    {
        OutResponse = FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
            FString::Printf(TEXT("Target pin '%s' not found on node"), *TargetPinName));
        return false;
    }

    OutResponse = CanConnectPins(SourcePin, TargetPin);
    return OutResponse.Response != CONNECT_RESPONSE_DISALLOW;
}

bool FBlueprintNodeConnectionService::ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& TargetGraph, TArray<bool>& OutResults)
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

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetFName() == FName(*TargetGraph))
        {
            SearchGraph = Graph;
            break;
        }
    }

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

        UEdGraphNode* SourceNode = FindNodeByIdOrType(SearchGraph, Connection.SourceNodeId);
        UEdGraphNode* TargetNode = FindNodeByIdOrType(SearchGraph, Connection.TargetNodeId);

        if (!SourceNode || !TargetNode)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find nodes for connection: Source=%s, Target=%s"),
                SourceNode ? TEXT("Found") : TEXT("NOT FOUND"),
                TargetNode ? TEXT("Found") : TEXT("NOT FOUND"));
            OutResults.Add(false);
            bAllSucceeded = false;
            continue;
        }

        bool bConnectionSucceeded = ConnectNodesWithAutoCast(SearchGraph, SourceNode, Connection.SourcePin, TargetNode, Connection.TargetPin);
        OutResults.Add(bConnectionSucceeded);

        if (!bConnectionSucceeded)
        {
            bAllSucceeded = false;
        }
    }

    // Mark modified if ANY connection succeeded (not just all).
    // Otherwise partial successes get silently dropped on next compile.
    int32 SuccessCount = 0;
    for (bool Result : OutResults) { if (Result) SuccessCount++; }
    if (SuccessCount > 0)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    return bAllSucceeded;
}

bool FBlueprintNodeConnectionService::ConnectBlueprintNodesEnhanced(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& TargetGraph, TArray<FConnectionResultInfo>& OutResults)
{
    if (!Blueprint)
    {
        return false;
    }

    OutResults.Empty();
    OutResults.Reserve(Connections.Num());

    bool bAllSucceeded = true;

    UEdGraph* SearchGraph = nullptr;

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetFName() == FName(*TargetGraph))
        {
            SearchGraph = Graph;
            break;
        }
    }

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

    if (!SearchGraph && TargetGraph == TEXT("EventGraph"))
    {
        SearchGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!SearchGraph)
    {
        FConnectionResultInfo FailResult;
        FailResult.bSuccess = false;
        FailResult.ErrorMessage = FString::Printf(TEXT("Target graph '%s' not found"), *TargetGraph);
        OutResults.Add(FailResult);
        return false;
    }

    for (const FBlueprintNodeConnectionParams& Connection : Connections)
    {
        FConnectionResultInfo Result;
        Result.SourceNodeId = Connection.SourceNodeId;
        Result.TargetNodeId = Connection.TargetNodeId;

        FString ValidationError;
        if (!Connection.IsValid(ValidationError))
        {
            Result.bSuccess = false;
            Result.ErrorMessage = ValidationError;
            OutResults.Add(Result);
            bAllSucceeded = false;
            continue;
        }

        UEdGraphNode* SourceNode = FindNodeByIdOrType(SearchGraph, Connection.SourceNodeId);
        UEdGraphNode* TargetNode = FindNodeByIdOrType(SearchGraph, Connection.TargetNodeId);

        if (!SourceNode || !TargetNode)
        {
            Result.bSuccess = false;
            Result.ErrorMessage = FString::Printf(TEXT("Node not found: %s=%s, %s=%s"),
                *Connection.SourceNodeId, SourceNode ? TEXT("found") : TEXT("NOT FOUND"),
                *Connection.TargetNodeId, TargetNode ? TEXT("found") : TEXT("NOT FOUND"));
            OutResults.Add(Result);
            bAllSucceeded = false;
            continue;
        }

        TArray<FAutoInsertedNodeInfo> AutoInsertedNodes;
        FString ConnectionErrorMessage;
        bool bConnectionSucceeded = ConnectNodesWithAutoCast(SearchGraph, SourceNode, Connection.SourcePin,
                                                              TargetNode, Connection.TargetPin, &AutoInsertedNodes, &ConnectionErrorMessage);

        Result.bSuccess = bConnectionSucceeded;
        Result.AutoInsertedNodes = AutoInsertedNodes;

        if (!bConnectionSucceeded)
        {
            // Use detailed error message if available, otherwise fall back to generic
            if (!ConnectionErrorMessage.IsEmpty())
            {
                Result.ErrorMessage = FString::Printf(TEXT("Failed to connect '%s'.%s -> '%s'.%s: %s"),
                    *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
                    *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin,
                    *ConnectionErrorMessage);
            }
            else
            {
                Result.ErrorMessage = FString::Printf(TEXT("Failed to connect '%s'.%s -> '%s'.%s"),
                    *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
                    *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);
            }
            bAllSucceeded = false;
        }

        OutResults.Add(Result);
    }

    // Mark modified if ANY connection succeeded (not just all).
    // Otherwise partial successes get silently dropped on next compile.
    int32 EnhancedSuccessCount = 0;
    for (const FConnectionResultInfo& R : OutResults) { if (R.bSuccess) EnhancedSuccessCount++; }
    if (EnhancedSuccessCount > 0)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    return bAllSucceeded;
}

bool FBlueprintNodeConnectionService::ConnectPins(UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!SourceNode || !TargetNode)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectPins: Invalid node(s)"));
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

    // Handle Return Node pin naming variations
    if (!TargetPin && TargetNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(TEXT("Return")))
    {
        TArray<FString> ReturnPinVariations = {
            TEXT("ReturnValue"), TEXT("Return Value"), TEXT("OutputDelegate"), TEXT("Value"), TEXT("Result")
        };

        for (const FString& Variation : ReturnPinVariations)
        {
            for (UEdGraphPin* Pin : TargetNode->Pins)
            {
                if (Pin && Pin->PinName.ToString() == Variation)
                {
                    TargetPin = Pin;
                    break;
                }
            }
            if (TargetPin) break;
        }
    }

    if (!SourcePin || !TargetPin)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectPins: Pin not found - Source '%s': %s, Target '%s': %s"),
               *SourcePinName, SourcePin ? TEXT("FOUND") : TEXT("NOT FOUND"),
               *TargetPinName, TargetPin ? TEXT("FOUND") : TEXT("NOT FOUND"));
        return false;
    }

    UEdGraph* Graph = SourceNode->GetGraph();
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectPins: No graph found for source node"));
        return false;
    }

    const UEdGraphSchema* Schema = Graph->GetSchema();
    if (!Schema)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }

    FPinConnectionResponse Response = Schema->CanCreateConnection(SourcePin, TargetPin);

    if (Response.Response == CONNECT_RESPONSE_DISALLOW)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectPins: Connection not allowed - %s"), *Response.Message.ToString());
        return false;
    }

    bool bSuccess = Schema->TryCreateConnection(SourcePin, TargetPin);
    return bSuccess;
}

bool FBlueprintNodeConnectionService::ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName, TArray<FAutoInsertedNodeInfo>* OutAutoInsertedNodes, FString* OutErrorMessage)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Invalid graph or node(s)");
        }
        return false;
    }

    // Track existing nodes before connection to detect auto-inserted nodes
    TSet<UEdGraphNode*> ExistingNodes;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        ExistingNodes.Add(Node);
    }

    // Find the pins
    UEdGraphPin* SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);

    if (!SourcePin || !TargetPin)
    {
        FString ErrorMsg = FString::Printf(TEXT("Pin not found - Source '%s': %s, Target '%s': %s"),
               *SourcePinName, SourcePin ? TEXT("FOUND") : TEXT("NOT FOUND"),
               *TargetPinName, TargetPin ? TEXT("FOUND") : TEXT("NOT FOUND"));
        UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
        if (OutErrorMessage)
        {
            *OutErrorMessage = ErrorMsg;
        }
        return false;
    }

    // Use Unreal's built-in schema validation
    const UEdGraphSchema* Schema = Graph->GetSchema();
    if (Schema)
    {
        FPinConnectionResponse Response = Schema->CanCreateConnection(SourcePin, TargetPin);
        if (Response.Response == CONNECT_RESPONSE_DISALLOW)
        {
            // Build a detailed error message with pin type information
            FString SourcePinType = SourcePin->PinType.PinCategory.ToString();
            FString TargetPinType = TargetPin->PinType.PinCategory.ToString();
            bool bSourceIsWildcard = (SourcePinType == TEXT("wildcard"));
            bool bTargetIsWildcard = (TargetPinType == TEXT("wildcard"));
            bool bTargetIsReference = TargetPin->PinType.bIsReference;

            FString DetailedError = FString::Printf(
                TEXT("Connection rejected: %s\n")
                TEXT("  Source pin '%s' type: %s%s\n")
                TEXT("  Target pin '%s' type: %s%s%s"),
                *Response.Message.ToString(),
                *SourcePinName, *SourcePinType, bSourceIsWildcard ? TEXT(" (wildcard - needs typed connection first)") : TEXT(""),
                *TargetPinName, *TargetPinType,
                bTargetIsWildcard ? TEXT(" (wildcard - needs typed connection first)") : TEXT(""),
                bTargetIsReference ? TEXT(" (reference parameter)") : TEXT(""));

            // Add hint for container operations with wildcard pins
            if (bTargetIsWildcard || bSourceIsWildcard)
            {
                DetailedError += TEXT("\n\nHINT: This function has wildcard pins (like Map_Add, Array_Add).");
                DetailedError += TEXT("\nConnect your typed container variable FIRST to resolve the wildcard types,");
                DetailedError += TEXT("\nthen connect other pins (Key, Value, etc.).");
            }

            UE_LOG(LogTemp, Error, TEXT("%s"), *DetailedError);
            if (OutErrorMessage)
            {
                *OutErrorMessage = DetailedError;
            }
            return false;
        }
    }

    // Check if we need a cast node using the dedicated service
    FBlueprintCastNodeService& CastService = FBlueprintCastNodeService::Get();
    bool bNeedsCast = CastService.DoesCastNeed(SourcePin, TargetPin);

    // If we need a cast, create it immediately
    if (bNeedsCast)
    {
        bool bCastCreated = CastService.CreateCastNode(Graph, SourcePin, TargetPin);
        if (bCastCreated)
        {
            UE_LOG(LogTemp, Display, TEXT("Auto-cast successful - created conversion node"));
            return true;
        }
        // Continue with direct connection attempt as fallback
    }

    // For execution pins, break existing connections first
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        if (SourcePin->LinkedTo.Num() > 0)
        {
            SourcePin->BreakAllPinLinks();
        }
    }

    // Use Schema->TryCreateConnection for proper validation instead of raw MakeLinkTo.
    // MakeLinkTo is low-level and bypasses schema constraints — it can create connections
    // that appear valid in LinkedTo but get silently dropped on compile/reconstruct (phantom success).
    bool bConnectionExists = false;
    if (Schema)
    {
        bConnectionExists = Schema->TryCreateConnection(SourcePin, TargetPin);
        if (!bConnectionExists)
        {
            FString ErrorMsg = FString::Printf(TEXT("Schema->TryCreateConnection failed for '%s'.%s -> '%s'.%s (types: %s -> %s)"),
                *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *SourcePinName,
                *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *TargetPinName,
                *SourcePin->PinType.PinCategory.ToString(), *TargetPin->PinType.PinCategory.ToString());
            UE_LOG(LogTemp, Error, TEXT("ConnectNodesWithAutoCast: %s"), *ErrorMsg);
            if (OutErrorMessage)
            {
                *OutErrorMessage = ErrorMsg;
            }
        }
    }
    else
    {
        // Fallback to raw MakeLinkTo only when no schema is available
        SourcePin->MakeLinkTo(TargetPin);
        bConnectionExists = SourcePin->LinkedTo.Contains(TargetPin);
    }

    // Notify nodes about pin connection changes
    if (bConnectionExists)
    {
        SourceNode->PinConnectionListChanged(SourcePin);
        TargetNode->PinConnectionListChanged(TargetPin);
    }

    // If direct connection failed and we didn't try cast yet, try now
    if (!bConnectionExists && !bNeedsCast)
    {
        bool bCastCreated = CastService.CreateCastNode(Graph, SourcePin, TargetPin);
        if (bCastCreated)
        {
            return true;
        }
        return false;
    }

    if (bConnectionExists)
    {
        // Handle PromotableOperator nodes
        if (UK2Node_PromotableOperator* PromotableOp = Cast<UK2Node_PromotableOperator>(SourceNode))
        {
            if (UEdGraph* SourceGraph = PromotableOp->GetGraph())
            {
                if (const UEdGraphSchema* SourceSchema = SourceGraph->GetSchema())
                {
                    SourceSchema->ForceVisualizationCacheClear();
                }
                SourceGraph->NotifyGraphChanged();
            }
        }

        if (UK2Node_PromotableOperator* PromotableOp = Cast<UK2Node_PromotableOperator>(TargetNode))
        {
            if (UEdGraph* TargetGraph = PromotableOp->GetGraph())
            {
                if (const UEdGraphSchema* TargetSchema = TargetGraph->GetSchema())
                {
                    TargetSchema->ForceVisualizationCacheClear();
                }
                TargetGraph->NotifyGraphChanged();
            }
        }
    }

    // Detect any newly created nodes (auto-inserted casts, conversions, etc.)
    if (OutAutoInsertedNodes)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!ExistingNodes.Contains(Node))
            {
                FAutoInsertedNodeInfo NodeInfo;
                NodeInfo.NodeId = FGraphUtils::GetReliableNodeId(Node);
                NodeInfo.NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                NodeInfo.NodeType = Node->GetClass()->GetName();

                bool bIsCastNode = Node->GetClass()->GetName().Contains(TEXT("DynamicCast"));
                NodeInfo.bRequiresExecConnection = bIsCastNode;

                if (bIsCastNode)
                {
                    bool bHasExecInput = false;
                    bool bHasExecOutput = false;
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                        {
                            if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
                            {
                                bHasExecInput = true;
                            }
                            else if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                            {
                                bHasExecOutput = true;
                            }
                        }
                    }
                    NodeInfo.bExecConnected = bHasExecInput && bHasExecOutput;
                }

                OutAutoInsertedNodes->Add(NodeInfo);
            }
        }
    }

    return bConnectionExists;
}

bool FBlueprintNodeConnectionService::ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType)
{
    // Delegate to the cast service
    return FBlueprintCastNodeService::Get().ArePinTypesCompatible(SourcePinType, TargetPinType);
}

bool FBlueprintNodeConnectionService::CreateCastNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Delegate to the cast service
    return FBlueprintCastNodeService::Get().CreateCastNode(Graph, SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CreateIntToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return FBlueprintCastNodeService::Get().CreateIntToStringCast(Graph, SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CreateFloatToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return FBlueprintCastNodeService::Get().CreateFloatToStringCast(Graph, SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CreateBoolToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return FBlueprintCastNodeService::Get().CreateBoolToStringCast(Graph, SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CreateStringToIntCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return FBlueprintCastNodeService::Get().CreateStringToIntCast(Graph, SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CreateStringToFloatCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return FBlueprintCastNodeService::Get().CreateStringToFloatCast(Graph, SourcePin, TargetPin);
}

bool FBlueprintNodeConnectionService::CreateObjectCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin,
                                                        FAutoInsertedNodeInfo* OutNodeInfo)
{
    return FBlueprintCastNodeService::Get().CreateObjectCast(Graph, SourcePin, TargetPin, OutNodeInfo);
}

UEdGraphNode* FBlueprintNodeConnectionService::FindNodeByIdOrType(UEdGraph* Graph, const FString& NodeIdOrType)
{
    if (!Graph)
    {
        return nullptr;
    }

    // First try to find by exact GUID or safe node ID
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            FString SafeNodeId = GetSafeNodeId(Node, NodeTitle);

            if (FGraphUtils::GetReliableNodeId(Node) == NodeIdOrType || SafeNodeId == NodeIdOrType)
            {
                return Node;
            }
        }
    }

    // If not found by GUID, try to find by node title (for Entry/Exit nodes)
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();

            if (NodeIdOrType == TEXT("FunctionEntry") || NodeIdOrType == TEXT("CanInteract"))
            {
                if (NodeTitle.Contains(TEXT("CanInteract")) && !NodeTitle.Contains(TEXT("Return")))
                {
                    return Node;
                }
            }
            else if (NodeIdOrType == TEXT("FunctionResult") || NodeIdOrType == TEXT("Return Node"))
            {
                if (NodeTitle.Contains(TEXT("Return")) && NodeTitle.Contains(TEXT("Node")))
                {
                    return Node;
                }
            }
            else if (NodeTitle == NodeIdOrType)
            {
                return Node;
            }
        }
    }

    return nullptr;
}
