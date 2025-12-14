#include "Services/BlueprintNode/BlueprintNodeQueryService.h"
#include "Services/IBlueprintNodeService.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Utils/UnrealMCPCommonUtils.h"

// Helper function to generate safe Node IDs for query service (not in namespace to avoid unity build conflicts)
static FString GetSafeNodeIdForQuery(UEdGraphNode* Node, const FString& NodeTitle)
{
    if (!Node)
    {
        return TEXT("InvalidNode");
    }

    FString NodeId = Node->NodeGuid.ToString();
    if (NodeId.IsEmpty() || NodeId == TEXT("00000000-0000-0000-0000-000000000000") || NodeId == TEXT("00000000000000000000000000000000"))
    {
        // Generate fallback ID using node pointer and title
        FString SafeTitle = NodeTitle.Replace(TEXT(" "), TEXT("_")).Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT(""));
        NodeId = FString::Printf(TEXT("Node_%p_%s"), Node, *SafeTitle);
    }

    return NodeId;
}

namespace
{
// Helper function to collect pin information from a node
static TArray<FBlueprintPinInfo> GetNodePinInfo(UEdGraphNode* Node)
{
    TArray<FBlueprintPinInfo> PinInfos;

    if (!Node)
    {
        return PinInfos;
    }

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin)
        {
            FString PinName = Pin->PinName.ToString();
            FString PinType;
            FString Direction = Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output");
            bool bIsExecution = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;

            // Determine pin type
            if (bIsExecution)
            {
                PinType = TEXT("exec");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
            {
                PinType = TEXT("bool");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
            {
                PinType = TEXT("int");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
            {
                PinType = TEXT("real");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
            {
                PinType = TEXT("string");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
            {
                PinType = TEXT("text");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
            {
                PinType = TEXT("object");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                PinType = TEXT("struct");
            }
            else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
            {
                PinType = TEXT("wildcard");
            }
            else
            {
                PinType = Pin->PinType.PinCategory.ToString();
            }

            PinInfos.Add(FBlueprintPinInfo(PinName, PinType, Direction, bIsExecution));
        }
    }

    return PinInfos;
}

// Helper function to check if a node is pure (has no execution pins)
static bool IsNodePure(UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            return false; // Has execution pin, not pure
        }
    }

    return true; // No execution pins found, it's pure
}

} // anonymous namespace

FBlueprintNodeQueryService& FBlueprintNodeQueryService::Get()
{
    static FBlueprintNodeQueryService Instance;
    return Instance;
}

bool FBlueprintNodeQueryService::FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FBlueprintNodeInfo>& OutNodeInfos)
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

                    // PROBLEM 2 FIX: Use safe Node ID generation
                    FString NodeId = GetSafeNodeIdForQuery(Node, NodeTitle);

                    // Get node type
                    FString NodeType = Node->GetClass()->GetName();

                    // Get node position
                    FVector2D Position(Node->NodePosX, Node->NodePosY);

                    // Collect pin information
                    TArray<FBlueprintPinInfo> PinInfos = GetNodePinInfo(Node);

                    // Create node info and set IsPure flag
                    FBlueprintNodeInfo NodeInfo(NodeId, NodeTitle, NodeType, Position, PinInfos);
                    NodeInfo.IsPure = IsNodePure(Node);
                    OutNodeInfos.Add(NodeInfo);
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

                            // PROBLEM 2 FIX: Use safe Node ID generation
                            FString NodeId = GetSafeNodeIdForQuery(Node, NodeTitle);

                            // Get node type
                            FString NodeType = Node->GetClass()->GetName();

                            // Get node position
                            FVector2D Position(Node->NodePosX, Node->NodePosY);

                            // Collect pin information
                            TArray<FBlueprintPinInfo> PinInfos = GetNodePinInfo(Node);

                            // Create node info and set IsPure flag
                            FBlueprintNodeInfo NodeInfo(NodeId, NodeTitle, NodeType, Position, PinInfos);
                            NodeInfo.IsPure = IsNodePure(Node);
                            OutNodeInfos.Add(NodeInfo);
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
                        FBlueprintNodeInfo NodeInfo(GetSafeNodeIdForQuery(EventNode, NodeTitle), NodeTitle);
                        NodeInfo.IsPure = IsNodePure(EventNode);
                        OutNodeInfos.Add(NodeInfo);
                    }
                }
                else
                {
                    // Add all event nodes
                    FString NodeTitle = EventNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                    OutNodeInfos.Add(FBlueprintNodeInfo(GetSafeNodeIdForQuery(EventNode, NodeTitle), NodeTitle));
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
                FBlueprintNodeInfo NodeInfo(GetSafeNodeIdForQuery(FunctionNode, NodeTitle), NodeTitle);
                NodeInfo.IsPure = IsNodePure(FunctionNode);
                OutNodeInfos.Add(NodeInfo);
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
                OutNodeInfos.Add(FBlueprintNodeInfo(GetSafeNodeIdForQuery(Node, NodeTitle), NodeTitle));
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
                    FBlueprintNodeInfo NodeInfo(GetSafeNodeIdForQuery(Node, NodeTitle), NodeTitle);
                    NodeInfo.IsPure = IsNodePure(Node);
                    OutNodeInfos.Add(NodeInfo);
                }
            }
        }
    }

    return true;
}

bool FBlueprintNodeQueryService::GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames)
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

bool FBlueprintNodeQueryService::GetVariableInfo(UBlueprint* Blueprint, const FString& VariableName, FString& OutVariableType, TSharedPtr<FJsonObject>& OutAdditionalInfo)
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

UEdGraphNode* FBlueprintNodeQueryService::FindNodeById(UBlueprint* Blueprint, const FString& NodeId)
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

UEdGraph* FBlueprintNodeQueryService::FindGraphInBlueprint(UBlueprint* Blueprint, const FString& GraphName)
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

FString FBlueprintNodeQueryService::GenerateNodeId(UEdGraphNode* Node) const
{
    if (!Node)
    {
        return TEXT("");
    }

    // Generate a unique ID based on the node's memory address and class name
    return FString::Printf(TEXT("%s_%p"), *Node->GetClass()->GetName(), Node);
}

FString FBlueprintNodeQueryService::GetCleanTypePromotionTitle(UEdGraphNode* Node, const FString& OriginalTitle) const
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
