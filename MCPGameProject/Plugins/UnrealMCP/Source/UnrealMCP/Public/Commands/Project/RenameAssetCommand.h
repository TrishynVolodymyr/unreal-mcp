#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for renaming assets
 */
class UNREALMCP_API FRenameAssetCommand : public IUnrealMCPCommand
{
public:
    FRenameAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FRenameAssetCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("rename_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
