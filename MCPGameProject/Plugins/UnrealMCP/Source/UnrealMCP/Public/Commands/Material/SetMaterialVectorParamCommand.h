#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for setting a vector (color/4D vector) parameter on a Material Instance.
 */
class UNREALMCP_API FSetMaterialVectorParamCommand : public IUnrealMCPCommand
{
public:
    FSetMaterialVectorParamCommand(IMaterialService& InMaterialService);
    virtual ~FSetMaterialVectorParamCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_material_vector_param"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateSuccessResponse(const FString& MaterialPath, const FString& ParameterName, const FLinearColor& Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
