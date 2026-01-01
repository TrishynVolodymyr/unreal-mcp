#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for creating Material Instance assets (static or dynamic)
 */
class UNREALMCP_API FCreateMaterialInstanceCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateMaterialInstanceCommand(IMaterialService& InMaterialService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    bool ParseParameters(const FString& JsonString, FMaterialInstanceCreationParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& InstancePath, bool bIsDynamic, const FString& ParentPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
