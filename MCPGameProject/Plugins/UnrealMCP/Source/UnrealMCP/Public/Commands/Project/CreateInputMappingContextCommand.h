#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to create Input Mapping Context assets
 */
class UNREALMCP_API FCreateInputMappingContextCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateInputMappingContextCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateInputMappingContextCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};