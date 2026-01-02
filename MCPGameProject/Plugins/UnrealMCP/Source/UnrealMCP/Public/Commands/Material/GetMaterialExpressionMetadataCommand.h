#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for getting material expression graph metadata
 */
class UNREALMCP_API FGetMaterialExpressionMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetMaterialExpressionMetadataCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
