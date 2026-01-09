#include "Commands/Project/SearchAssetsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Sound/SoundWave.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "Engine/DataTable.h"
#include "Animation/AnimSequence.h"

bool FSearchAssetsCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	// At least one search parameter is required: search_query, pattern, asset_type, asset_class, path, or folder
	FString SearchQuery, Pattern, AssetType, AssetClass, Path, Folder;
	bool bHasSearchQuery = JsonObject->TryGetStringField(TEXT("search_query"), SearchQuery) && !SearchQuery.IsEmpty();
	bool bHasPattern = JsonObject->TryGetStringField(TEXT("pattern"), Pattern) && !Pattern.IsEmpty();
	bool bHasAssetType = JsonObject->TryGetStringField(TEXT("asset_type"), AssetType) && !AssetType.IsEmpty();
	bool bHasAssetClass = JsonObject->TryGetStringField(TEXT("asset_class"), AssetClass) && !AssetClass.IsEmpty();
	bool bHasPath = JsonObject->TryGetStringField(TEXT("path"), Path) && !Path.IsEmpty();
	bool bHasFolder = JsonObject->TryGetStringField(TEXT("folder"), Folder) && !Folder.IsEmpty();

	return bHasSearchQuery || bHasPattern || bHasAssetType || bHasAssetClass || bHasPath || bHasFolder;
}

FString FSearchAssetsCommand::Execute(const FString& Parameters)
{
	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Validate parameters
	if (!ValidateParams(Parameters))
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("At least one parameter required. Accepts: search_query/pattern (string), asset_type/asset_class (string), path/folder (string), max_results (int). Valid asset_type values: Texture, Material, MaterialInstance, StaticMesh, SkeletalMesh, Sound, Blueprint, WidgetBlueprint, DataTable, Animation, NiagaraSystem"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Extract parameters - accept both naming conventions
	FString SearchQuery;
	if (!JsonObject->TryGetStringField(TEXT("search_query"), SearchQuery))
	{
		JsonObject->TryGetStringField(TEXT("pattern"), SearchQuery);
	}

	FString AssetType;
	if (!JsonObject->TryGetStringField(TEXT("asset_type"), AssetType))
	{
		JsonObject->TryGetStringField(TEXT("asset_class"), AssetType);
	}

	FString SearchPath = TEXT("/Game");
	if (!JsonObject->TryGetStringField(TEXT("path"), SearchPath))
	{
		JsonObject->TryGetStringField(TEXT("folder"), SearchPath);
	}

	int32 MaxResults = 50;
	JsonObject->TryGetNumberField(TEXT("max_results"), MaxResults);
	MaxResults = FMath::Clamp(MaxResults, 1, 500);

	// Get asset registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Build filter
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*SearchPath));
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	// Add class filter if asset_type specified
	if (!AssetType.IsEmpty())
	{
		UClass* FilterClass = nullptr;

		// Map common type names to UClass
		if (AssetType.Equals(TEXT("Texture2D"), ESearchCase::IgnoreCase) ||
			AssetType.Equals(TEXT("Texture"), ESearchCase::IgnoreCase))
		{
			FilterClass = UTexture2D::StaticClass();
		}
		else if (AssetType.Equals(TEXT("Material"), ESearchCase::IgnoreCase))
		{
			FilterClass = UMaterial::StaticClass();
		}
		else if (AssetType.Equals(TEXT("MaterialInstance"), ESearchCase::IgnoreCase))
		{
			FilterClass = UMaterialInstance::StaticClass();
		}
		else if (AssetType.Equals(TEXT("StaticMesh"), ESearchCase::IgnoreCase))
		{
			FilterClass = UStaticMesh::StaticClass();
		}
		else if (AssetType.Equals(TEXT("SkeletalMesh"), ESearchCase::IgnoreCase))
		{
			FilterClass = USkeletalMesh::StaticClass();
		}
		else if (AssetType.Equals(TEXT("SoundWave"), ESearchCase::IgnoreCase) ||
				 AssetType.Equals(TEXT("Sound"), ESearchCase::IgnoreCase))
		{
			FilterClass = USoundWave::StaticClass();
		}
		else if (AssetType.Equals(TEXT("Blueprint"), ESearchCase::IgnoreCase))
		{
			FilterClass = UBlueprint::StaticClass();
		}
		else if (AssetType.Equals(TEXT("WidgetBlueprint"), ESearchCase::IgnoreCase) ||
				 AssetType.Equals(TEXT("Widget"), ESearchCase::IgnoreCase))
		{
			FilterClass = UWidgetBlueprint::StaticClass();
		}
		else if (AssetType.Equals(TEXT("DataTable"), ESearchCase::IgnoreCase))
		{
			FilterClass = UDataTable::StaticClass();
		}
		else if (AssetType.Equals(TEXT("AnimSequence"), ESearchCase::IgnoreCase) ||
				 AssetType.Equals(TEXT("Animation"), ESearchCase::IgnoreCase))
		{
			FilterClass = UAnimSequence::StaticClass();
		}
		else if (AssetType.Equals(TEXT("NiagaraSystem"), ESearchCase::IgnoreCase) ||
				 AssetType.Equals(TEXT("Niagara"), ESearchCase::IgnoreCase))
		{
			// Niagara is an optional plugin, try to find the class dynamically
			UClass* NiagaraClass = FindObject<UClass>(nullptr, TEXT("/Script/Niagara.NiagaraSystem"));
			if (NiagaraClass)
			{
				FilterClass = NiagaraClass;
			}
		}

		if (FilterClass)
		{
			Filter.ClassPaths.Add(FilterClass->GetClassPathName());
		}
		else
		{
			// Try to find class by name in script modules
			FString FullPath = FString::Printf(TEXT("/Script/Engine.%s"), *AssetType);
			UClass* FoundClass = FindObject<UClass>(nullptr, *FullPath);
			if (!FoundClass)
			{
				FullPath = FString::Printf(TEXT("/Script/CoreUObject.%s"), *AssetType);
				FoundClass = FindObject<UClass>(nullptr, *FullPath);
			}
			if (FoundClass)
			{
				Filter.ClassPaths.Add(FoundClass->GetClassPathName());
			}
		}
	}

	// Query asset registry
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	// Filter by name and build results
	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	int32 MatchCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (MatchCount >= MaxResults)
		{
			break;
		}

		FString AssetName = AssetData.AssetName.ToString();

		// Case-insensitive name match (contains)
		if (AssetName.Contains(SearchQuery, ESearchCase::IgnoreCase))
		{
			TSharedPtr<FJsonObject> AssetInfo = MakeShared<FJsonObject>();
			AssetInfo->SetStringField(TEXT("name"), AssetName);
			AssetInfo->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
			AssetInfo->SetStringField(TEXT("package_path"), AssetData.PackagePath.ToString());
			AssetInfo->SetStringField(TEXT("class_name"), AssetData.AssetClassPath.GetAssetName().ToString());

			AssetsArray.Add(MakeShared<FJsonValueObject>(AssetInfo));
			MatchCount++;
		}
	}

	// Create success response
	TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
	ResponseData->SetBoolField(TEXT("success"), true);
	ResponseData->SetStringField(TEXT("search_query"), SearchQuery);
	ResponseData->SetStringField(TEXT("asset_type"), AssetType.IsEmpty() ? TEXT("all") : AssetType);
	ResponseData->SetStringField(TEXT("path"), SearchPath);
	ResponseData->SetNumberField(TEXT("count"), AssetsArray.Num());
	ResponseData->SetNumberField(TEXT("total_scanned"), AssetDataList.Num());
	ResponseData->SetArrayField(TEXT("assets"), AssetsArray);

	if (AssetsArray.Num() >= MaxResults)
	{
		ResponseData->SetStringField(TEXT("note"), FString::Printf(TEXT("Results limited to %d. Use more specific query or increase max_results."), MaxResults));
	}

	// Convert response to JSON string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
	return OutputString;
}
