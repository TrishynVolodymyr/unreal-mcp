#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for connecting nodes in Animation Blueprint AnimGraphs
 * Typically used to connect state machine output to the root pose node
 */
class UNREALMCP_API FConnectAnimGraphNodesCommand : public IUnrealMCPCommand
{
public:
    explicit FConnectAnimGraphNodesCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateSuccessResponse(const FString& SourceNodeName, const FString& TargetNodeName, const FString& SourcePinName, const FString& TargetPinName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
