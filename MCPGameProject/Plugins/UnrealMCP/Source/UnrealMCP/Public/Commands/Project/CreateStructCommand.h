#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for creating structs
 */
class UNREALMCP_API FCreateStructCommand : public IUnrealMCPCommand
{
public:
    FCreateStructCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateStructCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("create_struct"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
