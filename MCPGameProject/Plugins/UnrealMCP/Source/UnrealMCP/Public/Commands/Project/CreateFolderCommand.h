#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for creating folders
 */
class UNREALMCP_API FCreateFolderCommand : public IUnrealMCPCommand
{
public:
    FCreateFolderCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateFolderCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("create_folder"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
