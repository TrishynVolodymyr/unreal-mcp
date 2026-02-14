#include "Commands/PCG/ConnectPCGNodesCommand.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGEdge.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Utils/PCGEditorRefreshUtils.h"

namespace
{
    UPCGNode* FindNodeByName(UPCGGraph* Graph, const FString& NodeName)
    {
        if (Graph->GetInputNode() && Graph->GetInputNode()->GetName() == NodeName)
        {
            return Graph->GetInputNode();
        }
        if (Graph->GetOutputNode() && Graph->GetOutputNode()->GetName() == NodeName)
        {
            return Graph->GetOutputNode();
        }
        for (UPCGNode* Node : Graph->GetNodes())
        {
            if (Node && Node->GetName() == NodeName)
            {
                return Node;
            }
        }
        return nullptr;
    }
}

FConnectPCGNodesCommand::FConnectPCGNodesCommand()
{
}

FString FConnectPCGNodesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Parse required parameters
    FString GraphPath;
    if (!JsonObject->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString SourceNodeId;
    if (!JsonObject->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString TargetNodeId;
    if (!JsonObject->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    // Parse optional parameters with defaults
    FString SourcePin = TEXT("Out");
    JsonObject->TryGetStringField(TEXT("source_pin"), SourcePin);

    FString TargetPin = TEXT("In");
    JsonObject->TryGetStringField(TEXT("target_pin"), TargetPin);

    // Load the PCG Graph asset
    UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
    if (!Graph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found at path: %s"), *GraphPath));
    }

    // Find source node
    UPCGNode* SourceNode = FindNodeByName(Graph, SourceNodeId);
    if (!SourceNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Source node not found: %s"), *SourceNodeId));
    }

    // Find target node
    UPCGNode* TargetNode = FindNodeByName(Graph, TargetNodeId);
    if (!TargetNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId));
    }

    // Validate that the source pin exists on the source node
    bool bSourcePinFound = false;
    for (const UPCGPin* Pin : SourceNode->GetOutputPins())
    {
        if (Pin && Pin->Properties.Label.ToString() == SourcePin)
        {
            bSourcePinFound = true;
            break;
        }
    }
    if (!bSourcePinFound)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Source pin '%s' not found on node '%s'"), *SourcePin, *SourceNodeId));
    }

    // Validate that the target pin exists on the target node
    bool bTargetPinFound = false;
    for (const UPCGPin* Pin : TargetNode->GetInputPins())
    {
        if (Pin && Pin->Properties.Label.ToString() == TargetPin)
        {
            bTargetPinFound = true;
            break;
        }
    }
    if (!bTargetPinFound)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Target pin '%s' not found on node '%s'"), *TargetPin, *TargetNodeId));
    }

    // Check if edge already exists
    UPCGPin* SrcPin = SourceNode->GetOutputPin(FName(*SourcePin));
    if (SrcPin)
    {
        for (const UPCGEdge* Edge : SrcPin->Edges)
        {
            const UPCGPin* OtherPin = Edge ? Edge->GetOtherPin(SrcPin) : nullptr;
            if (OtherPin && OtherPin->Node == TargetNode
                && OtherPin->Properties.Label.ToString() == TargetPin)
            {
                return CreateErrorResponse(FString::Printf(
                    TEXT("Edge already exists: %s.%s -> %s.%s"),
                    *SourceNodeId, *SourcePin, *TargetNodeId, *TargetPin));
            }
        }
    }

    // Add the edge
    UPCGNode* Result = Graph->AddEdge(SourceNode, FName(*SourcePin), TargetNode, FName(*TargetPin));
    if (!Result)
    {
        return CreateErrorResponse(FString::Printf(
            TEXT("Failed to create edge from %s.%s to %s.%s"),
            *SourceNodeId, *SourcePin, *TargetNodeId, *TargetPin));
    }

    // Notify editor and save graph to disk
#if WITH_EDITOR
    Graph->OnGraphChangedDelegate.Broadcast(Graph, EPCGChangeType::Edge);
#endif
    Graph->MarkPackageDirty();
    {
        UPackage* Package = Graph->GetOutermost();
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, Graph, *PackageFileName, SaveArgs);
    }

    // Refresh the PCG editor graph if it's open
    PCGEditorRefreshUtils::RefreshEditorGraph(Graph);

    FString Message = FString::Printf(TEXT("Connected %s.%s -> %s.%s"),
        *SourceNodeId, *SourcePin, *TargetNodeId, *TargetPin);

    return CreateSuccessResponse(Message);
}

FString FConnectPCGNodesCommand::GetCommandName() const
{
    return TEXT("connect_pcg_nodes");
}

bool FConnectPCGNodesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString GraphPath, SourceNodeId, TargetNodeId;
    if (!JsonObject->TryGetStringField(TEXT("graph_path"), GraphPath)) return false;
    if (!JsonObject->TryGetStringField(TEXT("source_node_id"), SourceNodeId)) return false;
    if (!JsonObject->TryGetStringField(TEXT("target_node_id"), TargetNodeId)) return false;

    return true;
}

FString FConnectPCGNodesCommand::CreateSuccessResponse(const FString& Message) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), Message);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FConnectPCGNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
