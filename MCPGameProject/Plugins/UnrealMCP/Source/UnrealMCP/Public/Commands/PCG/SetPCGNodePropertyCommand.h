#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for setting a property on a PCG node's settings.
 * Uses FProperty::ImportText to handle any property type from a string value.
 */
class UNREALMCP_API FSetPCGNodePropertyCommand : public IUnrealMCPCommand
{
public:
    FSetPCGNodePropertyCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString GetPropertyTypeString(const FProperty* Property) const;
    FString CreateSuccessResponse(const FString& NodeId, const FString& PropertyName, const FString& Value, const FString& PropertyType) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
