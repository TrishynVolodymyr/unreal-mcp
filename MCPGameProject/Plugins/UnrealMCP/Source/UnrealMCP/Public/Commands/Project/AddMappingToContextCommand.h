#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to add key mappings to Input Mapping Context assets
 */
class UNREALMCP_API FAddMappingToContextCommand : public IUnrealMCPCommand
{
public:
    explicit FAddMappingToContextCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FAddMappingToContextCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};