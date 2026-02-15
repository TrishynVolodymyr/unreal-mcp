#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for configuring mesh entries on a PCG Static Mesh Spawner node.
 * Reaches into the nested MeshSelectorParameters → MeshEntries that
 * set_pcg_node_property cannot access.
 */
class UNREALMCP_API FConfigurePCGMeshSpawnerCommand : public IUnrealMCPCommand
{
public:
	FConfigurePCGMeshSpawnerCommand();

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
