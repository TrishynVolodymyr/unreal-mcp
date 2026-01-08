#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for setting a scalar (float) parameter on a Material Instance.
 */
class UNREALMCP_API FSetMaterialScalarParamCommand : public IUnrealMCPCommand
{
public:
    FSetMaterialScalarParamCommand(IMaterialService& InMaterialService);
    virtual ~FSetMaterialScalarParamCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_material_scalar_param"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateSuccessResponse(const FString& MaterialPath, const FString& ParameterName, float Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
