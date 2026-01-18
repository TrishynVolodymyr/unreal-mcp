#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to set a property on a Sound Cue node
 */
class UNREALMCP_API FSetSoundCueNodePropertyCommand : public IUnrealMCPCommand
{
public:
    explicit FSetSoundCueNodePropertyCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutNodeId,
        FString& OutPropertyName, TSharedPtr<FJsonValue>& OutPropertyValue, FString& OutError) const;
    FString CreateSuccessResponse() const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
