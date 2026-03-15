#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to set per-mapping PlayerMappableKeySettings on an IMC key mapping.
 * Sets SettingBehavior=OverrideSettings and creates a UPlayerMappableKeySettings
 * with unique Name/DisplayName/DisplayCategory per mapping.
 * This enables per-key rebinding for multi-key actions like WASD movement.
 */
class UNREALMCP_API FSetIMCMappingSettingsCommand : public IUnrealMCPCommand
{
public:
	explicit FSetIMCMappingSettingsCommand(TSharedPtr<IProjectService> InProjectService);
	virtual ~FSetIMCMappingSettingsCommand() = default;

	virtual FString GetCommandName() const override;
	virtual FString Execute(const FString& Parameters) override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	TSharedPtr<IProjectService> ProjectService;
};
