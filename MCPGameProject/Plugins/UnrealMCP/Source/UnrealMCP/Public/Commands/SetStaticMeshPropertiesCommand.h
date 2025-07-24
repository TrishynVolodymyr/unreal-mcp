#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to set static mesh properties on a component in a Blueprint
 */
class UNREALMCP_API FSetStaticMeshPropertiesCommand : public IUnrealMCPCommand
{
public:
    explicit FSetStaticMeshPropertiesCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName,
                        FString& OutStaticMeshPath, FString& OutError) const;
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
