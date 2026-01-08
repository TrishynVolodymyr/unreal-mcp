#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for linking animation layers to an Animation Blueprint
 */
class UNREALMCP_API FLinkAnimationLayerCommand : public IUnrealMCPCommand
{
public:
    explicit FLinkAnimationLayerCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateSuccessResponse(const FString& LayerName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
