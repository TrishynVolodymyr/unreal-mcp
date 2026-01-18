#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to add a graph output to a MetaSound
 */
class UNREALMCP_API FAddMetaSoundOutputCommand : public IUnrealMCPCommand
{
public:
    explicit FAddMetaSoundOutputCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutOutputName,
        FString& OutDataType, FString& OutError) const;
    FString CreateSuccessResponse(const FString& OutputNodeId, const FString& OutputName, const FString& DataType) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
