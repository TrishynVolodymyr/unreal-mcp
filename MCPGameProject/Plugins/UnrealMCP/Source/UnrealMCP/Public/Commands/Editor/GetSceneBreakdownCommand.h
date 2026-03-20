#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Diagnostic command: per-mesh breakdown of scene rendering cost.
 * Iterates all StaticMesh/ISM/HISM components, aggregates by mesh,
 * returns sorted by total triangle cost (instances * tris).
 */
class UNREALMCP_API FGetSceneBreakdownCommand : public IUnrealMCPCommand
{
public:
	FGetSceneBreakdownCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
