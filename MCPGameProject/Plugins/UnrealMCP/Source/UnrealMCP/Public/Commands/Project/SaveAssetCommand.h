#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for saving an asset's package to disk (persist a dirty / in-memory asset).
 */
class UNREALMCP_API FSaveAssetCommand : public IUnrealMCPCommand
{
public:
    FSaveAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FSaveAssetCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("save_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
