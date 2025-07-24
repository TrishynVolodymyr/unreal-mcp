#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to show variables and types of a struct
 */
class UNREALMCP_API FShowStructVariablesCommand : public IUnrealMCPCommand
{
public:
    explicit FShowStructVariablesCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FShowStructVariablesCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};