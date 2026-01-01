#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for getting material parameter values
 */
class UNREALMCP_API FGetMaterialParameterCommand : public IUnrealMCPCommand
{
public:
    explicit FGetMaterialParameterCommand(IMaterialService& InMaterialService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    struct FParameterGetRequest
    {
        FString MaterialPath;
        FString ParameterName;
        FString ParameterType;
    };

    bool ParseParameters(const FString& JsonString, FParameterGetRequest& OutRequest, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ParameterName, const FString& ParameterType, const TSharedPtr<FJsonValue>& Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
