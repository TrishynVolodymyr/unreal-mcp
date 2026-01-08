#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for getting metadata from DataAsset instances
 */
class UNREALMCP_API FGetDataAssetMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetDataAssetMetadataCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FGetDataAssetMetadataCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_data_asset_metadata"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
