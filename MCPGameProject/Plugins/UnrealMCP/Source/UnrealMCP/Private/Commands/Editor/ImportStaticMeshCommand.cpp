#include "Commands/Editor/ImportStaticMeshCommand.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/StaticMesh.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"

FString FImportStaticMeshCommand::GetCommandName() const
{
	return TEXT("import_static_mesh");
}

bool FImportStaticMeshCommand::ValidateParams(const FString& Parameters) const
{
	FString SourceFilePath, AssetName, FolderPath, Error;
	bool bImportMaterials;
	return ParseParameters(Parameters, SourceFilePath, AssetName, FolderPath, bImportMaterials, Error);
}

FString FImportStaticMeshCommand::Execute(const FString& Parameters)
{
	FString SourceFilePath, AssetName, FolderPath, Error;
	bool bImportMaterials;
	if (!ParseParameters(Parameters, SourceFilePath, AssetName, FolderPath, bImportMaterials, Error))
	{
		return CreateErrorResponse(Error);
	}

	// Validate source file exists
	if (!FPaths::FileExists(SourceFilePath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Source file does not exist: %s"), *SourceFilePath));
	}

	// Validate file extension
	FString Extension = FPaths::GetExtension(SourceFilePath).ToLower();
	if (Extension != TEXT("fbx") && Extension != TEXT("obj"))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Unsupported mesh format: %s. Supported: fbx, obj"), *Extension));
	}

	// Normalize destination path
	FString DestinationPath = FolderPath;
	if (!DestinationPath.StartsWith(TEXT("/Game")))
	{
		if (DestinationPath.StartsWith(TEXT("/")))
		{
			DestinationPath = TEXT("/Game") + DestinationPath;
		}
		else
		{
			DestinationPath = TEXT("/Game/") + DestinationPath;
		}
	}

	// Create package for the asset
	FString PackagePath = DestinationPath / AssetName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}
	Package->FullyLoad();

	// Use FbxFactory directly — avoids TaskGraph recursion from ImportAssetTasks
	UFbxFactory* FbxFactory = NewObject<UFbxFactory>();
	FbxFactory->AddToRoot();

	// Configure for automated import
	FbxFactory->ImportUI->bIsObjImport = (Extension == TEXT("obj"));
	FbxFactory->ImportUI->bImportMesh = true;
	FbxFactory->ImportUI->bImportMaterials = bImportMaterials;
	FbxFactory->ImportUI->bImportTextures = bImportMaterials;
	FbxFactory->ImportUI->bImportAnimations = false;
	FbxFactory->ImportUI->bAutomatedImportShouldDetectType = false;
	FbxFactory->ImportUI->MeshTypeToImport = FBXIT_StaticMesh;
	FbxFactory->ImportUI->bOverrideFullName = true;

	// Static mesh specific settings
	FbxFactory->ImportUI->StaticMeshImportData->bCombineMeshes = true;
	FbxFactory->ImportUI->StaticMeshImportData->bAutoGenerateCollision = true;

	bool bCancelled = false;
	UObject* ImportedObject = FbxFactory->ImportObject(
		UStaticMesh::StaticClass(),
		Package,
		FName(*AssetName),
		RF_Public | RF_Standalone,
		SourceFilePath,
		nullptr,
		bCancelled
	);

	FbxFactory->RemoveFromRoot();

	if (!ImportedObject || bCancelled)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to import mesh file: %s"), *SourceFilePath));
	}

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(ImportedObject);
	Package->MarkPackageDirty();

	// Get mesh stats
	UStaticMesh* ImportedMesh = Cast<UStaticMesh>(ImportedObject);
	int32 VertexCount = 0;
	int32 TriangleCount = 0;

	if (ImportedMesh)
	{
		// Don't call Build() here — it triggers TaskGraph recursion when called from MCP thread.
		// UE will build the mesh lazily when needed (e.g., on save or viewport display).
		// Try to read stats if render data already exists.
		if (ImportedMesh->GetRenderData() && ImportedMesh->GetRenderData()->LODResources.Num() > 0)
		{
			const FStaticMeshLODResources& LOD0 = ImportedMesh->GetRenderData()->LODResources[0];
			VertexCount = LOD0.GetNumVertices();
			TriangleCount = LOD0.GetNumTriangles();
		}

		UE_LOG(LogTemp, Log, TEXT("Imported static mesh '%s' from '%s' (Verts: %d, Tris: %d)"),
			*PackagePath, *SourceFilePath, VertexCount, TriangleCount);
	}

	return CreateSuccessResponse(PackagePath, AssetName, VertexCount, TriangleCount);
}

bool FImportStaticMeshCommand::ParseParameters(
	const FString& JsonString,
	FString& OutSourceFilePath,
	FString& OutAssetName,
	FString& OutFolderPath,
	bool& OutImportMaterials,
	FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Failed to parse JSON parameters");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("source_file_path"), OutSourceFilePath) || OutSourceFilePath.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: source_file_path");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("asset_name"), OutAssetName) || OutAssetName.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: asset_name");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath) || OutFolderPath.IsEmpty())
	{
		OutFolderPath = TEXT("/Game/Meshes");
	}

	if (!JsonObject->TryGetBoolField(TEXT("import_materials"), OutImportMaterials))
	{
		OutImportMaterials = false;
	}

	return true;
}

FString FImportStaticMeshCommand::CreateSuccessResponse(const FString& AssetPath, const FString& AssetName, int32 VertexCount, int32 TriangleCount) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("path"), AssetPath);
	Response->SetStringField(TEXT("name"), AssetName);
	Response->SetNumberField(TEXT("vertex_count"), VertexCount);
	Response->SetNumberField(TEXT("triangle_count"), TriangleCount);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully imported mesh as: %s (Verts: %d, Tris: %d)"), *AssetPath, VertexCount, TriangleCount));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

FString FImportStaticMeshCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}
