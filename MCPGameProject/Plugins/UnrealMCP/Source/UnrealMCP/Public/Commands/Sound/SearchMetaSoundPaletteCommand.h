#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to search the MetaSound node palette for available node classes
 */
class UNREALMCP_API FSearchMetaSoundPaletteCommand : public IUnrealMCPCommand
{
public:
    explicit FSearchMetaSoundPaletteCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSearchQuery, int32& OutMaxResults, FString& OutError) const;
    FString CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Results) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
