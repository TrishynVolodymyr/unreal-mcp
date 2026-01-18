#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to remove a node from a Sound Cue graph
 */
class UNREALMCP_API FRemoveSoundCueNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveSoundCueNodeCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutNodeId, FString& OutError) const;
    FString CreateSuccessResponse(const FString& NodeId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
