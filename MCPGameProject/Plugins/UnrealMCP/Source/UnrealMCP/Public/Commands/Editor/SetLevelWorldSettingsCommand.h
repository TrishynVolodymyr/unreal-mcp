#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Modify a level's World Settings. Currently supports setting the GameMode override.
 *
 * Params (JSON):
 *   level_path  (string, required) — full asset path, e.g. "/Game/Tests/TestLevel_Building"
 *   game_mode   (string, optional) — content path to a GameMode (BP or C++ class).
 *                                    Examples:
 *                                      "/Game/TopDown/Blueprints/BP_TopDownGameMode"
 *                                      "/Script/SimPrototype.GameModeMain"
 *                                    Empty string clears the override.
 *
 * Loads the level, modifies WorldSettings, marks dirty, saves. Becomes the active editor level.
 */
class UNREALMCP_API FSetLevelWorldSettingsCommand : public IUnrealMCPCommand
{
public:
	FSetLevelWorldSettingsCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
