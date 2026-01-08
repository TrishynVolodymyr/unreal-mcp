#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for creating DataAsset instances
 */
class UNREALMCP_API FCreateDataAssetCommand : public IUnrealMCPCommand
{
public:
    FCreateDataAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateDataAssetCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("create_data_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
