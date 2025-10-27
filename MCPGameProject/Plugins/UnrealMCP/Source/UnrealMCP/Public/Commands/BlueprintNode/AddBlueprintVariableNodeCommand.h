#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command for adding variable nodes to Blueprints
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FAddBlueprintVariableNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FAddBlueprintVariableNodeCommand(IBlueprintNodeService& InBlueprintNodeService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintNodeService& BlueprintNodeService;
    
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutVariableName, bool& OutIsGetter, FVector2D& OutPosition, FString& OutError) const;
    FString CreateSuccessResponse(const FString& NodeId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};