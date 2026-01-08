#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for configuring animation slots in Animation Blueprints
 */
class UNREALMCP_API FConfigureAnimSlotCommand : public IUnrealMCPCommand
{
public:
    explicit FConfigureAnimSlotCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateSuccessResponse(const FString& SlotName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
