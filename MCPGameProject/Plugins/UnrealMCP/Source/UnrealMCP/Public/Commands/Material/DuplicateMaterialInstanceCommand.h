#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for duplicating a Material Instance to create a variation.
 */
class UNREALMCP_API FDuplicateMaterialInstanceCommand : public IUnrealMCPCommand
{
public:
    FDuplicateMaterialInstanceCommand(IMaterialService& InMaterialService);
    virtual ~FDuplicateMaterialInstanceCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("duplicate_material_instance"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateSuccessResponse(const FString& Name, const FString& Path, const FString& Parent) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
