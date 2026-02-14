#include "Commands/PCG/AddPCGNodeCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGSettings.h"
#include "UObject/UObjectIterator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Utils/PCGEditorRefreshUtils.h"
#include "UObject/SavePackage.h"

FAddPCGNodeCommand::FAddPCGNodeCommand()
{
}

FString FAddPCGNodeCommand::Execute(const FString& Parameters)
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

	FString SettingsClassName;
	if (!JsonObject->TryGetStringField(TEXT("settings_class"), SettingsClassName))
	{
		return CreateErrorResponse(TEXT("Missing 'settings_class' parameter"));
	}

	// Optional params
	int32 NodePosX = 0;
	int32 NodePosY = 0;
	bool bHasPosition = false;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (JsonObject->TryGetArrayField(TEXT("node_position"), PositionArray) && PositionArray->Num() >= 2)
	{
		NodePosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		NodePosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
		bHasPosition = true;
	}

	FString NodeLabel;
	JsonObject->TryGetStringField(TEXT("node_label"), NodeLabel);

	// Load the PCG Graph asset
	UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
	if (!Graph)
	{
		return CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found at path: %s"), *GraphPath));
	}

	// Find the settings UClass by name
	UClass* SettingsClass = FindFirstObject<UClass>(*SettingsClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);

	// If not found, try with/without 'U' prefix
	if (!SettingsClass)
	{
		if (SettingsClassName.StartsWith(TEXT("U")))
		{
			FString WithoutPrefix = SettingsClassName.Mid(1);
			SettingsClass = FindFirstObject<UClass>(*WithoutPrefix, EFindFirstObjectOptions::EnsureIfAmbiguous);
		}
		else
		{
			FString WithPrefix = FString::Printf(TEXT("U%s"), *SettingsClassName);
			SettingsClass = FindFirstObject<UClass>(*WithPrefix, EFindFirstObjectOptions::EnsureIfAmbiguous);
		}
	}

	if (!SettingsClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Settings class not found: %s. Use search_pcg_palette to discover available classes."), *SettingsClassName));
	}

	// Verify it's a subclass of UPCGSettings
	if (!SettingsClass->IsChildOf(UPCGSettings::StaticClass()))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Class '%s' is not a subclass of UPCGSettings"), *SettingsClassName));
	}

	// Add the node to the graph
	UPCGSettings* DefaultNodeSettings = nullptr;
	UPCGNode* NewNode = Graph->AddNodeOfType(TSubclassOf<UPCGSettings>(SettingsClass), DefaultNodeSettings);

	if (!NewNode)
	{
		return CreateErrorResponse(TEXT("Failed to add node to graph"));
	}

	// Set node position
#if WITH_EDITOR
	if (bHasPosition)
	{
		NewNode->PositionX = NodePosX;
		NewNode->PositionY = NodePosY;
	}
#endif

	// Set node label if provided
	if (!NodeLabel.IsEmpty())
	{
#if WITH_EDITOR
		NewNode->NodeComment = NodeLabel;
#endif
	}

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
	ResponseObj->SetStringField(TEXT("node_id"), NewNode->GetName());

	// Get node title from settings
	FString NodeTitle;
	if (DefaultNodeSettings)
	{
		NodeTitle = DefaultNodeSettings->GetClass()->GetName();
		// Remove "Settings" suffix for cleaner display
		NodeTitle.RemoveFromEnd(TEXT("Settings"));
		// Remove "PCG" prefix if present
		if (NodeTitle.StartsWith(TEXT("PCG")))
		{
			NodeTitle = NodeTitle.Mid(3);
		}
	}
	ResponseObj->SetStringField(TEXT("node_title"), NodeTitle);
	ResponseObj->SetStringField(TEXT("settings_class"), SettingsClassName);

	// Position
	TArray<TSharedPtr<FJsonValue>> PosArray;
#if WITH_EDITOR
	PosArray.Add(MakeShared<FJsonValueNumber>(NewNode->PositionX));
	PosArray.Add(MakeShared<FJsonValueNumber>(NewNode->PositionY));
#else
	PosArray.Add(MakeShared<FJsonValueNumber>(NodePosX));
	PosArray.Add(MakeShared<FJsonValueNumber>(NodePosY));
#endif
	ResponseObj->SetArrayField(TEXT("position"), PosArray);

	// Pins
	TSharedPtr<FJsonObject> PinsObj = MakeShared<FJsonObject>();

	// Input pins
	TArray<TSharedPtr<FJsonValue>> InputPinsArray;
	for (const UPCGPin* Pin : NewNode->GetInputPins())
	{
		if (Pin)
		{
			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("label"), Pin->Properties.Label.ToString());
			InputPinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}
	}
	PinsObj->SetArrayField(TEXT("inputs"), InputPinsArray);

	// Output pins
	TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
	for (const UPCGPin* Pin : NewNode->GetOutputPins())
	{
		if (Pin)
		{
			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("label"), Pin->Properties.Label.ToString());
			OutputPinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}
	}
	PinsObj->SetArrayField(TEXT("outputs"), OutputPinsArray);

	ResponseObj->SetObjectField(TEXT("pins"), PinsObj);

	// Serialize response
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

	return OutputString;
}

FString FAddPCGNodeCommand::GetCommandName() const
{
	return TEXT("add_pcg_node");
}

bool FAddPCGNodeCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	return JsonObject->HasField(TEXT("graph_path")) && JsonObject->HasField(TEXT("settings_class"));
}

FString FAddPCGNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

	return OutputString;
}
