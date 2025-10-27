#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to create Enhanced Input Action assets
 */
class UNREALMCP_API FCreateEnhancedInputActionCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateEnhancedInputActionCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateEnhancedInputActionCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};