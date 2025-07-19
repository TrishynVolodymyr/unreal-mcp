#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for getting project directory
 */
class UNREALMCP_API FGetProjectDirCommand : public IUnrealMCPCommand
{
public:
    FGetProjectDirCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FGetProjectDirCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_project_dir"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
