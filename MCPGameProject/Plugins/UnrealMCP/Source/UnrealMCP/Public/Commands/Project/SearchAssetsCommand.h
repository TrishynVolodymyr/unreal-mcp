#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for searching assets by name and/or type.
 * Uses UE's Asset Registry for efficient querying.
 */
class UNREALMCP_API FSearchAssetsCommand : public IUnrealMCPCommand
{
public:
	FSearchAssetsCommand() = default;
	virtual ~FSearchAssetsCommand() = default;

	// IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override { return TEXT("search_assets"); }
	virtual bool ValidateParams(const FString& Parameters) const override;
};
