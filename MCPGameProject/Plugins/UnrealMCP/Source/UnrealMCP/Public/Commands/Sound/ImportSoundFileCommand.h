#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

// Forward declaration
class ISoundService;

/**
 * Command to import an audio file from disk into the Unreal project
 * Supports WAV, MP3, OGG, FLAC, and AIFF formats
 */
class UNREALMCP_API FImportSoundFileCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InSoundService - Reference to the sound service
     */
    explicit FImportSoundFileCommand(ISoundService& InSoundService);

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
     * @param OutSourceFilePath - Path to source audio file on disk
     * @param OutAssetName - Name for the imported asset
     * @param OutFolderPath - Destination folder in Content (default: /Game/Audio)
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutSourceFilePath, FString& OutAssetName, FString& OutFolderPath, FString& OutError) const;

    /**
     * Create a success response
     * @param AssetPath - Path of the imported asset
     * @param AssetName - Name of the imported asset
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& AssetPath, const FString& AssetName) const;

    /**
     * Create an error response
     * @param ErrorMessage - Error message to include
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
