#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for getting PCG Graph metadata including nodes, pins, and connections
 */
class UNREALMCP_API FGetPCGGraphMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetPCGGraphMetadataCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
