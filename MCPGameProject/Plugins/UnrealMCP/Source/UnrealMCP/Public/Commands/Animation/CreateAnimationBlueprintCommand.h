#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for creating Animation Blueprint assets
 */
class UNREALMCP_API FCreateAnimationBlueprintCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateAnimationBlueprintCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    bool ParseParameters(const FString& JsonString, FAnimBlueprintCreationParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(class UAnimBlueprint* AnimBlueprint) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
