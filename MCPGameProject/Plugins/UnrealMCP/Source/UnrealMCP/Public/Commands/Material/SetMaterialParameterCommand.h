#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for setting material parameters (scalar, vector, texture)
 */
class UNREALMCP_API FSetMaterialParameterCommand : public IUnrealMCPCommand
{
public:
    explicit FSetMaterialParameterCommand(IMaterialService& InMaterialService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    struct FParameterSetRequest
    {
        FString MaterialPath;
        FString ParameterName;
        FString ParameterType;
        TSharedPtr<FJsonValue> Value;
    };

    bool ParseParameters(const FString& JsonString, FParameterSetRequest& OutRequest, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ParameterName, const FString& ParameterType) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
