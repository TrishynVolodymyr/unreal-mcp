#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to connect two nodes in a MetaSound graph
 */
class UNREALMCP_API FConnectMetaSoundNodesCommand : public IUnrealMCPCommand
{
public:
    explicit FConnectMetaSoundNodesCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutSourceNodeId,
        FString& OutSourcePinName, FString& OutTargetNodeId, FString& OutTargetPinName, FString& OutError) const;
    FString CreateSuccessResponse(const FString& SourceNodeId, const FString& SourcePinName,
        const FString& TargetNodeId, const FString& TargetPinName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
