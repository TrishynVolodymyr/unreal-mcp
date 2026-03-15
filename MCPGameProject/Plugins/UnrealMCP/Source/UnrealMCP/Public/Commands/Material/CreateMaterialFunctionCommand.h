#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for creating new MaterialFunction assets.
 * MaterialFunctions are reusable node graphs that can be called from materials.
 */
class UNREALMCP_API FCreateMaterialFunctionCommand : public IUnrealMCPCommand
{
public:
	FCreateMaterialFunctionCommand() = default;

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	struct FParams
	{
		FString Name;
		FString Path = TEXT("/Game/Materials/Functions");
		FString Description;
		bool bExposeToLibrary = true;
		TArray<FString> LibraryCategories;

		bool IsValid(FString& OutError) const
		{
			if (Name.IsEmpty()) { OutError = TEXT("Missing 'name' parameter"); return false; }
			return true;
		}
	};

	bool ParseParameters(const FString& JsonString, FParams& OutParams, FString& OutError) const;
	FString CreateSuccessResponse(const FString& FunctionPath) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
