#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to compile/validate a MetaSound
 */
class UNREALMCP_API FCompileMetaSoundCommand : public IUnrealMCPCommand
{
public:
    explicit FCompileMetaSoundCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutError) const;
    FString CreateSuccessResponse(const FString& MetaSoundPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
