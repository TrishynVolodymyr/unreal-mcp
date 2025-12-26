#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for duplicating assets (Blueprints, Widgets, DataTables, etc.)
 */
class UNREALMCP_API FDuplicateAssetCommand : public IUnrealMCPCommand
{
public:
    FDuplicateAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FDuplicateAssetCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("duplicate_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
