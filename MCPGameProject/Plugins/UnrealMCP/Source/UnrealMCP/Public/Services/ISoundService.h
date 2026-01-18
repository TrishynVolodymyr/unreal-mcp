#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class USoundWave;
class USoundCue;
class USoundAttenuation;
class USoundClass;
class USoundMix;
class AAmbientSound;
class UMetaSoundSource;

/**
 * Parameters for importing a sound wave
 */
struct UNREALMCP_API FSoundWaveImportParams
{
    /** Path to the source audio file */
    FString SourceFilePath;

    /** Name for the imported sound wave asset */
    FString AssetName;

    /** Content folder path (e.g., "/Game/Audio") */
    FString FolderPath = TEXT("/Game/Audio");

    /** Default constructor */
    FSoundWaveImportParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (SourceFilePath.IsEmpty())
        {
            OutError = TEXT("Source file path cannot be empty");
            return false;
        }
        if (AssetName.IsEmpty())
        {
            OutError = TEXT("Asset name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for spawning an ambient sound actor
 */
struct UNREALMCP_API FAmbientSoundSpawnParams
{
    /** Path to the sound asset (SoundWave, SoundCue, or MetaSound) */
    FString SoundPath;

    /** Name for the spawned actor */
    FString ActorName;

    /** Spawn location */
    FVector Location = FVector::ZeroVector;

    /** Spawn rotation */
    FRotator Rotation = FRotator::ZeroRotator;

    /** Whether to auto-activate on spawn */
    bool bAutoActivate = true;

    /** Optional attenuation settings path */
    FString AttenuationPath;

    /** Default constructor */
    FAmbientSoundSpawnParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (SoundPath.IsEmpty())
        {
            OutError = TEXT("Sound path cannot be empty");
            return false;
        }
        if (ActorName.IsEmpty())
        {
            OutError = TEXT("Actor name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating sound attenuation settings
 */
struct UNREALMCP_API FSoundAttenuationParams
{
    /** Name for the attenuation asset */
    FString AssetName;

    /** Content folder path */
    FString FolderPath = TEXT("/Game/Audio");

    /** Inner radius - sound is at full volume inside this */
    float InnerRadius = 400.0f;

    /** Falloff distance - sound fades from inner to outer radius */
    float FalloffDistance = 3600.0f;

    /** Attenuation function: "Linear", "Logarithmic", "Inverse", "LogReverse", "NaturalSound" */
    FString AttenuationFunction = TEXT("Linear");

    /** Whether to spatialize the sound */
    bool bSpatialize = true;

    /** Default constructor */
    FSoundAttenuationParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (AssetName.IsEmpty())
        {
            OutError = TEXT("Asset name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating a Sound Cue
 */
struct UNREALMCP_API FSoundCueCreationParams
{
    /** Name for the Sound Cue asset */
    FString AssetName;

    /** Content folder path */
    FString FolderPath = TEXT("/Game/Audio");

    /** Optional initial sound wave to add */
    FString InitialSoundWavePath;

    /** Default constructor */
    FSoundCueCreationParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (AssetName.IsEmpty())
        {
            OutError = TEXT("Asset name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding a node to a Sound Cue
 */
struct UNREALMCP_API FSoundCueNodeParams
{
    /** Path to the Sound Cue */
    FString SoundCuePath;

    /** Node type: "WavePlayer", "Mixer", "Random", "Modulator", "Looping", "Delay", "Concatenator" */
    FString NodeType;

    /** Optional sound wave path (for WavePlayer nodes) */
    FString SoundWavePath;

    /** Node position X */
    int32 PosX = 0;

    /** Node position Y */
    int32 PosY = 0;

    /** Default constructor */
    FSoundCueNodeParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (SoundCuePath.IsEmpty())
        {
            OutError = TEXT("Sound Cue path cannot be empty");
            return false;
        }
        if (NodeType.IsEmpty())
        {
            OutError = TEXT("Node type cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating a Sound Class
 */
struct UNREALMCP_API FSoundClassParams
{
    /** Name for the Sound Class asset */
    FString AssetName;

    /** Content folder path */
    FString FolderPath = TEXT("/Game/Audio");

    /** Optional parent sound class path */
    FString ParentClassPath;

    /** Default volume (0.0 - 1.0) */
    float DefaultVolume = 1.0f;

    /** Default constructor */
    FSoundClassParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (AssetName.IsEmpty())
        {
            OutError = TEXT("Asset name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating a Sound Mix
 */
struct UNREALMCP_API FSoundMixParams
{
    /** Name for the Sound Mix asset */
    FString AssetName;

    /** Content folder path */
    FString FolderPath = TEXT("/Game/Audio");

    /** Default constructor */
    FSoundMixParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (AssetName.IsEmpty())
        {
            OutError = TEXT("Asset name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating a MetaSound Source
 */
struct UNREALMCP_API FMetaSoundSourceParams
{
    /** Name for the MetaSound Source asset */
    FString AssetName;

    /** Content folder path */
    FString FolderPath = TEXT("/Game/Audio/MetaSounds");

    /** Output audio format: "Mono", "Stereo", "Quad", "FiveDotOne", "SevenDotOne" */
    FString OutputFormat = TEXT("Stereo");

    /** Whether this is a one-shot sound (auto-terminates) or continuous */
    bool bIsOneShot = true;

    /** Default constructor */
    FMetaSoundSourceParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (AssetName.IsEmpty())
        {
            OutError = TEXT("Asset name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding a node to a MetaSound
 */
struct UNREALMCP_API FMetaSoundNodeParams
{
    /** Path to the MetaSound asset */
    FString MetaSoundPath;

    /** Node class name (e.g., "Trigger Repeat", "AD Envelope", "Sine") */
    FString NodeClassName;

    /** Node namespace (e.g., "UE" for built-in, or plugin namespace) */
    FString NodeNamespace = TEXT("UE");

    /** Node variant (e.g., "Audio" for oscillator nodes, empty for trigger nodes) */
    FString NodeVariant;

    /** Node position X in the graph editor */
    int32 PosX = 0;

    /** Node position Y in the graph editor */
    int32 PosY = 0;

    /** Default constructor */
    FMetaSoundNodeParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (MetaSoundPath.IsEmpty())
        {
            OutError = TEXT("MetaSound path cannot be empty");
            return false;
        }
        if (NodeClassName.IsEmpty())
        {
            OutError = TEXT("Node class name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding an input to a MetaSound
 */
struct UNREALMCP_API FMetaSoundInputParams
{
    /** Path to the MetaSound asset */
    FString MetaSoundPath;

    /** Name for the input */
    FString InputName;

    /** Data type: "Float", "Int32", "Bool", "Trigger", "Audio", "String" */
    FString DataType = TEXT("Float");

    /** Default value (as string, converted based on type) */
    FString DefaultValue;

    /** Default constructor */
    FMetaSoundInputParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (MetaSoundPath.IsEmpty())
        {
            OutError = TEXT("MetaSound path cannot be empty");
            return false;
        }
        if (InputName.IsEmpty())
        {
            OutError = TEXT("Input name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding an output to a MetaSound
 */
struct UNREALMCP_API FMetaSoundOutputParams
{
    /** Path to the MetaSound asset */
    FString MetaSoundPath;

    /** Name for the output */
    FString OutputName;

    /** Data type: "Float", "Int32", "Bool", "Trigger", "Audio" */
    FString DataType = TEXT("Audio");

    /** Default constructor */
    FMetaSoundOutputParams() = default;

    bool IsValid(FString& OutError) const
    {
        if (MetaSoundPath.IsEmpty())
        {
            OutError = TEXT("MetaSound path cannot be empty");
            return false;
        }
        if (OutputName.IsEmpty())
        {
            OutError = TEXT("Output name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Interface for Sound service operations
 * Provides abstraction for audio asset creation, modification, and management
 */
class UNREALMCP_API ISoundService
{
public:
    virtual ~ISoundService() = default;

    // ========================================================================
    // Sound Wave Operations (Phase 1)
    // ========================================================================

    /**
     * Import an audio file from disk into the Unreal project
     * @param Params - Import parameters including source path, asset name, folder path
     * @param OutAssetPath - Path of the imported sound wave asset
     * @param OutError - Error message if import fails
     * @return true if import was successful
     */
    virtual bool ImportSoundFile(const FSoundWaveImportParams& Params, FString& OutAssetPath, FString& OutError) = 0;

    /**
     * Get metadata about a sound wave asset
     * @param SoundWavePath - Path to the sound wave
     * @param OutMetadata - Output JSON with duration, channels, sample rate, etc.
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool GetSoundWaveMetadata(const FString& SoundWavePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError) = 0;

    /**
     * Set properties on a sound wave
     * @param SoundWavePath - Path to the sound wave
     * @param bLooping - Whether the sound loops
     * @param Volume - Base volume multiplier
     * @param Pitch - Base pitch multiplier
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool SetSoundWaveProperties(const FString& SoundWavePath, bool bLooping, float Volume, float Pitch, FString& OutError) = 0;

    // ========================================================================
    // Audio Component Operations (Phase 1)
    // ========================================================================

    /**
     * Spawn an ambient sound actor in the level
     * @param Params - Spawn parameters
     * @param OutActorName - Name of the spawned actor
     * @param OutError - Error message if operation fails
     * @return Spawned actor or nullptr
     */
    virtual AAmbientSound* SpawnAmbientSound(const FAmbientSoundSpawnParams& Params, FString& OutActorName, FString& OutError) = 0;

    /**
     * Play a sound at a specific world location (one-shot)
     * @param SoundPath - Path to the sound asset
     * @param Location - World location
     * @param VolumeMultiplier - Volume multiplier
     * @param PitchMultiplier - Pitch multiplier
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool PlaySoundAtLocation(const FString& SoundPath, const FVector& Location, float VolumeMultiplier, float PitchMultiplier, FString& OutError) = 0;

    // ========================================================================
    // Sound Attenuation Operations (Phase 1)
    // ========================================================================

    /**
     * Create a sound attenuation settings asset
     * @param Params - Attenuation parameters
     * @param OutAssetPath - Path of the created asset
     * @param OutError - Error message if operation fails
     * @return Created attenuation or nullptr
     */
    virtual USoundAttenuation* CreateSoundAttenuation(const FSoundAttenuationParams& Params, FString& OutAssetPath, FString& OutError) = 0;

    /**
     * Set properties on an attenuation settings asset
     * @param AttenuationPath - Path to the attenuation asset
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value as JSON
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool SetAttenuationProperty(const FString& AttenuationPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;

    // ========================================================================
    // Sound Cue Operations (Phase 2)
    // ========================================================================

    /**
     * Create a new Sound Cue asset
     * @param Params - Creation parameters
     * @param OutAssetPath - Path of the created asset
     * @param OutError - Error message if operation fails
     * @return Created Sound Cue or nullptr
     */
    virtual USoundCue* CreateSoundCue(const FSoundCueCreationParams& Params, FString& OutAssetPath, FString& OutError) = 0;

    /**
     * Get metadata about a Sound Cue
     * @param SoundCuePath - Path to the Sound Cue
     * @param OutMetadata - Output JSON with nodes, connections, etc.
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool GetSoundCueMetadata(const FString& SoundCuePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError) = 0;

    /**
     * Add a node to a Sound Cue
     * @param Params - Node parameters
     * @param OutNodeId - ID of the created node
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool AddSoundCueNode(const FSoundCueNodeParams& Params, FString& OutNodeId, FString& OutError) = 0;

    /**
     * Connect two nodes in a Sound Cue
     * @param SoundCuePath - Path to the Sound Cue
     * @param SourceNodeId - Source node ID
     * @param TargetNodeId - Target node ID (or "Output" for final output)
     * @param SourcePinIndex - Source pin index (default 0)
     * @param TargetPinIndex - Target pin index (default 0)
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool ConnectSoundCueNodes(const FString& SoundCuePath, const FString& SourceNodeId, const FString& TargetNodeId, int32 SourcePinIndex, int32 TargetPinIndex, FString& OutError) = 0;

    /**
     * Set a property on a Sound Cue node
     * @param SoundCuePath - Path to the Sound Cue
     * @param NodeId - ID of the node
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value as JSON
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool SetSoundCueNodeProperty(const FString& SoundCuePath, const FString& NodeId, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;

    /**
     * Remove a node from a Sound Cue
     * @param SoundCuePath - Path to the Sound Cue
     * @param NodeId - ID of the node to remove
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool RemoveSoundCueNode(const FString& SoundCuePath, const FString& NodeId, FString& OutError) = 0;

    /**
     * Compile/validate a Sound Cue
     * @param SoundCuePath - Path to the Sound Cue
     * @param OutError - Error message if operation fails
     * @return true if compiled successfully
     */
    virtual bool CompileSoundCue(const FString& SoundCuePath, FString& OutError) = 0;

    // ========================================================================
    // Sound Class/Mix Operations (Phase 4 - Music System)
    // ========================================================================

    /**
     * Create a Sound Class asset
     * @param Params - Creation parameters
     * @param OutAssetPath - Path of the created asset
     * @param OutError - Error message if operation fails
     * @return Created Sound Class or nullptr
     */
    virtual USoundClass* CreateSoundClass(const FSoundClassParams& Params, FString& OutAssetPath, FString& OutError) = 0;

    /**
     * Create a Sound Mix asset
     * @param Params - Creation parameters
     * @param OutAssetPath - Path of the created asset
     * @param OutError - Error message if operation fails
     * @return Created Sound Mix or nullptr
     */
    virtual USoundMix* CreateSoundMix(const FSoundMixParams& Params, FString& OutAssetPath, FString& OutError) = 0;

    /**
     * Add a sound class modifier to a sound mix
     * @param SoundMixPath - Path to the Sound Mix
     * @param SoundClassPath - Path to the Sound Class to modify
     * @param VolumeAdjust - Volume adjustment (-1 to 1)
     * @param PitchAdjust - Pitch adjustment (-1 to 1)
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool AddSoundMixModifier(const FString& SoundMixPath, const FString& SoundClassPath, float VolumeAdjust, float PitchAdjust, FString& OutError) = 0;

    // ========================================================================
    // MetaSound Operations (Phase 3)
    // ========================================================================

    /**
     * Create a new MetaSound Source asset
     * @param Params - Creation parameters
     * @param OutAssetPath - Path of the created asset
     * @param OutError - Error message if operation fails
     * @return Created MetaSound Source or nullptr
     */
    virtual UMetaSoundSource* CreateMetaSoundSource(const FMetaSoundSourceParams& Params, FString& OutAssetPath, FString& OutError) = 0;

    /**
     * Get metadata about a MetaSound
     * @param MetaSoundPath - Path to the MetaSound
     * @param OutMetadata - Output JSON with graph, inputs, outputs, etc.
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool GetMetaSoundMetadata(const FString& MetaSoundPath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError) = 0;

    /**
     * Add a node to a MetaSound graph
     * @param Params - Node parameters
     * @param OutNodeId - GUID of the created node
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool AddMetaSoundNode(const FMetaSoundNodeParams& Params, FString& OutNodeId, FString& OutError) = 0;

    /**
     * Connect two nodes in a MetaSound graph
     * @param MetaSoundPath - Path to the MetaSound
     * @param SourceNodeId - Source node GUID
     * @param SourcePinName - Source pin name
     * @param TargetNodeId - Target node GUID
     * @param TargetPinName - Target pin name
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool ConnectMetaSoundNodes(const FString& MetaSoundPath, const FString& SourceNodeId, const FString& SourcePinName, const FString& TargetNodeId, const FString& TargetPinName, FString& OutError) = 0;

    /**
     * Set an input value on a MetaSound node
     * @param MetaSoundPath - Path to the MetaSound
     * @param NodeId - GUID of the node
     * @param InputName - Name of the input pin
     * @param Value - Value as JSON
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool SetMetaSoundNodeInput(const FString& MetaSoundPath, const FString& NodeId, const FString& InputName, const TSharedPtr<FJsonValue>& Value, FString& OutError) = 0;

    /**
     * Add a graph input to a MetaSound
     * @param Params - Input parameters
     * @param OutInputNodeId - GUID of the created input node
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool AddMetaSoundInput(const FMetaSoundInputParams& Params, FString& OutInputNodeId, FString& OutError) = 0;

    /**
     * Add a graph output to a MetaSound
     * @param Params - Output parameters
     * @param OutOutputNodeId - GUID of the created output node
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool AddMetaSoundOutput(const FMetaSoundOutputParams& Params, FString& OutOutputNodeId, FString& OutError) = 0;

    /**
     * Compile/validate a MetaSound
     * @param MetaSoundPath - Path to the MetaSound
     * @param OutError - Error message if operation fails
     * @return true if compiled successfully
     */
    virtual bool CompileMetaSound(const FString& MetaSoundPath, FString& OutError) = 0;

    /**
     * Search the MetaSound node palette for available node classes
     * @param SearchQuery - Query string to filter nodes (searches name, namespace, keywords, category)
     * @param MaxResults - Maximum number of results to return
     * @param OutResults - Array of JSON objects with node class info (namespace, name, variant, display_name, description, category)
     * @param OutError - Error message if operation fails
     * @return true if successful
     */
    virtual bool SearchMetaSoundPalette(const FString& SearchQuery, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutResults, FString& OutError) = 0;

    /**
     * Find a MetaSound Source by path
     * @param MetaSoundPath - Path to the MetaSound
     * @return MetaSound Source or nullptr
     */
    virtual UMetaSoundSource* FindMetaSoundSource(const FString& MetaSoundPath) = 0;

    // ========================================================================
    // Utility Methods
    // ========================================================================

    /**
     * Find a sound wave by path
     * @param SoundWavePath - Path to the sound wave
     * @return Sound wave or nullptr
     */
    virtual USoundWave* FindSoundWave(const FString& SoundWavePath) = 0;

    /**
     * Find a sound cue by path
     * @param SoundCuePath - Path to the sound cue
     * @return Sound cue or nullptr
     */
    virtual USoundCue* FindSoundCue(const FString& SoundCuePath) = 0;
};
