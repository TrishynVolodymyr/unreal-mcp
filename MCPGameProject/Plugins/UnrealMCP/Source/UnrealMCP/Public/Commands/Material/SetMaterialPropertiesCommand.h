#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for setting properties on an existing Material (BlendMode, ShadingModel, usage flags)
 * This modifies the base Material, not Material Instances
 */
class UNREALMCP_API FSetMaterialPropertiesCommand : public IUnrealMCPCommand
{
public:
    FSetMaterialPropertiesCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateSuccessResponse(const FString& MaterialPath, const TArray<FString>& ChangedProperties) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
