#pragma once

#include "CoreMinimal.h"
#include "Services/ISoundService.h"

// Log category for Sound service
DECLARE_LOG_CATEGORY_EXTERN(LogSoundService, Log, All);

/**
 * Concrete implementation of ISoundService
 * Provides audio operations with proper error handling and logging
 */
class UNREALMCP_API FSoundService : public ISoundService
{
public:
    /** Constructor */
    FSoundService();

    /** Destructor */
    virtual ~FSoundService() = default;

    /**
     * Get singleton instance
     * @return Reference to the singleton instance
     */
    static FSoundService& Get();

    // ========================================================================
    // ISoundService interface implementation - Sound Wave Operations
    // ========================================================================

    virtual bool ImportSoundFile(const FSoundWaveImportParams& Params, FString& OutAssetPath, FString& OutError) override;
    virtual bool GetSoundWaveMetadata(const FString& SoundWavePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError) override;
    virtual bool SetSoundWaveProperties(const FString& SoundWavePath, bool bLooping, float Volume, float Pitch, FString& OutError) override;

    // ========================================================================
    // ISoundService interface implementation - Audio Component Operations
    // ========================================================================

    virtual AAmbientSound* SpawnAmbientSound(const FAmbientSoundSpawnParams& Params, FString& OutActorName, FString& OutError) override;
    virtual bool PlaySoundAtLocation(const FString& SoundPath, const FVector& Location, float VolumeMultiplier, float PitchMultiplier, FString& OutError) override;

    // ========================================================================
    // ISoundService interface implementation - Sound Attenuation Operations
    // ========================================================================

    virtual USoundAttenuation* CreateSoundAttenuation(const FSoundAttenuationParams& Params, FString& OutAssetPath, FString& OutError) override;
    virtual bool SetAttenuationProperty(const FString& AttenuationPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) override;

    // ========================================================================
    // ISoundService interface implementation - Sound Cue Operations
    // ========================================================================

    virtual USoundCue* CreateSoundCue(const FSoundCueCreationParams& Params, FString& OutAssetPath, FString& OutError) override;
    virtual bool GetSoundCueMetadata(const FString& SoundCuePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError) override;
    virtual bool AddSoundCueNode(const FSoundCueNodeParams& Params, FString& OutNodeId, FString& OutError) override;
    virtual bool ConnectSoundCueNodes(const FString& SoundCuePath, const FString& SourceNodeId, const FString& TargetNodeId, int32 SourcePinIndex, int32 TargetPinIndex, FString& OutError) override;
    virtual bool SetSoundCueNodeProperty(const FString& SoundCuePath, const FString& NodeId, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) override;
    virtual bool RemoveSoundCueNode(const FString& SoundCuePath, const FString& NodeId, FString& OutError) override;
    virtual bool CompileSoundCue(const FString& SoundCuePath, FString& OutError) override;

    // ========================================================================
    // ISoundService interface implementation - Sound Class/Mix Operations
    // ========================================================================

    virtual USoundClass* CreateSoundClass(const FSoundClassParams& Params, FString& OutAssetPath, FString& OutError) override;
    virtual USoundMix* CreateSoundMix(const FSoundMixParams& Params, FString& OutAssetPath, FString& OutError) override;
    virtual bool AddSoundMixModifier(const FString& SoundMixPath, const FString& SoundClassPath, float VolumeAdjust, float PitchAdjust, FString& OutError) override;

    // ========================================================================
    // ISoundService interface implementation - MetaSound Operations
    // ========================================================================

    virtual UMetaSoundSource* CreateMetaSoundSource(const FMetaSoundSourceParams& Params, FString& OutAssetPath, FString& OutError) override;
    virtual bool GetMetaSoundMetadata(const FString& MetaSoundPath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError) override;
    virtual bool AddMetaSoundNode(const FMetaSoundNodeParams& Params, FString& OutNodeId, FString& OutError) override;
    virtual bool ConnectMetaSoundNodes(const FString& MetaSoundPath, const FString& SourceNodeId, const FString& SourcePinName, const FString& TargetNodeId, const FString& TargetPinName, FString& OutError) override;
    virtual bool SetMetaSoundNodeInput(const FString& MetaSoundPath, const FString& NodeId, const FString& InputName, const TSharedPtr<FJsonValue>& Value, FString& OutError) override;
    virtual bool AddMetaSoundInput(const FMetaSoundInputParams& Params, FString& OutInputNodeId, FString& OutError) override;
    virtual bool AddMetaSoundOutput(const FMetaSoundOutputParams& Params, FString& OutOutputNodeId, FString& OutError) override;
    virtual bool CompileMetaSound(const FString& MetaSoundPath, FString& OutError) override;
    virtual bool SearchMetaSoundPalette(const FString& SearchQuery, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutResults, FString& OutError) override;
    virtual UMetaSoundSource* FindMetaSoundSource(const FString& MetaSoundPath) override;

    // ========================================================================
    // ISoundService interface implementation - Utility Methods
    // ========================================================================

    virtual USoundWave* FindSoundWave(const FString& SoundWavePath) override;
    virtual USoundCue* FindSoundCue(const FString& SoundCuePath) override;

private:
    /** Singleton instance */
    static TUniquePtr<FSoundService> Instance;

    // ========================================================================
    // Internal Helper Methods
    // ========================================================================

    /**
     * Create a unique package for an asset
     * @param Path - Base path
     * @param Name - Asset name
     * @param OutPackage - Output package
     * @param OutError - Error message if creation fails
     * @return true if package was created
     */
    bool CreateAssetPackage(const FString& Path, const FString& Name, UPackage*& OutPackage, FString& OutError) const;

    /**
     * Save an asset to disk
     * @param Asset - Asset to save
     * @param OutError - Error message if saving fails
     * @return true if saved successfully
     */
    bool SaveAsset(UObject* Asset, FString& OutError) const;

    /**
     * Get attenuation function enum from string
     * @param FunctionName - "Linear", "Logarithmic", etc.
     * @return Attenuation function enum value
     */
    uint8 GetAttenuationFunction(const FString& FunctionName) const;
};
