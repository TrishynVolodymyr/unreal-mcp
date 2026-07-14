#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Factories/FbxMeshImportData.h"

class UFbxImportUI;
class UFbxStaticMeshImportData;
class UStaticMesh;

struct FImportStaticMeshSettings
{
	bool bImportMaterials = false;
	bool bAutoGenerateCollision = true;
	EVertexColorImportOption::Type VertexColorImportOption = EVertexColorImportOption::Replace;
	FColor VertexOverrideColor = FColor::White;
	bool bHasVertexOverrideColor = false;
};

/**
 * Command to import a static mesh file (FBX, OBJ) from disk into the Unreal project.
 * Self-contained — uses UAssetImportTask + AssetTools directly, no service layer needed.
 */
class UNREALMCP_API FImportStaticMeshCommand : public IUnrealMCPCommand
{
public:
	using FExecutionOverride = TFunction<FString(const FImportStaticMeshSettings&, const UFbxImportUI&)>;
	explicit FImportStaticMeshCommand(FExecutionOverride InExecutionOverride = {})
		: ExecutionOverride(MoveTemp(InExecutionOverride))
	{
	}

	//~ IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
	//~ End IUnrealMCPCommand interface

#if WITH_DEV_AUTOMATION_TESTS
	bool ParseParametersForTest(
		const FString& JsonString,
		FImportStaticMeshSettings& OutSettings,
		FString& OutError) const;
	static void ConfigureImportUIForTest(UFbxImportUI& ImportUI, const FImportStaticMeshSettings& Settings);
	static UFbxStaticMeshImportData* PrepareExistingMeshForReimportForTest(
		UStaticMesh& ExistingMesh,
		UFbxImportUI& ImportUI,
		const FImportStaticMeshSettings& Settings,
		const FString& SourceFilePath);
#endif

private:
	FExecutionOverride ExecutionOverride;

	bool ParseParameters(
		const FString& JsonString,
		FString& OutSourceFilePath,
		FString& OutAssetName,
		FString& OutFolderPath,
		FImportStaticMeshSettings& OutSettings,
		FString& OutError) const;
	static void ConfigureImportUI(UFbxImportUI& ImportUI, const FImportStaticMeshSettings& Settings);
	static UFbxStaticMeshImportData* PrepareExistingMeshForReimport(
		UStaticMesh& ExistingMesh,
		UFbxImportUI& ImportUI,
		const FImportStaticMeshSettings& Settings,
		const FString& SourceFilePath);
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
