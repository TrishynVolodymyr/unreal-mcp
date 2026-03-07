#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to import a static mesh file (FBX, OBJ) from disk into the Unreal project.
 * Self-contained — uses UAssetImportTask + AssetTools directly, no service layer needed.
 */
class UNREALMCP_API FImportStaticMeshCommand : public IUnrealMCPCommand
{
public:
	FImportStaticMeshCommand() = default;

	//~ IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
	//~ End IUnrealMCPCommand interface

private:
	bool ParseParameters(
		const FString& JsonString,
		FString& OutSourceFilePath,
		FString& OutAssetName,
		FString& OutFolderPath,
		bool& OutImportMaterials,
		FString& OutError) const;

	FString CreateSuccessResponse(const FString& AssetPath, const FString& AssetName, int32 VertexCount, int32 TriangleCount) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
