#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to add a node to a MetaSound graph
 */
class UNREALMCP_API FAddMetaSoundNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FAddMetaSoundNodeCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutNodeClassName,
        FString& OutNodeNamespace, FString& OutNodeVariant, int32& OutPosX, int32& OutPosY, FString& OutError) const;
    FString CreateSuccessResponse(const FString& NodeId, const FString& NodeClassName, const FString& NodeNamespace, const FString& NodeVariant) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
