#include "Utils/GraphUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"

bool FGraphUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                   UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }

    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);

    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }

    return false;
}

UEdGraphPin* FGraphUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }

    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"),
           *PinName, (int32)Direction, *Node->GetName());

    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"),
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }

    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }

    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) &&
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }

    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

UK2Node_Event* FGraphUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

FString FGraphUtils::GetReliableNodeId(UEdGraphNode* Node)
{
    if (!Node)
    {
        return TEXT("00000000000000000000000000000000");
    }

    // Check if NodeGuid is valid (not all zeros)
    if (Node->NodeGuid.IsValid())
    {
        return Node->NodeGuid.ToString();
    }

    // NodeGuid is invalid - generate a stable ID from the object's unique ID
    // This ensures we get a unique identifier even for nodes with uninitialized GUIDs
    // Format: "OBJID_" followed by the hex representation of the unique ID
    uint32 UniqueId = Node->GetUniqueID();
    return FString::Printf(TEXT("OBJID_%08X%08X%08X%08X"), UniqueId, UniqueId ^ 0xDEADBEEF, UniqueId ^ 0xCAFEBABE, UniqueId ^ 0x12345678);
}

void FGraphUtils::DetectGraphWarnings(UEdGraph* Graph, TArray<FGraphWarning>& OutWarnings)
{
    if (!Graph)
    {
        return;
    }

    FString GraphName = Graph->GetName();

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Check for Cast nodes (K2Node_DynamicCast) with disconnected exec pins
        FString NodeClassName = Node->GetClass()->GetName();
        if (NodeClassName.Contains(TEXT("DynamicCast")))
        {
            bool bHasExecInput = false;
            bool bHasExecOutput = false;

            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
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

            // Warn if cast node has disconnected exec pins
            if (!bHasExecInput || !bHasExecOutput)
            {
                FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                FString Message = FString::Printf(
                    TEXT("Cast node '%s' has disconnected exec pins - it will NOT execute at runtime"),
                    *NodeTitle);

                OutWarnings.Add(FGraphWarning(
                    TEXT("disconnected_cast_exec"),
                    GetReliableNodeId(Node),
                    NodeTitle,
                    GraphName,
                    Message
                ));
            }
        }
    }
}

void FGraphUtils::DetectBlueprintWarnings(UBlueprint* Blueprint, TArray<FGraphWarning>& OutWarnings)
{
    if (!Blueprint)
    {
        return;
    }

    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        DetectGraphWarnings(Graph, OutWarnings);
    }
}
