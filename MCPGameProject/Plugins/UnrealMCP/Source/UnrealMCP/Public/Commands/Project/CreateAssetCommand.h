#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for creating an asset of ANY UObject class (generic — for asset
 * types without a dedicated MCP tool, e.g. Voxel plugin assets).
 */
class UNREALMCP_API FCreateAssetCommand : public IUnrealMCPCommand
{
public:
    FCreateAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateAssetCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("create_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
