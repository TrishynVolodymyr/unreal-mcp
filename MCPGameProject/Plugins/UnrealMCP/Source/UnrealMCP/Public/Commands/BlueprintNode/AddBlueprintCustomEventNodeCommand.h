#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

class UNREALMCP_API FAddBlueprintCustomEventNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FAddBlueprintCustomEventNodeCommand(IBlueprintNodeService& InBlueprintNodeService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintNodeService& BlueprintNodeService;
    
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutEventName, FVector2D& OutPosition, FString& OutError) const;
    FString CreateSuccessResponse(const FString& NodeId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};