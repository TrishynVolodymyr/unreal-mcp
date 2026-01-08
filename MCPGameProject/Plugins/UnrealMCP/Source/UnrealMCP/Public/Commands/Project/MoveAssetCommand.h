#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for moving assets to different folders
 */
class UNREALMCP_API FMoveAssetCommand : public IUnrealMCPCommand
{
public:
    FMoveAssetCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FMoveAssetCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("move_asset"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
