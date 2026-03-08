#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to set PlayerMappableKeySettings on an InputAction asset.
 * This marks the InputAction as rebindable by the player via UEnhancedInputUserSettings.
 */
class UNREALMCP_API FSetInputActionMappableSettingsCommand : public IUnrealMCPCommand
{
public:
	explicit FSetInputActionMappableSettingsCommand(TSharedPtr<IProjectService> InProjectService);
	virtual ~FSetInputActionMappableSettingsCommand() = default;

	virtual FString GetCommandName() const override;
	virtual FString Execute(const FString& Parameters) override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	TSharedPtr<IProjectService> ProjectService;
};
