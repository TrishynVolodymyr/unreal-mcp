#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

class UNREALMCP_API FGetVariableInfoCommand : public IUnrealMCPCommand
{
public:
    explicit FGetVariableInfoCommand(IBlueprintNodeService& InBlueprintNodeService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintNodeService& BlueprintNodeService;
    
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutVariableName, FString& OutError) const;
    FString CreateSuccessResponse(const FString& VariableType, const TSharedPtr<FJsonObject>& AdditionalInfo) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};