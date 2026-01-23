#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for retrieving metadata from a StateTree
 */
class UNREALMCP_API FGetStateTreeMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetStateTreeMetadataCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
