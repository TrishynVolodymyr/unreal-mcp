#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for setting a texture parameter on a Material Instance.
 */
class UNREALMCP_API FSetMaterialTextureParamCommand : public IUnrealMCPCommand
{
public:
    FSetMaterialTextureParamCommand(IMaterialService& InMaterialService);
    virtual ~FSetMaterialTextureParamCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_material_texture_param"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateSuccessResponse(const FString& MaterialPath, const FString& ParameterName, const FString& TexturePath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
