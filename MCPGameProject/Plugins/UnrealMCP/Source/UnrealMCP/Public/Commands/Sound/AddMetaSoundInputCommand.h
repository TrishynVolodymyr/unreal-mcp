#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to add a graph input to a MetaSound
 */
class UNREALMCP_API FAddMetaSoundInputCommand : public IUnrealMCPCommand
{
public:
    explicit FAddMetaSoundInputCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutInputName,
        FString& OutDataType, FString& OutDefaultValue, FString& OutError) const;
    FString CreateSuccessResponse(const FString& InputNodeId, const FString& InputName, const FString& DataType) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
