#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to list Enhanced Input Action assets
 */
class UNREALMCP_API FListInputActionsCommand : public IUnrealMCPCommand
{
public:
    explicit FListInputActionsCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FListInputActionsCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};