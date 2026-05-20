#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Create a new empty level (umap) at a given content path.
 *
 * Params (JSON):
 *   level_name  (string, required) — name without prefix/suffix, e.g. "TestLevel_Building"
 *   save_path   (string, required) — folder path, e.g. "/Game/Tests"
 *   template    (string, optional) — content path to an existing level to clone; empty = blank
 *
 * Returns success + the full asset path of the created level.
 *
 * Side effect: the newly-created level becomes the active editor world.
 */
class UNREALMCP_API FCreateLevelCommand : public IUnrealMCPCommand
{
public:
	FCreateLevelCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
