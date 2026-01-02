#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for creating new Material assets
 */
class UNREALMCP_API FCreateMaterialCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateMaterialCommand(IMaterialService& InMaterialService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    bool ParseParameters(const FString& JsonString, FMaterialCreationParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& MaterialPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
