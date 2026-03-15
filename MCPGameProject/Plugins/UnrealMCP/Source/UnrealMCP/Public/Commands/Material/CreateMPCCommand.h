#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for creating Material Parameter Collection (MPC) assets.
 * MPCs provide global material parameters shared across all materials.
 */
class UNREALMCP_API FCreateMPCCommand : public IUnrealMCPCommand
{
public:
	FCreateMPCCommand() = default;

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	bool ParseAndExecute(const FString& JsonString, FString& OutResponse);
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
