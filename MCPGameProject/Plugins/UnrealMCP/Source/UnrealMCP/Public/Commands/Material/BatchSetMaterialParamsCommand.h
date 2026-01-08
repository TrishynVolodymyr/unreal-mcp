#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for setting multiple parameters on a Material Instance in a single operation.
 */
class UNREALMCP_API FBatchSetMaterialParamsCommand : public IUnrealMCPCommand
{
public:
    FBatchSetMaterialParamsCommand(IMaterialService& InMaterialService);
    virtual ~FBatchSetMaterialParamsCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("batch_set_material_params"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateSuccessResponse(const FString& MaterialPath, const TArray<FString>& ScalarParams, const TArray<FString>& VectorParams, const TArray<FString>& TextureParams) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
