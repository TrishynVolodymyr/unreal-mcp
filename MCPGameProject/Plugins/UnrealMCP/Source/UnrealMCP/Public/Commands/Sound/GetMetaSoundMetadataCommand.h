#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to get metadata about a MetaSound asset
 */
class UNREALMCP_API FGetMetaSoundMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetMetaSoundMetadataCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
