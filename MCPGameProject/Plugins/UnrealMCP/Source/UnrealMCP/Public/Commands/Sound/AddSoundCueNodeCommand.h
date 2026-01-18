#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to add a node to a Sound Cue graph
 */
class UNREALMCP_API FAddSoundCueNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FAddSoundCueNodeCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutNodeType,
        FString& OutSoundWavePath, int32& OutPosX, int32& OutPosY, FString& OutError) const;
    FString CreateSuccessResponse(const FString& NodeId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
