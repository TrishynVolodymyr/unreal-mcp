#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to set the linked asset for a LinkedAsset state type
 */
class UNREALMCP_API FSetLinkedStateAssetCommand : public IUnrealMCPCommand
{
public:
    FSetLinkedStateAssetCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
