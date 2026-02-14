#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for connecting two nodes in a PCG Graph via edge creation.
 */
class UNREALMCP_API FConnectPCGNodesCommand : public IUnrealMCPCommand
{
public:
    FConnectPCGNodesCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateSuccessResponse(const FString& Message) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
