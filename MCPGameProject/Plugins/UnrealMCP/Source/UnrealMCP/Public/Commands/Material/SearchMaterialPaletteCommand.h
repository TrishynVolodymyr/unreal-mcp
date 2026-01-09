#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to search the Material Palette for expressions and functions.
 * Uses UE's built-in MaterialExpressionClasses and Asset Registry.
 */
class UNREALMCP_API FSearchMaterialPaletteCommand : public IUnrealMCPCommand
{
public:
    FSearchMaterialPaletteCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateErrorResponse(const FString& ErrorMessage) const;
    FString CreateSuccessResponse(TSharedPtr<FJsonObject> ResultObj) const;
};
