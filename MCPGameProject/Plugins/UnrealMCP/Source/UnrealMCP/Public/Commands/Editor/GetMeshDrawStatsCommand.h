#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Per-mesh draw count breakdown per render pass.
 * Triggers r.MeshDrawCommands.DumpStats CSV and parses results.
 */
class UNREALMCP_API FGetMeshDrawStatsCommand : public IUnrealMCPCommand
{
public:
	FGetMeshDrawStatsCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
