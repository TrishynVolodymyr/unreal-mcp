#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to list the contents of a folder
 */
class UNREALMCP_API FListFolderContentsCommand : public IUnrealMCPCommand
{
public:
    explicit FListFolderContentsCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FListFolderContentsCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};