#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for deleting assets (Blueprints, Widgets, DataTables, Materials, etc.)
 */
class UNREALMCP_API FDeleteAssetCommand : public IUnrealMCPCommand
{
public:
    FDeleteAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FDeleteAssetCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("delete_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
