#include "Commands/UMG/GetWidgetBlueprintMetadataCommand.h"
#include "Services/UMG/IUMGService.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogGetWidgetBlueprintMetadata, Log, All);

FGetWidgetBlueprintMetadataCommand::FGetWidgetBlueprintMetadataCommand(TSharedPtr<IUMGService> InUMGService)
	: UMGService(InUMGService)
	, MetadataBuilder(InUMGService)
{
}

FString FGetWidgetBlueprintMetadataCommand::GetCommandName() const
{
	return TEXT("get_widget_blueprint_metadata");
}

FString FGetWidgetBlueprintMetadataCommand::Execute(const FString& Parameters)
{
	UE_LOG(LogGetWidgetBlueprintMetadata, Log, TEXT("Executing get_widget_blueprint_metadata command"));

	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(TEXT("Invalid JSON parameters"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Validate parameters
	FString ValidationError;
	if (!ValidateParamsInternal(JsonObject, ValidationError))
	{
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(ValidationError);
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Execute and return result
	TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

bool FGetWidgetBlueprintMetadataCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString ValidationError;
	return ValidateParamsInternal(JsonObject, ValidationError);
}

bool FGetWidgetBlueprintMetadataCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
	if (!Params.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	// Check for widget_name (required)
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing required parameter: widget_name");
		return false;
	}

	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	if (WidgetName.IsEmpty())
	{
		OutError = TEXT("widget_name cannot be empty");
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	// Parse requested fields (default to "*" if not specified)
	TArray<FString> RequestedFields;
	if (Params->HasField(TEXT("fields")))
	{
		const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
		if (Params->TryGetArrayField(TEXT("fields"), FieldsArray))
		{
			for (const TSharedPtr<FJsonValue>& FieldValue : *FieldsArray)
			{
				FString FieldStr;
				if (FieldValue->TryGetString(FieldStr))
				{
					RequestedFields.Add(FieldStr.ToLower());
				}
			}
		}
	}

	// If no fields specified, default to all
	if (RequestedFields.Num() == 0)
	{
		RequestedFields.Add(TEXT("*"));
	}

	// Optional container name for dimensions
	FString ContainerName = TEXT("CanvasPanel_0");
	if (Params->HasField(TEXT("container_name")))
	{
		ContainerName = Params->GetStringField(TEXT("container_name"));
	}

	// Find the widget blueprint with retry mechanism
	TArray<FString> AttemptedPaths;
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprintWithRetry(WidgetName, AttemptedPaths);

	if (!WidgetBlueprint)
	{
		FString AttemptedPathsStr = FString::Join(AttemptedPaths, TEXT(", "));
		return CreateErrorResponse(FString::Printf(TEXT("Widget blueprint '%s' not found. Tried paths: [%s]. Note: If the asset exists but this error persists, the asset may not be fully loaded in the editor - try saving all assets and retrying."), *WidgetName, *AttemptedPathsStr));
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget blueprint '%s' has no widget tree"), *WidgetName));
	}

	// Build metadata response
	TSharedPtr<FJsonObject> MetadataObj = MakeShared<FJsonObject>();
	MetadataObj->SetStringField(TEXT("widget_name"), WidgetName);
	MetadataObj->SetStringField(TEXT("asset_path"), WidgetBlueprint->GetPathName());

	// Parent class info
	if (WidgetBlueprint->ParentClass)
	{
		MetadataObj->SetStringField(TEXT("parent_class"), WidgetBlueprint->ParentClass->GetName());
	}

	// Build requested sections using MetadataBuilder
	if (ShouldIncludeField(RequestedFields, TEXT("components")))
	{
		TSharedPtr<FJsonObject> ComponentsInfo = MetadataBuilder.BuildComponentsInfo(WidgetBlueprint);
		if (ComponentsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("components"), ComponentsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("layout")))
	{
		TSharedPtr<FJsonObject> LayoutInfo = MetadataBuilder.BuildLayoutInfo(WidgetBlueprint);
		if (LayoutInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("layout"), LayoutInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("dimensions")))
	{
		TSharedPtr<FJsonObject> DimensionsInfo = MetadataBuilder.BuildDimensionsInfo(WidgetBlueprint, ContainerName);
		if (DimensionsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("dimensions"), DimensionsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("hierarchy")))
	{
		TSharedPtr<FJsonObject> HierarchyInfo = MetadataBuilder.BuildHierarchyInfo(WidgetBlueprint);
		if (HierarchyInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("hierarchy"), HierarchyInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("bindings")))
	{
		TSharedPtr<FJsonObject> BindingsInfo = MetadataBuilder.BuildBindingsInfo(WidgetBlueprint);
		if (BindingsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("bindings"), BindingsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("events")))
	{
		TSharedPtr<FJsonObject> EventsInfo = MetadataBuilder.BuildEventsInfo(WidgetBlueprint);
		if (EventsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("events"), EventsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("variables")))
	{
		TSharedPtr<FJsonObject> VariablesInfo = MetadataBuilder.BuildVariablesInfo(WidgetBlueprint);
		if (VariablesInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("variables"), VariablesInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("functions")))
	{
		TSharedPtr<FJsonObject> FunctionsInfo = MetadataBuilder.BuildFunctionsInfo(WidgetBlueprint);
		if (FunctionsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("functions"), FunctionsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("orphaned_nodes")))
	{
		TSharedPtr<FJsonObject> OrphanedNodesInfo = MetadataBuilder.BuildOrphanedNodesInfo(WidgetBlueprint);
		if (OrphanedNodesInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("orphaned_nodes"), OrphanedNodesInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("graph_warnings")))
	{
		TSharedPtr<FJsonObject> WarningsInfo = MetadataBuilder.BuildGraphWarningsInfo(WidgetBlueprint);
		if (WarningsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("graph_warnings"), WarningsInfo);
		}
	}

	return CreateSuccessResponse(MetadataObj);
}

UWidgetBlueprint* FGetWidgetBlueprintMetadataCommand::FindWidgetBlueprintWithRetry(const FString& WidgetName, TArray<FString>& OutAttemptedPaths) const
{
	// Maximum number of retry attempts
	constexpr int32 MaxRetries = 2;
	// Delay between retries in seconds
	constexpr float RetryDelaySeconds = 0.1f;

	for (int32 Attempt = 0; Attempt <= MaxRetries; ++Attempt)
	{
		if (Attempt > 0)
		{
			UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Retry attempt %d/%d for widget '%s'"), Attempt, MaxRetries, *WidgetName);
			// Small delay to allow asset loading to complete
			FPlatformProcess::Sleep(RetryDelaySeconds);
		}

		UWidgetBlueprint* WidgetBlueprint = nullptr;

		// Check if full path provided
		if (WidgetName.StartsWith(TEXT("/Game/")) || WidgetName.StartsWith(TEXT("/Script/")))
		{
			// Try multiple path variations
			TArray<FString> PathVariations;

			if (WidgetName.Contains(TEXT(".")))
			{
				// Path already has asset suffix - try as-is first
				PathVariations.Add(WidgetName);
				// Also try without suffix
				int32 DotIndex;
				if (WidgetName.FindLastChar(TEXT('.'), DotIndex))
				{
					FString PathWithoutSuffix = WidgetName.Left(DotIndex);
					FString AssetName = FPaths::GetBaseFilename(PathWithoutSuffix);
					PathVariations.Add(PathWithoutSuffix + TEXT(".") + AssetName);
				}
			}
			else
			{
				// No suffix - add asset name suffix
				FString AssetName = FPaths::GetBaseFilename(WidgetName);
				PathVariations.Add(WidgetName + TEXT(".") + AssetName);
				PathVariations.Add(WidgetName);
			}

			for (const FString& Path : PathVariations)
			{
				if (Attempt == 0)
				{
					OutAttemptedPaths.Add(Path);
				}
				UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Attempting to load Widget Blueprint at path: '%s'"), *Path);
				UObject* Asset = UEditorAssetLibrary::LoadAsset(Path);
				if (Asset)
				{
					UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Loaded asset of type: %s"), *Asset->GetClass()->GetName());
					WidgetBlueprint = Cast<UWidgetBlueprint>(Asset);
					if (WidgetBlueprint)
					{
						UE_LOG(LogGetWidgetBlueprintMetadata, Log, TEXT("Successfully found Widget Blueprint at: '%s'"), *Path);
						return WidgetBlueprint;
					}
					else
					{
						UE_LOG(LogGetWidgetBlueprintMetadata, Warning, TEXT("Asset at path '%s' is not a WidgetBlueprint, it is: %s"), *Path, *Asset->GetClass()->GetName());
					}
				}
				else
				{
					UE_LOG(LogGetWidgetBlueprintMetadata, Warning, TEXT("Failed to load asset at path: '%s'"), *Path);
				}
			}
		}

		// If not found by path, search using asset registry
		if (!WidgetBlueprint)
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

			FARFilter Filter;
			Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
			Filter.PackagePaths.Add(FName(TEXT("/Game")));
			Filter.bRecursivePaths = true;

			TArray<FAssetData> AssetData;
			AssetRegistryModule.Get().GetAssets(Filter, AssetData);

			// Extract just the name for comparison
			FString SearchName = FPaths::GetBaseFilename(WidgetName);
			if (SearchName.IsEmpty())
			{
				SearchName = WidgetName;
			}

			for (const FAssetData& Asset : AssetData)
			{
				if (Asset.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
				{
					// Try multiple path formats
					FString ObjectPath = Asset.GetObjectPathString();
					FString SoftPath = Asset.GetSoftObjectPath().ToString();

					if (Attempt == 0)
					{
						OutAttemptedPaths.Add(FString::Printf(TEXT("AssetRegistry:%s"), *ObjectPath));
					}
					UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Found in asset registry: ObjectPath='%s', SoftPath='%s'"), *ObjectPath, *SoftPath);

					// Try loading with object path first
					UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(ObjectPath);
					if (!LoadedAsset)
					{
						// Try with soft path format
						LoadedAsset = UEditorAssetLibrary::LoadAsset(SoftPath);
					}

					if (LoadedAsset)
					{
						UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Loaded asset type: %s"), *LoadedAsset->GetClass()->GetName());
						WidgetBlueprint = Cast<UWidgetBlueprint>(LoadedAsset);
						if (WidgetBlueprint)
						{
							UE_LOG(LogGetWidgetBlueprintMetadata, Log, TEXT("Successfully loaded Widget Blueprint from registry: '%s'"), *ObjectPath);
							return WidgetBlueprint;
						}
						else
						{
							UE_LOG(LogGetWidgetBlueprintMetadata, Warning, TEXT("Asset loaded but is not a WidgetBlueprint, it is: %s"), *LoadedAsset->GetClass()->GetName());
						}
					}
					else
					{
						UE_LOG(LogGetWidgetBlueprintMetadata, Warning, TEXT("Failed to load asset from path: %s"), *ObjectPath);
					}
				}
			}
		}
	}

	// All retries exhausted
	UE_LOG(LogGetWidgetBlueprintMetadata, Error, TEXT("Failed to find Widget Blueprint '%s' after %d retries"), *WidgetName, MaxRetries);
	return nullptr;
}

bool FGetWidgetBlueprintMetadataCommand::ShouldIncludeField(const TArray<FString>& RequestedFields, const FString& FieldName) const
{
	if (RequestedFields.Contains(TEXT("*")))
	{
		return true;
	}
	return RequestedFields.Contains(FieldName.ToLower());
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& MetadataObj) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);

	// Copy all metadata fields to response
	if (MetadataObj.IsValid())
	{
		for (auto& Pair : MetadataObj->Values)
		{
			ResponseObj->SetField(Pair.Key, Pair.Value);
		}
	}

	return ResponseObj;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
	return ResponseObj;
}
