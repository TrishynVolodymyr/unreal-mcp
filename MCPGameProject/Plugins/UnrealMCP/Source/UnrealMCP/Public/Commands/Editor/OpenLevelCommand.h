#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Open an EXISTING level (umap) as the ACTIVE editor world, by content path.
 *
 * The missing counterpart to FCreateLevelCommand (makes a NEW level active) and
 * FSetLevelWorldSettingsCommand (edits a level WITHOUT switching the active world):
 * this switches the editor's current world to an already-existing level — e.g. to leave
 * the default startup map and work on a test level — which no other MCP tool can do.
 *
 * Params (JSON):
 *   level_path (string, required) — package path of the level, e.g. "/Game/Tests/TestLevel_ColonyView"
 *
 * Returns success + level_path; on a World Partition target returns success=false +
 * was_world_partition=true + a clear error.
 *
 * SAFETY: World Partition levels are REFUSED. Switching the active editor world INTO a WP
 * map via code can trip the engine map-load leak-check fatal (EditorServer.cpp "World Memory
 * Leaks"; the project has hit this — see game-design/tech/voxel.md anti-pattern). The target
 * world is first loaded as an OBJECT (no world switch) to detect WP safely; only a non-WP
 * level is then switched in via ULevelEditorSubsystem::LoadLevel.
 *
 * Side effect: a non-WP target becomes the active editor world.
 */
class UNREALMCP_API FOpenLevelCommand : public IUnrealMCPCommand
{
public:
	FOpenLevelCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
