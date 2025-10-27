#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for creating input mappings
 */
class UNREALMCP_API FCreateInputMappingCommand : public IUnrealMCPCommand
{
public:
    FCreateInputMappingCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FCreateInputMappingCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("create_input_mapping"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
