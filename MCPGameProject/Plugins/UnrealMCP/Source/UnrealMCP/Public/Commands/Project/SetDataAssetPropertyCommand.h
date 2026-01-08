#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for setting properties on DataAsset instances
 */
class UNREALMCP_API FSetDataAssetPropertyCommand : public IUnrealMCPCommand
{
public:
    FSetDataAssetPropertyCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FSetDataAssetPropertyCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_data_asset_property"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
