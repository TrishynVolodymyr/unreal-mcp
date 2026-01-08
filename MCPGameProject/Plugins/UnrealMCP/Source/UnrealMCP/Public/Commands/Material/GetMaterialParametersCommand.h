#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for getting all available parameters from a Material (parent material or instance).
 */
class UNREALMCP_API FGetMaterialParametersCommand : public IUnrealMCPCommand
{
public:
    FGetMaterialParametersCommand(IMaterialService& InMaterialService);
    virtual ~FGetMaterialParametersCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_material_parameters"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
