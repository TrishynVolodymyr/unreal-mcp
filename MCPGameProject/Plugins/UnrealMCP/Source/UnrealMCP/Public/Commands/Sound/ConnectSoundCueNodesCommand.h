#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to connect two nodes in a Sound Cue graph
 */
class UNREALMCP_API FConnectSoundCueNodesCommand : public IUnrealMCPCommand
{
public:
    explicit FConnectSoundCueNodesCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutSourceNodeId,
        FString& OutTargetNodeId, int32& OutSourcePinIndex, int32& OutTargetPinIndex, FString& OutError) const;
    FString CreateSuccessResponse() const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
