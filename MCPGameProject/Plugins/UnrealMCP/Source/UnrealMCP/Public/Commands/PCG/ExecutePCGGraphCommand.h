#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for executing/regenerating a PCG Graph on an actor's PCG Component.
 * Finds an actor by name/label, locates its UPCGComponent, and triggers generation.
 */
class UNREALMCP_API FExecutePCGGraphCommand : public IUnrealMCPCommand
{
public:
    FExecutePCGGraphCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    bool ParseParameters(const FString& JsonString, FString& OutActorName, bool& OutForce, FString& OutError) const;
    FString CreateSuccessResponse(const FString& Message) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
