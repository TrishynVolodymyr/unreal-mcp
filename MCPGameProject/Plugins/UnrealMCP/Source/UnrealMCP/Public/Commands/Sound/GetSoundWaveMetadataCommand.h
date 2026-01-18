#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

// Forward declaration
class ISoundService;

/**
 * Command to get metadata about a sound wave asset
 * Returns duration, sample rate, channels, and other audio properties
 */
class UNREALMCP_API FGetSoundWaveMetadataCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InSoundService - Reference to the sound service
     */
    explicit FGetSoundWaveMetadataCommand(ISoundService& InSoundService);

    //~ IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;
    //~ End IUnrealMCPCommand interface

private:
    /** Reference to sound service for operations */
    ISoundService& SoundService;

    /**
     * Parse command parameters from JSON
     * @param JsonString - JSON parameters string
     * @param OutSoundWavePath - Extracted sound wave path
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutSoundWavePath, FString& OutError) const;

    /**
     * Create a success response with metadata
     * @param Metadata - JSON metadata object
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const;

    /**
     * Create an error response
     * @param ErrorMessage - Error message to include
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
