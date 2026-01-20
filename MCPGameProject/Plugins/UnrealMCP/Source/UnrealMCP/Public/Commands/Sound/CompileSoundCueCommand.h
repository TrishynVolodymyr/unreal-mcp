#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to compile/validate a Sound Cue
 */
class UNREALMCP_API FCompileSoundCueCommand : public IUnrealMCPCommand
{
public:
    explicit FCompileSoundCueCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutError) const;
    FString CreateSuccessResponse() const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
