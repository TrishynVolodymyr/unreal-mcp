#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to search the PCG Palette for available PCG Settings node types.
 * Iterates UPCGSettings subclasses and returns their names, categories, and descriptions.
 */
class UNREALMCP_API FSearchPCGPaletteCommand : public IUnrealMCPCommand
{
public:
	FSearchPCGPaletteCommand();

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	FString CreateErrorResponse(const FString& ErrorMessage) const;
	FString CreateSuccessResponse(TSharedPtr<FJsonObject> ResultObj) const;
	static FString SettingsTypeToString(uint8 TypeValue);
};
