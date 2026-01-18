#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to set an input value on a MetaSound node
 */
class UNREALMCP_API FSetMetaSoundInputCommand : public IUnrealMCPCommand
{
public:
    explicit FSetMetaSoundInputCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    FString CreateSuccessResponse(const FString& InputName, const FString& NodeId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
