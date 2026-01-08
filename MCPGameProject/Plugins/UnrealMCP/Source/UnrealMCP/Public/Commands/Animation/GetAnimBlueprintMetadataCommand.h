#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for retrieving Animation Blueprint metadata
 */
class UNREALMCP_API FGetAnimBlueprintMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetAnimBlueprintMetadataCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
