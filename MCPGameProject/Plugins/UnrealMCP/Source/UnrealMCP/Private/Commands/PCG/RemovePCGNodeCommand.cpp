#include "Commands/PCG/RemovePCGNodeCommand.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SavePackage.h"
#include "Utils/PCGEditorRefreshUtils.h"

FRemovePCGNodeCommand::FRemovePCGNodeCommand()
{
}

FString FRemovePCGNodeCommand::Execute(const FString& Parameters)
{
	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return CreateErrorResponse(TEXT("Invalid JSON parameters"));
	}

	// Required params
	FString GraphPath;
	if (!JsonObject->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!JsonObject->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	// Load the PCG Graph asset
	UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
	if (!Graph)
	{
		return CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found at path: %s"), *GraphPath));
	}

	// Check if the target is an Input or Output node (not allowed to remove)
	UPCGNode* InputNode = Graph->GetInputNode();
	UPCGNode* OutputNode = Graph->GetOutputNode();

	if (InputNode && InputNode->GetName() == NodeId)
	{
		return CreateErrorResponse(TEXT("Cannot remove the Input node from a PCG Graph"));
	}

	if (OutputNode && OutputNode->GetName() == NodeId)
	{
		return CreateErrorResponse(TEXT("Cannot remove the Output node from a PCG Graph"));
	}

	// Find the node among regular nodes
	UPCGNode* TargetNode = nullptr;
	const TArray<UPCGNode*>& Nodes = Graph->GetNodes();
	for (UPCGNode* Node : Nodes)
	{
		if (Node && Node->GetName() == NodeId)
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node '%s' not found in graph"), *NodeId));
	}

	// Get node title for the response message before removing
	FString NodeTitle = TargetNode->GetNodeTitle(EPCGNodeTitleType::ListView).ToString();
	if (NodeTitle.IsEmpty())
	{
		NodeTitle = NodeId;
	}

	// Remove the node (auto-disconnects edges)
	Graph->RemoveNode(TargetNode);

	// Notify editor and save graph to disk
#if WITH_EDITOR
	Graph->OnGraphChangedDelegate.Broadcast(Graph, EPCGChangeType::Structural);
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

	// Build response
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Removed node '%s'"), *NodeTitle));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

	return OutputString;
}

FString FRemovePCGNodeCommand::GetCommandName() const
{
	return TEXT("remove_pcg_node");
}

bool FRemovePCGNodeCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	return JsonObject->HasField(TEXT("graph_path")) && JsonObject->HasField(TEXT("node_id"));
}

FString FRemovePCGNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

	return OutputString;
}
