#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to list Input Mapping Context assets
 */
class UNREALMCP_API FListInputMappingContextsCommand : public IUnrealMCPCommand
{
public:
    explicit FListInputMappingContextsCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FListInputMappingContextsCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};