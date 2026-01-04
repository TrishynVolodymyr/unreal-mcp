#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for compiling a material and returning validation info including orphan detection
 */
class UNREALMCP_API FCompileMaterialCommand : public IUnrealMCPCommand
{
public:
    FCompileMaterialCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
