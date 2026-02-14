#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for creating new PCG Graph assets
 */
class UNREALMCP_API FCreatePCGGraphCommand : public IUnrealMCPCommand
{
public:
    FCreatePCGGraphCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    bool ParseParameters(const FString& JsonString, FString& OutName, FString& OutPath, FString& OutError) const;
    FString CreateSuccessResponse(const FString& GraphPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
