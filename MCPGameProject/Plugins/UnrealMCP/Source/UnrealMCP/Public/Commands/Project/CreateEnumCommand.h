#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for creating user-defined enums
 */
class UNREALMCP_API FCreateEnumCommand : public IUnrealMCPCommand
{
public:
    FCreateEnumCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateEnumCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("create_enum"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
