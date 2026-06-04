#include "Commands/Editor/ImportStaticMeshCommand.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/StaticMesh.h"
#include "AssetImportTask.h"
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

	// Import via UFbxFactory::ImportObject directly. This is the SAME proven call the command used before
	// (it does NOT hang) — but with bCombineMeshes=false it SPLITS a multi-mesh "pack" FBX into one asset
	// per mesh, named after its FBX node, at the FBX's own scale: i.e. what the editor's default Import
	// dialog produces. (The old code forced bCombineMeshes=true + a single overridden name → every prop
	// merged into one mis-scaled mesh.) NOTE: ImportAssetTasks() was tried and SPINS/hangs from the MCP
	// FTSTicker context (verified 2026-06-03, 120s bridge timeout) — do NOT reintroduce it here.
	// DestinationPath is the FOLDER; the split meshes land in it under their FBX-node names.
	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Snapshot the folder's existing static meshes so we can isolate the newly-imported ones (the
	// factory creates one package PER mesh node, so the single ImportObject return value is not enough).
	TSet<FString> PreExisting;
	{
		TArray<FAssetData> Existing;
		AssetRegistryModule.Get().GetAssetsByPath(FName(*DestinationPath), Existing, /*bRecursive*/ false);
		for (const FAssetData& A : Existing) { PreExisting.Add(A.GetObjectPathString()); }
	}

	// Anchor package (ImportObject needs an InParent). With bOverrideFullName=false the factory names the
	// meshes by FBX node; this package is just the import anchor in the target folder.
	UPackage* AnchorPackage = CreatePackage(*(DestinationPath / AssetName));
	if (!AnchorPackage)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package in: %s"), *DestinationPath));
	}
	AnchorPackage->FullyLoad();

	UFbxFactory* FbxFactory = NewObject<UFbxFactory>();
	FbxFactory->AddToRoot();
	FbxFactory->ImportUI->bIsObjImport = (Extension == TEXT("obj"));
	FbxFactory->ImportUI->bImportMesh = true;
	FbxFactory->ImportUI->bImportAsSkeletal = false;
	FbxFactory->ImportUI->MeshTypeToImport = FBXIT_StaticMesh;
	FbxFactory->ImportUI->bImportMaterials = bImportMaterials;
	FbxFactory->ImportUI->bImportTextures = bImportMaterials;
	FbxFactory->ImportUI->bImportAnimations = false;
	FbxFactory->ImportUI->bAutomatedImportShouldDetectType = false;
	FbxFactory->ImportUI->bOverrideFullName = false;                       // per-node FBX names
	FbxFactory->ImportUI->StaticMeshImportData->bCombineMeshes = false;    // SPLIT — match the dialog default
	FbxFactory->ImportUI->StaticMeshImportData->bConvertSceneUnit = true;  // FBX scene unit (m) → UE cm — correct size
	FbxFactory->ImportUI->StaticMeshImportData->bAutoGenerateCollision = true;

	// THE SCALE FIX (UE 5.7, NOT Interchange): ImportObject uses the LEGACY libfbx path, but it only copies
	// the StaticMeshImportData scale/unit options into the live FBXImportOptions when the import is AUTOMATED
	// (UFactory::IsAutomatedImport) or a modal dialog is shown. With neither, FbxMainImport::GetImportOptions
	// takes the "else" branch and bConvertSceneUnit / ImportUniformScale silently NO-OP — which is why earlier
	// imports were 100× too small (bCombineMeshes survived only because it is read straight off ImportUI, not
	// off FBXImportOptions). An automated UAssetImportTask flips that gate AND suppresses the dialog; the
	// factory also adopts Task->Options as its UFbxImportUI. (Headless automation tests would NOT catch the
	// scale bug — GIsAutomationTesting flips the same gate, so scale applies there even without the task.)
	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	ImportTask->AddToRoot();
	ImportTask->bAutomated = true;
	ImportTask->bAsync = false;
	ImportTask->Options = FbxFactory->ImportUI;   // same configured UFbxImportUI the factory will use
	FbxFactory->SetAssetImportTask(ImportTask);

	bool bCancelled = false;
	UObject* ImportedObject = FbxFactory->ImportObject(
		UStaticMesh::StaticClass(), AnchorPackage, FName(*AssetName),
		RF_Public | RF_Standalone, SourceFilePath, nullptr, bCancelled);
	FbxFactory->RemoveFromRoot();
	ImportTask->RemoveFromRoot();

	if (bCancelled)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Import was cancelled: %s"), *SourceFilePath));
	}

	// Collect every static mesh now in the folder that was not there before (handles the multi-asset split).
	TArray<UStaticMesh*> ImportedMeshes;
	{
		TArray<FAssetData> After;
		AssetRegistryModule.Get().GetAssetsByPath(FName(*DestinationPath), After, /*bRecursive*/ false);
		for (const FAssetData& A : After)
		{
			if (PreExisting.Contains(A.GetObjectPathString())) { continue; }
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(A.GetAsset())) { ImportedMeshes.AddUnique(Mesh); }
		}
	}
	if (UStaticMesh* Direct = Cast<UStaticMesh>(ImportedObject)) { ImportedMeshes.AddUnique(Direct); }

	if (ImportedMeshes.Num() == 0)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Import produced no static meshes from: %s"), *SourceFilePath));
	}

	// Backward-compat: a SINGLE-mesh FBX renames its one asset to the requested asset_name (the old
	// one-asset contract). A multi-mesh "pack" keeps the per-node FBX names (asset_name is not a single
	// target then — documented on the tool). Best-effort: a rename failure does not fail the import.
	if (ImportedMeshes.Num() == 1 && !AssetName.IsEmpty() && ImportedMeshes[0]->GetName() != AssetName)
	{
		TArray<FAssetRenameData> Renames;
		Renames.Emplace(ImportedMeshes[0], DestinationPath, AssetName);
		AssetToolsModule.Get().RenameAssets(Renames);
	}

	// Build the response: one entry per created static mesh (+ a top-level mirror of the first for
	// backward-compat with callers that read response["path"]/["name"]).
	TArray<TSharedPtr<FJsonValue>> MeshEntries;
	FString FirstPath, FirstName;
	int32 FirstVerts = 0, FirstTris = 0;
	for (UStaticMesh* Mesh : ImportedMeshes)
	{
		FAssetRegistryModule::AssetCreated(Mesh);
		Mesh->MarkPackageDirty();

		// Don't Build() here (TaskGraph recursion risk from the MCP tick); read stats if already built.
		int32 V = 0, T = 0;
		if (Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() > 0)
		{
			V = Mesh->GetRenderData()->LODResources[0].GetNumVertices();
			T = Mesh->GetRenderData()->LODResources[0].GetNumTriangles();
		}
		const FString MeshPkgPath = Mesh->GetOutermost()->GetName();

		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Mesh->GetName());
		Entry->SetStringField(TEXT("path"), MeshPkgPath);
		Entry->SetNumberField(TEXT("vertex_count"), V);
		Entry->SetNumberField(TEXT("triangle_count"), T);
		MeshEntries.Add(MakeShared<FJsonValueObject>(Entry));

		if (FirstPath.IsEmpty()) { FirstPath = MeshPkgPath; FirstName = Mesh->GetName(); FirstVerts = V; FirstTris = T; }

		UE_LOG(LogTemp, Log, TEXT("Imported static mesh '%s' (Verts: %d, Tris: %d)"), *MeshPkgPath, V, T);
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetNumberField(TEXT("count"), MeshEntries.Num());
	Response->SetStringField(TEXT("folder"), DestinationPath);
	Response->SetArrayField(TEXT("meshes"), MeshEntries);
	Response->SetStringField(TEXT("path"), FirstPath);
	Response->SetStringField(TEXT("name"), FirstName);
	Response->SetNumberField(TEXT("vertex_count"), FirstVerts);
	Response->SetNumberField(TEXT("triangle_count"), FirstTris);
	Response->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Imported %d static mesh(es) into %s"), MeshEntries.Num(), *DestinationPath));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
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
