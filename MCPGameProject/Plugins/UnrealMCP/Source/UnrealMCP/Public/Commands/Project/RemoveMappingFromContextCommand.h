#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to remove key mappings from Input Mapping Context assets.
 *
 * Two modes:
 *  - precise: action_path + key -> removes that exact (action, key) pair
 *  - bulk (set remove_all_keys=true): action_path only -> removes ALL keys
 *    bound to that action in this context
 *
 * Mirror of FAddMappingToContextCommand.
 */
class UNREALMCP_API FRemoveMappingFromContextCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveMappingFromContextCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FRemoveMappingFromContextCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
