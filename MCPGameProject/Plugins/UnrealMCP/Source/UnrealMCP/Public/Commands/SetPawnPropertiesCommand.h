#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to set pawn properties on a Blueprint
 */
class UNREALMCP_API FSetPawnPropertiesCommand : public IUnrealMCPCommand
{
public:
    explicit FSetPawnPropertiesCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                        TMap<FString, FString>& OutPawnParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& BlueprintName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
