#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to update existing struct assets
 */
class UNREALMCP_API FUpdateStructCommand : public IUnrealMCPCommand
{
public:
    explicit FUpdateStructCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FUpdateStructCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};