#include "Commands/PCG/ConfigurePCGMeshSpawnerCommand.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "MeshSelectors/PCGMeshSelectorWeighted.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SavePackage.h"
#include "Utils/PCGEditorRefreshUtils.h"
#include "Utils/PCGNodeUtils.h"

FConfigurePCGMeshSpawnerCommand::FConfigurePCGMeshSpawnerCommand()
{
}

FString FConfigurePCGMeshSpawnerCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return CreateErrorResponse(TEXT("Invalid JSON parameters"));
	}

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

	const TArray<TSharedPtr<FJsonValue>>* MeshEntriesJson = nullptr;
	if (!JsonObject->TryGetArrayField(TEXT("mesh_entries"), MeshEntriesJson) || !MeshEntriesJson)
	{
		return CreateErrorResponse(TEXT("Missing 'mesh_entries' parameter"));
	}

	bool bAppend = false;
	JsonObject->TryGetBoolField(TEXT("append"), bAppend);

	// Load graph
	UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
	if (!Graph)
	{
		return CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found at path: %s"), *GraphPath));
	}

	// Find node
	UPCGNode* Node = PCGNodeUtils::FindNodeByName(Graph, NodeId);
	if (!Node)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// Get settings, cast to Static Mesh Spawner
	UPCGSettings* Settings = Node->GetSettings();
	if (!Settings)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node '%s' has no settings"), *NodeId));
	}

	UPCGStaticMeshSpawnerSettings* SpawnerSettings = Cast<UPCGStaticMeshSpawnerSettings>(Settings);
	if (!SpawnerSettings)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node '%s' is not a Static Mesh Spawner (class: %s)"),
			*NodeId, *Settings->GetClass()->GetName()));
	}

	// Get MeshSelectorParameters, cast to weighted selector
	UPCGMeshSelectorWeighted* MeshSelector = Cast<UPCGMeshSelectorWeighted>(SpawnerSettings->MeshSelectorParameters);
	if (!MeshSelector)
	{
		return CreateErrorResponse(TEXT("MeshSelectorParameters is not UPCGMeshSelectorWeighted. "
			"Ensure the node uses the Weighted mesh selector."));
	}

#if WITH_EDITOR
	MeshSelector->PreEditChange(nullptr);
#endif

	// Clear or keep existing entries
	if (!bAppend)
	{
		MeshSelector->MeshEntries.Empty();
	}

	// Add new entries
	int32 AddedCount = 0;
	for (const TSharedPtr<FJsonValue>& EntryValue : *MeshEntriesJson)
	{
		const TSharedPtr<FJsonObject>* EntryObj = nullptr;
		if (!EntryValue->TryGetObject(EntryObj) || !EntryObj || !(*EntryObj).IsValid())
		{
			continue;
		}

		FString MeshPath;
		if (!(*EntryObj)->TryGetStringField(TEXT("mesh"), MeshPath))
		{
			continue;
		}

		int32 Weight = 1;
		(*EntryObj)->TryGetNumberField(TEXT("weight"), Weight);

		FPCGMeshSelectorWeightedEntry NewEntry;
		NewEntry.Descriptor.StaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
		NewEntry.Weight = Weight;

		MeshSelector->MeshEntries.Add(NewEntry);
		AddedCount++;
	}

#if WITH_EDITOR
	FPropertyChangedEvent PropertyChangedEvent(nullptr);
	MeshSelector->PostEditChangeProperty(PropertyChangedEvent);
#endif

	// Save
	Settings->MarkPackageDirty();
	Graph->MarkPackageDirty();
	{
		UPackage* Package = Graph->GetOutermost();
		FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, Graph, *PackageFileName, SaveArgs);
	}

	// Refresh editor
	PCGEditorRefreshUtils::RefreshEditorGraph(Graph);

	// Build response
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("node_id"), NodeId);
	ResponseObj->SetNumberField(TEXT("entries_count"), MeshSelector->MeshEntries.Num());
	ResponseObj->SetNumberField(TEXT("added_count"), AddedCount);
	ResponseObj->SetBoolField(TEXT("appended"), bAppend);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

	return OutputString;
}

FString FConfigurePCGMeshSpawnerCommand::GetCommandName() const
{
	return TEXT("configure_pcg_mesh_spawner");
}

bool FConfigurePCGMeshSpawnerCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	return JsonObject->HasField(TEXT("graph_path")) &&
	       JsonObject->HasField(TEXT("node_id")) &&
	       JsonObject->HasField(TEXT("mesh_entries"));
}

FString FConfigurePCGMeshSpawnerCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

	return OutputString;
}
