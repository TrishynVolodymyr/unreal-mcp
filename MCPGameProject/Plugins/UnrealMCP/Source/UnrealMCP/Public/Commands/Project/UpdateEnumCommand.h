#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for updating user-defined enums
 */
class UNREALMCP_API FUpdateEnumCommand : public IUnrealMCPCommand
{
public:
    FUpdateEnumCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FUpdateEnumCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("update_enum"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
