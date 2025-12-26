#include "Commands/GraphManipulation/CleanupBlueprintGraphCommand.h"
#include "Services/IBlueprintService.h"
#include "Utils/GraphUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

FCleanupBlueprintGraphCommand::FCleanupBlueprintGraphCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCleanupBlueprintGraphCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString GraphName;
    ECleanupMode CleanupMode;
    bool bIncludeEventGraph = false;
    FString ParseError;

    if (!ParseParameters(Parameters, BlueprintName, CleanupMode, GraphName, bIncludeEventGraph, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Execute the appropriate cleanup mode
    switch (CleanupMode)
    {
    case ECleanupMode::Orphans:
        return ExecuteOrphansCleanup(Blueprint, GraphName, bIncludeEventGraph);
    case ECleanupMode::PrintStrings:
        return ExecutePrintStringsCleanup(Blueprint, GraphName, bIncludeEventGraph);
    default:
        return CreateErrorResponse(TEXT("Invalid cleanup mode"));
    }
}

FString FCleanupBlueprintGraphCommand::GetCommandName() const
{
    return TEXT("cleanup_blueprint_graph");
}

bool FCleanupBlueprintGraphCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, GraphName, Error;
    ECleanupMode CleanupMode;
    bool bIncludeEventGraph;
    return ParseParameters(Parameters, BlueprintName, CleanupMode, GraphName, bIncludeEventGraph, Error);
}

bool FCleanupBlueprintGraphCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName,
                                                     ECleanupMode& OutCleanupMode, FString& OutGraphName,
                                                     bool& OutIncludeEventGraph, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Required: blueprint_name
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }

    // Required: cleanup_mode
    FString CleanupModeStr;
    if (!JsonObject->TryGetStringField(TEXT("cleanup_mode"), CleanupModeStr))
    {
        OutError = TEXT("Missing required 'cleanup_mode' parameter. Options: 'orphans', 'print_strings'");
        return false;
    }

    if (CleanupModeStr.Equals(TEXT("orphans"), ESearchCase::IgnoreCase))
    {
        OutCleanupMode = ECleanupMode::Orphans;
    }
    else if (CleanupModeStr.Equals(TEXT("print_strings"), ESearchCase::IgnoreCase))
    {
        OutCleanupMode = ECleanupMode::PrintStrings;
    }
    else
    {
        OutError = FString::Printf(TEXT("Invalid cleanup_mode '%s'. Options: 'orphans', 'print_strings'"), *CleanupModeStr);
        return false;
    }

    // Optional: graph_name
    JsonObject->TryGetStringField(TEXT("graph_name"), OutGraphName);

    // Optional: include_event_graph (default: false)
    OutIncludeEventGraph = false;
    JsonObject->TryGetBoolField(TEXT("include_event_graph"), OutIncludeEventGraph);

    return true;
}

FString FCleanupBlueprintGraphCommand::ExecuteOrphansCleanup(UBlueprint* Blueprint, const FString& GraphName,
                                                             bool bIncludeEventGraph)
{
    // Collect graphs to process
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    TArray<UEdGraph*> GraphsToProcess;
    TArray<FString> SkippedGraphs;

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        FString CurrentGraphName = Graph->GetName();

        // If specific graph requested, only process that one
        if (!GraphName.IsEmpty())
        {
            if (CurrentGraphName.Equals(GraphName, ESearchCase::IgnoreCase))
            {
                GraphsToProcess.Add(Graph);
            }
            continue;
        }

        // Skip EventGraph unless explicitly included
        if (!bIncludeEventGraph && CurrentGraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
        {
            SkippedGraphs.Add(TEXT("EventGraph (excluded by default)"));
            continue;
        }

        GraphsToProcess.Add(Graph);
    }

    // Check if specific graph was requested but not found
    if (!GraphName.IsEmpty() && GraphsToProcess.Num() == 0)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph '%s' not found"), *GraphName));
    }

    // Collect and delete orphaned nodes
    TArray<FString> DeletedNodeTitles;
    int32 TotalDeleted = 0;

    const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Orphaned Nodes")));

    for (UEdGraph* Graph : GraphsToProcess)
    {
        TArray<FString> OrphanedNodeIds;
        if (!FGraphUtils::DetectOrphanedNodes(Graph, OrphanedNodeIds))
        {
            continue;
        }

        // Build node ID to node map
        TMap<FString, UEdGraphNode*> NodeIdMap;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node)
            {
                NodeIdMap.Add(FGraphUtils::GetReliableNodeId(Node), Node);
            }
        }

        // Delete each orphaned node
        for (const FString& NodeId : OrphanedNodeIds)
        {
            UEdGraphNode** NodePtr = NodeIdMap.Find(NodeId);
            if (!NodePtr || !*NodePtr)
            {
                continue;
            }

            UEdGraphNode* Node = *NodePtr;

            // Skip auto-generated Return Nodes at (0,0)
            UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node);
            if (ResultNode && Node->NodePosX == 0 && Node->NodePosY == 0)
            {
                bool bHasConnections = false;
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin && Pin->LinkedTo.Num() > 0)
                    {
                        bHasConnections = true;
                        break;
                    }
                }
                if (!bHasConnections)
                {
                    continue;
                }
            }

            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            FString FullTitle = FString::Printf(TEXT("%s [%s]"), *NodeTitle, *Graph->GetName());

            Graph->Modify();
            Node->Modify();

            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin)
                {
                    Pin->BreakAllPinLinks();
                }
            }

            Graph->RemoveNode(Node);
            DeletedNodeTitles.Add(FullTitle);
            TotalDeleted++;
        }
    }

    if (TotalDeleted > 0)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    return CreateSuccessResponse(Blueprint->GetName(), TEXT("orphans"), TotalDeleted, 0, DeletedNodeTitles, SkippedGraphs);
}

FString FCleanupBlueprintGraphCommand::ExecutePrintStringsCleanup(UBlueprint* Blueprint, const FString& GraphName,
                                                                   bool bIncludeEventGraph)
{
    // Collect graphs to process
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    TArray<UEdGraph*> GraphsToProcess;
    TArray<FString> SkippedGraphs;

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        FString CurrentGraphName = Graph->GetName();

        // If specific graph requested, only process that one
        if (!GraphName.IsEmpty())
        {
            if (CurrentGraphName.Equals(GraphName, ESearchCase::IgnoreCase))
            {
                GraphsToProcess.Add(Graph);
            }
            continue;
        }

        // Skip EventGraph unless explicitly included
        if (!bIncludeEventGraph && CurrentGraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
        {
            SkippedGraphs.Add(TEXT("EventGraph (excluded by default)"));
            continue;
        }

        GraphsToProcess.Add(Graph);
    }

    if (!GraphName.IsEmpty() && GraphsToProcess.Num() == 0)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph '%s' not found"), *GraphName));
    }

    TArray<FString> DeletedNodeTitles;
    int32 TotalDeleted = 0;
    int32 TotalRewired = 0;

    const FScopedTransaction Transaction(FText::FromString(TEXT("Cleanup Print String Nodes")));

    for (UEdGraph* Graph : GraphsToProcess)
    {
        // Collect all Print String nodes first (avoid modifying while iterating)
        TArray<UEdGraphNode*> PrintStringNodes;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node && IsPrintStringNode(Node))
            {
                PrintStringNodes.Add(Node);
            }
        }

        // Process each Print String node
        for (UEdGraphNode* Node : PrintStringNodes)
        {
            UEdGraphPin* ExecIn = GetExecInputPin(Node);
            UEdGraphPin* ExecOut = GetExecOutputPin(Node);

            bool bIsMiddleware = (ExecIn && ExecIn->LinkedTo.Num() > 0 &&
                                  ExecOut && ExecOut->LinkedTo.Num() > 0);

            Graph->Modify();
            Node->Modify();

            if (bIsMiddleware)
            {
                // Rewire: connect source's exec to target's exec
                // Get the source node's output pin that connects to our execute
                UEdGraphPin* SourceExecOut = ExecIn->LinkedTo[0];
                // Get the target node's input pin that we connect to
                UEdGraphPin* TargetExecIn = ExecOut->LinkedTo[0];

                if (SourceExecOut && TargetExecIn)
                {
                    // Break existing connections
                    ExecIn->BreakAllPinLinks();
                    ExecOut->BreakAllPinLinks();

                    // Create new connection bypassing the Print String
                    SourceExecOut->MakeLinkTo(TargetExecIn);
                    TotalRewired++;
                }
            }

            // Break any remaining connections (data pins, etc.)
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin)
                {
                    Pin->BreakAllPinLinks();
                }
            }

            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            FString FullTitle = FString::Printf(TEXT("%s [%s]%s"), *NodeTitle, *Graph->GetName(),
                                                bIsMiddleware ? TEXT(" (rewired)") : TEXT(""));

            Graph->RemoveNode(Node);
            DeletedNodeTitles.Add(FullTitle);
            TotalDeleted++;
        }
    }

    if (TotalDeleted > 0)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    return CreateSuccessResponse(Blueprint->GetName(), TEXT("print_strings"), TotalDeleted, TotalRewired,
                                DeletedNodeTitles, SkippedGraphs);
}

bool FCleanupBlueprintGraphCommand::IsPrintStringNode(UEdGraphNode* Node) const
{
    UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node);
    if (!FuncNode)
    {
        return false;
    }

    UFunction* Func = FuncNode->GetTargetFunction();
    if (!Func)
    {
        return false;
    }

    // Check if this is a PrintString function
    FString FuncName = Func->GetName();
    return FuncName.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase) ||
           FuncName.Contains(TEXT("PrintString"));
}

UEdGraphPin* FCleanupBlueprintGraphCommand::GetExecInputPin(UEdGraphNode* Node) const
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
            Pin->Direction == EGPD_Input)
        {
            return Pin;
        }
    }
    return nullptr;
}

UEdGraphPin* FCleanupBlueprintGraphCommand::GetExecOutputPin(UEdGraphNode* Node) const
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
            Pin->Direction == EGPD_Output)
        {
            return Pin;
        }
    }
    return nullptr;
}

FString FCleanupBlueprintGraphCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& CleanupMode,
                                                              int32 DeletedCount, int32 RewiredCount,
                                                              const TArray<FString>& DeletedNodeTitles,
                                                              const TArray<FString>& SkippedGraphs) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("cleanup_mode"), CleanupMode);
    ResponseObj->SetNumberField(TEXT("deleted_count"), DeletedCount);

    if (CleanupMode.Equals(TEXT("print_strings")))
    {
        ResponseObj->SetNumberField(TEXT("rewired_count"), RewiredCount);
    }

    TArray<TSharedPtr<FJsonValue>> DeletedArray;
    for (const FString& Title : DeletedNodeTitles)
    {
        DeletedArray.Add(MakeShared<FJsonValueString>(Title));
    }
    ResponseObj->SetArrayField(TEXT("deleted_nodes"), DeletedArray);

    if (SkippedGraphs.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> SkippedArray;
        for (const FString& Skipped : SkippedGraphs)
        {
            SkippedArray.Add(MakeShared<FJsonValueString>(Skipped));
        }
        ResponseObj->SetArrayField(TEXT("skipped_graphs"), SkippedArray);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCleanupBlueprintGraphCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
