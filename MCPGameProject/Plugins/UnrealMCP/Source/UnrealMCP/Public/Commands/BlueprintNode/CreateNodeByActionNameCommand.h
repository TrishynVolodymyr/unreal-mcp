#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command for creating Blueprint nodes by action/function name using the Blueprint Action Database
 */
class UNREALMCP_API FCreateNodeByActionNameCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateNodeByActionNameCommand(IBlueprintNodeService& InBlueprintNodeService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintNodeService& BlueprintNodeService;

    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutFunctionName, 
                        FString& OutClassName, FString& OutNodePosition, FString& OutJsonParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& NodeId, const FString& NodeType, const FString& Message) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};