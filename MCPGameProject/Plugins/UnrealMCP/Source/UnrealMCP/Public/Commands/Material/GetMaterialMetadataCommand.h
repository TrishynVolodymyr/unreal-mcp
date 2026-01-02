#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for getting metadata about a material
 */
class UNREALMCP_API FGetMaterialMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetMaterialMetadataCommand(IMaterialService& InMaterialService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    bool ParseParameters(const FString& JsonString, FString& OutMaterialPath, TArray<FString>& OutFields, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
