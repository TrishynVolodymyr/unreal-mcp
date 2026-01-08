#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IMaterialService;

/**
 * Command for getting metadata and current parameter values from a Material Instance.
 */
class UNREALMCP_API FGetMaterialInstanceMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetMaterialInstanceMetadataCommand(IMaterialService& InMaterialService);
    virtual ~FGetMaterialInstanceMetadataCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_material_instance_metadata"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
