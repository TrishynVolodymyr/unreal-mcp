#include "Services/SoundService.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundNode.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundNodeMixer.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeDelay.h"
#include "Sound/SoundNodeConcatenator.h"
#include "Sound/SoundNodeAttenuation.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Dom/JsonValue.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetImportTask.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// MetaSound includes
#include "MetasoundSource.h"
#include "MetasoundBuilderSubsystem.h"
#include "MetasoundBuilderBase.h"
#include "MetasoundFrontendDocumentBuilder.h"
#include "MetasoundFrontendSearchEngine.h"
#include "MetasoundAssetManager.h"
#include "MetasoundAssetBase.h"
#include "MetasoundUObjectRegistry.h"
#include "MetasoundDocumentBuilderRegistry.h"
#include "MetasoundEditorSubsystem.h"  // For RegisterGraphWithFrontend
#include "NodeTemplates/MetasoundFrontendNodeTemplateInput.h"  // For FInputNodeTemplate::CreateNode

DEFINE_LOG_CATEGORY(LogSoundService);

// Singleton instance
TUniquePtr<FSoundService> FSoundService::Instance;

FSoundService::FSoundService()
{
    UE_LOG(LogSoundService, Log, TEXT("SoundService initialized"));
}

FSoundService& FSoundService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FSoundService>();
    }
    return *Instance;
}

// ============================================================================
// Sound Wave Operations
// ============================================================================

bool FSoundService::ImportSoundFile(const FSoundWaveImportParams& Params, FString& OutAssetPath, FString& OutError)
{
    // Validate parameters
    if (!Params.IsValid(OutError))
    {
        return false;
    }

    // Check if source file exists
    if (!FPaths::FileExists(Params.SourceFilePath))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *Params.SourceFilePath);
        return false;
    }

    // Get the file extension to validate it's an audio file
    FString Extension = FPaths::GetExtension(Params.SourceFilePath).ToLower();
    TArray<FString> SupportedExtensions = { TEXT("wav"), TEXT("mp3"), TEXT("ogg"), TEXT("flac"), TEXT("aiff"), TEXT("aif") };
    if (!SupportedExtensions.Contains(Extension))
    {
        OutError = FString::Printf(TEXT("Unsupported audio format: %s. Supported: wav, mp3, ogg, flac, aiff"), *Extension);
        return false;
    }

    // Get the asset tools module
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // Prepare the destination path
    FString DestinationPath = Params.FolderPath;
    if (!DestinationPath.StartsWith(TEXT("/")))
    {
        DestinationPath = TEXT("/Game/") + DestinationPath;
    }
    else if (!DestinationPath.StartsWith(TEXT("/Game")))
    {
        // Make sure the path is under /Game
        if (DestinationPath.StartsWith(TEXT("/")))
        {
            DestinationPath = TEXT("/Game") + DestinationPath;
        }
    }

    // Create the import task
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->Filename = Params.SourceFilePath;
    ImportTask->DestinationPath = DestinationPath;
    ImportTask->DestinationName = Params.AssetName;
    ImportTask->bReplaceExisting = true;
    ImportTask->bAutomated = true;
    ImportTask->bSave = true;

    // Execute the import
    TArray<UAssetImportTask*> ImportTasks;
    ImportTasks.Add(ImportTask);
    AssetTools.ImportAssetTasks(ImportTasks);

    // Check if import was successful
    if (ImportTask->ImportedObjectPaths.Num() == 0)
    {
        OutError = FString::Printf(TEXT("Failed to import audio file: %s"), *Params.SourceFilePath);
        return false;
    }

    // Get the imported asset path
    OutAssetPath = ImportTask->ImportedObjectPaths[0];

    // Verify the asset was imported as a SoundWave
    USoundWave* ImportedSound = FindSoundWave(OutAssetPath);
    if (!ImportedSound)
    {
        OutError = FString::Printf(TEXT("Import succeeded but asset is not a SoundWave: %s"), *OutAssetPath);
        return false;
    }

    UE_LOG(LogSoundService, Log, TEXT("Imported audio file '%s' as SoundWave: %s (Duration: %.2fs, Channels: %d)"),
        *Params.SourceFilePath, *OutAssetPath, ImportedSound->Duration, ImportedSound->NumChannels);

    return true;
}

bool FSoundService::GetSoundWaveMetadata(const FString& SoundWavePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError)
{
    USoundWave* SoundWave = FindSoundWave(SoundWavePath);
    if (!SoundWave)
    {
        OutError = FString::Printf(TEXT("Sound wave not found: %s"), *SoundWavePath);
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), SoundWave->GetName());
    OutMetadata->SetStringField(TEXT("path"), SoundWavePath);
    OutMetadata->SetNumberField(TEXT("duration"), SoundWave->Duration);
    OutMetadata->SetNumberField(TEXT("sample_rate"), SoundWave->GetSampleRateForCurrentPlatform());
    OutMetadata->SetNumberField(TEXT("num_channels"), SoundWave->NumChannels);
    OutMetadata->SetBoolField(TEXT("is_looping"), SoundWave->bLooping);
    OutMetadata->SetNumberField(TEXT("volume"), SoundWave->Volume);
    OutMetadata->SetNumberField(TEXT("pitch"), SoundWave->Pitch);

    // Compression and streaming info
    OutMetadata->SetBoolField(TEXT("is_streaming"), SoundWave->IsStreaming());

    UE_LOG(LogSoundService, Log, TEXT("Retrieved metadata for sound wave: %s (Duration: %.2fs, Channels: %d)"),
        *SoundWavePath, SoundWave->Duration, SoundWave->NumChannels);

    return true;
}

bool FSoundService::SetSoundWaveProperties(const FString& SoundWavePath, bool bLooping, float Volume, float Pitch, FString& OutError)
{
    USoundWave* SoundWave = FindSoundWave(SoundWavePath);
    if (!SoundWave)
    {
        OutError = FString::Printf(TEXT("Sound wave not found: %s"), *SoundWavePath);
        return false;
    }

    SoundWave->Modify();
    SoundWave->bLooping = bLooping;
    SoundWave->Volume = FMath::Clamp(Volume, 0.0f, 4.0f);
    SoundWave->Pitch = FMath::Clamp(Pitch, 0.1f, 4.0f);

    if (!SaveAsset(SoundWave, OutError))
    {
        return false;
    }

    UE_LOG(LogSoundService, Log, TEXT("Set properties on sound wave: %s (Looping: %d, Volume: %.2f, Pitch: %.2f)"),
        *SoundWavePath, bLooping, Volume, Pitch);

    return true;
}

// ============================================================================
// Audio Component Operations
// ============================================================================

AAmbientSound* FSoundService::SpawnAmbientSound(const FAmbientSoundSpawnParams& Params, FString& OutActorName, FString& OutError)
{
    // Get the editor world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No editor world available");
        return nullptr;
    }

    // Load the sound asset
    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *Params.SoundPath));
    if (!Sound)
    {
        OutError = FString::Printf(TEXT("Failed to load sound: %s"), *Params.SoundPath);
        return nullptr;
    }

    // Spawn the ambient sound actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(*Params.ActorName);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAmbientSound* AmbientSound = World->SpawnActor<AAmbientSound>(
        AAmbientSound::StaticClass(),
        Params.Location,
        Params.Rotation,
        SpawnParams
    );

    if (!AmbientSound)
    {
        OutError = TEXT("Failed to spawn ambient sound actor");
        return nullptr;
    }

    // Set the sound
    UAudioComponent* AudioComp = AmbientSound->GetAudioComponent();
    if (AudioComp)
    {
        AudioComp->SetSound(Sound);
        AudioComp->bAutoActivate = Params.bAutoActivate;

        // Set attenuation if provided
        if (!Params.AttenuationPath.IsEmpty())
        {
            USoundAttenuation* Attenuation = Cast<USoundAttenuation>(
                StaticLoadObject(USoundAttenuation::StaticClass(), nullptr, *Params.AttenuationPath)
            );
            if (Attenuation)
            {
                AudioComp->AttenuationSettings = Attenuation;
            }
        }
    }

    // Set the actor label
    AmbientSound->SetActorLabel(Params.ActorName);
    OutActorName = Params.ActorName;

    UE_LOG(LogSoundService, Log, TEXT("Spawned ambient sound: %s at (%.2f, %.2f, %.2f)"),
        *Params.ActorName, Params.Location.X, Params.Location.Y, Params.Location.Z);

    return AmbientSound;
}

bool FSoundService::PlaySoundAtLocation(const FString& SoundPath, const FVector& Location, float VolumeMultiplier, float PitchMultiplier, FString& OutError)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No editor world available");
        return false;
    }

    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundPath));
    if (!Sound)
    {
        OutError = FString::Printf(TEXT("Failed to load sound: %s"), *SoundPath);
        return false;
    }

    UGameplayStatics::PlaySoundAtLocation(
        World,
        Sound,
        Location,
        FRotator::ZeroRotator,
        VolumeMultiplier,
        PitchMultiplier
    );

    UE_LOG(LogSoundService, Log, TEXT("Playing sound at location: %s at (%.2f, %.2f, %.2f)"),
        *SoundPath, Location.X, Location.Y, Location.Z);

    return true;
}

// ============================================================================
// Sound Attenuation Operations
// ============================================================================

USoundAttenuation* FSoundService::CreateSoundAttenuation(const FSoundAttenuationParams& Params, FString& OutAssetPath, FString& OutError)
{
    UPackage* Package = nullptr;
    if (!CreateAssetPackage(Params.FolderPath, Params.AssetName, Package, OutError))
    {
        return nullptr;
    }

    USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(Package, FName(*Params.AssetName), RF_Public | RF_Standalone);
    if (!Attenuation)
    {
        OutError = TEXT("Failed to create sound attenuation object");
        return nullptr;
    }

    // Configure attenuation settings
    FSoundAttenuationSettings& Settings = Attenuation->Attenuation;
    Settings.bAttenuate = true;
    Settings.bSpatialize = Params.bSpatialize;
    Settings.AttenuationShapeExtents = FVector(Params.InnerRadius, 0.f, 0.f);
    Settings.FalloffDistance = Params.FalloffDistance;
    Settings.DistanceAlgorithm = static_cast<EAttenuationDistanceModel>(GetAttenuationFunction(Params.AttenuationFunction));

    if (!SaveAsset(Attenuation, OutError))
    {
        return nullptr;
    }

    OutAssetPath = Package->GetPathName();
    UE_LOG(LogSoundService, Log, TEXT("Created sound attenuation: %s"), *OutAssetPath);

    return Attenuation;
}

bool FSoundService::SetAttenuationProperty(const FString& AttenuationPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    USoundAttenuation* Attenuation = Cast<USoundAttenuation>(
        StaticLoadObject(USoundAttenuation::StaticClass(), nullptr, *AttenuationPath)
    );
    if (!Attenuation)
    {
        OutError = FString::Printf(TEXT("Sound attenuation not found: %s"), *AttenuationPath);
        return false;
    }

    Attenuation->Modify();
    FSoundAttenuationSettings& Settings = Attenuation->Attenuation;

    // Handle common properties
    if (PropertyName == TEXT("inner_radius"))
    {
        Settings.AttenuationShapeExtents.X = PropertyValue->AsNumber();
    }
    else if (PropertyName == TEXT("falloff_distance"))
    {
        Settings.FalloffDistance = PropertyValue->AsNumber();
    }
    else if (PropertyName == TEXT("spatialize"))
    {
        Settings.bSpatialize = PropertyValue->AsBool();
    }
    else if (PropertyName == TEXT("attenuate"))
    {
        Settings.bAttenuate = PropertyValue->AsBool();
    }
    else
    {
        OutError = FString::Printf(TEXT("Unknown property: %s"), *PropertyName);
        return false;
    }

    if (!SaveAsset(Attenuation, OutError))
    {
        return false;
    }

    UE_LOG(LogSoundService, Log, TEXT("Set attenuation property: %s.%s"), *AttenuationPath, *PropertyName);
    return true;
}

// ============================================================================
// Sound Cue Operations (Phase 2)
// ============================================================================

USoundCue* FSoundService::CreateSoundCue(const FSoundCueCreationParams& Params, FString& OutAssetPath, FString& OutError)
{
    UPackage* Package = nullptr;
    if (!CreateAssetPackage(Params.FolderPath, Params.AssetName, Package, OutError))
    {
        return nullptr;
    }

    USoundCue* SoundCue = NewObject<USoundCue>(Package, FName(*Params.AssetName), RF_Public | RF_Standalone);
    if (!SoundCue)
    {
        OutError = TEXT("Failed to create Sound Cue object");
        return nullptr;
    }

    // Set default properties
    SoundCue->VolumeMultiplier = 1.0f;
    SoundCue->PitchMultiplier = 1.0f;

#if WITH_EDITOR
    // Create the sound cue graph for the editor
    SoundCue->CreateGraph();
#endif

    // If an initial sound wave path is provided, create a WavePlayer node
    if (!Params.InitialSoundWavePath.IsEmpty())
    {
        USoundWave* SoundWave = FindSoundWave(Params.InitialSoundWavePath);
        if (SoundWave)
        {
            USoundNodeWavePlayer* WavePlayer = SoundCue->ConstructSoundNode<USoundNodeWavePlayer>();
            if (WavePlayer)
            {
                WavePlayer->SetSoundWave(SoundWave);
                SoundCue->FirstNode = WavePlayer;

#if WITH_EDITOR
                SoundCue->LinkGraphNodesFromSoundNodes();
#endif
            }
        }
        else
        {
            UE_LOG(LogSoundService, Warning, TEXT("Initial sound wave not found: %s"), *Params.InitialSoundWavePath);
        }
    }

    if (!SaveAsset(SoundCue, OutError))
    {
        return nullptr;
    }

    OutAssetPath = Package->GetPathName();
    UE_LOG(LogSoundService, Log, TEXT("Created Sound Cue: %s"), *OutAssetPath);

    return SoundCue;
}

bool FSoundService::GetSoundCueMetadata(const FString& SoundCuePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError)
{
    USoundCue* SoundCue = FindSoundCue(SoundCuePath);
    if (!SoundCue)
    {
        OutError = FString::Printf(TEXT("Sound cue not found: %s"), *SoundCuePath);
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), SoundCue->GetName());
    OutMetadata->SetStringField(TEXT("path"), SoundCuePath);
    OutMetadata->SetNumberField(TEXT("duration"), SoundCue->Duration);
    OutMetadata->SetNumberField(TEXT("max_distance"), SoundCue->GetMaxDistance());
    OutMetadata->SetNumberField(TEXT("volume_multiplier"), SoundCue->VolumeMultiplier);
    OutMetadata->SetNumberField(TEXT("pitch_multiplier"), SoundCue->PitchMultiplier);

    // First node (root) info
    if (SoundCue->FirstNode)
    {
        OutMetadata->SetStringField(TEXT("first_node"), SoundCue->FirstNode->GetName());
    }
    else
    {
        OutMetadata->SetStringField(TEXT("first_node"), TEXT("None"));
    }

#if WITH_EDITORONLY_DATA
    // Build node list from AllNodes
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;

    for (USoundNode* Node : SoundCue->AllNodes)
    {
        if (!Node) continue;

        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("id"), Node->GetName());
        NodeObj->SetStringField(TEXT("type"), Node->GetClass()->GetName());

        // Add type-specific properties
        if (USoundNodeWavePlayer* WavePlayer = Cast<USoundNodeWavePlayer>(Node))
        {
            if (USoundWave* Wave = WavePlayer->GetSoundWave())
            {
                NodeObj->SetStringField(TEXT("sound_wave"), Wave->GetPathName());
            }
            NodeObj->SetBoolField(TEXT("looping"), WavePlayer->bLooping);
        }
        else if (USoundNodeMixer* Mixer = Cast<USoundNodeMixer>(Node))
        {
            TArray<TSharedPtr<FJsonValue>> VolumesArray;
            for (float Vol : Mixer->InputVolume)
            {
                VolumesArray.Add(MakeShared<FJsonValueNumber>(Vol));
            }
            NodeObj->SetArrayField(TEXT("input_volumes"), VolumesArray);
        }
        else if (USoundNodeRandom* Random = Cast<USoundNodeRandom>(Node))
        {
            TArray<TSharedPtr<FJsonValue>> WeightsArray;
            for (float Weight : Random->Weights)
            {
                WeightsArray.Add(MakeShared<FJsonValueNumber>(Weight));
            }
            NodeObj->SetArrayField(TEXT("weights"), WeightsArray);
            NodeObj->SetBoolField(TEXT("randomize_without_replacement"), Random->bRandomizeWithoutReplacement);
        }
        else if (USoundNodeModulator* Modulator = Cast<USoundNodeModulator>(Node))
        {
            NodeObj->SetNumberField(TEXT("pitch_min"), Modulator->PitchMin);
            NodeObj->SetNumberField(TEXT("pitch_max"), Modulator->PitchMax);
            NodeObj->SetNumberField(TEXT("volume_min"), Modulator->VolumeMin);
            NodeObj->SetNumberField(TEXT("volume_max"), Modulator->VolumeMax);
        }
        else if (USoundNodeLooping* Looping = Cast<USoundNodeLooping>(Node))
        {
            NodeObj->SetNumberField(TEXT("loop_count"), Looping->LoopCount);
            NodeObj->SetBoolField(TEXT("loop_indefinitely"), Looping->bLoopIndefinitely != 0);
        }

        // Count children
        NodeObj->SetNumberField(TEXT("child_count"), Node->ChildNodes.Num());

        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));

        // Build connections from this node's children
        for (int32 ChildIdx = 0; ChildIdx < Node->ChildNodes.Num(); ChildIdx++)
        {
            USoundNode* ChildNode = Node->ChildNodes[ChildIdx];
            if (ChildNode)
            {
                TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                ConnObj->SetStringField(TEXT("source_node"), ChildNode->GetName());
                ConnObj->SetStringField(TEXT("target_node"), Node->GetName());
                ConnObj->SetNumberField(TEXT("target_pin_index"), ChildIdx);
                ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
            }
        }
    }

    OutMetadata->SetArrayField(TEXT("nodes"), NodesArray);
    OutMetadata->SetArrayField(TEXT("connections"), ConnectionsArray);
    OutMetadata->SetNumberField(TEXT("node_count"), SoundCue->AllNodes.Num());
#endif

    UE_LOG(LogSoundService, Log, TEXT("Retrieved metadata for Sound Cue: %s"), *SoundCuePath);
    return true;
}

bool FSoundService::AddSoundCueNode(const FSoundCueNodeParams& Params, FString& OutNodeId, FString& OutError)
{
    USoundCue* SoundCue = FindSoundCue(Params.SoundCuePath);
    if (!SoundCue)
    {
        OutError = FString::Printf(TEXT("Sound Cue not found: %s"), *Params.SoundCuePath);
        return false;
    }

    SoundCue->Modify();

    USoundNode* NewNode = nullptr;
    FString NodeType = Params.NodeType.ToLower();

    // Create the appropriate node type
    if (NodeType == TEXT("waveplayer") || NodeType == TEXT("wave_player"))
    {
        USoundNodeWavePlayer* WavePlayer = SoundCue->ConstructSoundNode<USoundNodeWavePlayer>();
        if (WavePlayer && !Params.SoundWavePath.IsEmpty())
        {
            USoundWave* SoundWave = FindSoundWave(Params.SoundWavePath);
            if (SoundWave)
            {
                WavePlayer->SetSoundWave(SoundWave);
            }
            else
            {
                UE_LOG(LogSoundService, Warning, TEXT("Sound wave not found: %s"), *Params.SoundWavePath);
            }
        }
        NewNode = WavePlayer;
    }
    else if (NodeType == TEXT("mixer"))
    {
        NewNode = SoundCue->ConstructSoundNode<USoundNodeMixer>();
    }
    else if (NodeType == TEXT("random"))
    {
        NewNode = SoundCue->ConstructSoundNode<USoundNodeRandom>();
    }
    else if (NodeType == TEXT("modulator"))
    {
        USoundNodeModulator* Modulator = SoundCue->ConstructSoundNode<USoundNodeModulator>();
        if (Modulator)
        {
            // Set default ranges
            Modulator->PitchMin = 1.0f;
            Modulator->PitchMax = 1.0f;
            Modulator->VolumeMin = 1.0f;
            Modulator->VolumeMax = 1.0f;
        }
        NewNode = Modulator;
    }
    else if (NodeType == TEXT("looping"))
    {
        USoundNodeLooping* Looping = SoundCue->ConstructSoundNode<USoundNodeLooping>();
        if (Looping)
        {
            Looping->LoopCount = 1;
            Looping->bLoopIndefinitely = false;
        }
        NewNode = Looping;
    }
    else if (NodeType == TEXT("delay"))
    {
        NewNode = SoundCue->ConstructSoundNode<USoundNodeDelay>();
    }
    else if (NodeType == TEXT("concatenator"))
    {
        NewNode = SoundCue->ConstructSoundNode<USoundNodeConcatenator>();
    }
    else if (NodeType == TEXT("attenuation"))
    {
        NewNode = SoundCue->ConstructSoundNode<USoundNodeAttenuation>();
    }
    else
    {
        OutError = FString::Printf(TEXT("Unknown node type: %s. Valid types: WavePlayer, Mixer, Random, Modulator, Looping, Delay, Concatenator, Attenuation"), *Params.NodeType);
        return false;
    }

    if (!NewNode)
    {
        OutError = FString::Printf(TEXT("Failed to create node of type: %s"), *Params.NodeType);
        return false;
    }

#if WITH_EDITOR
    // Link graph nodes to reflect the new node
    SoundCue->LinkGraphNodesFromSoundNodes();
#endif

    // Save the asset
    FString SaveError;
    if (!SaveAsset(SoundCue, SaveError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save Sound Cue after adding node: %s"), *SaveError);
    }

    OutNodeId = NewNode->GetName();
    UE_LOG(LogSoundService, Log, TEXT("Added %s node '%s' to Sound Cue: %s"), *Params.NodeType, *OutNodeId, *Params.SoundCuePath);
    return true;
}

bool FSoundService::ConnectSoundCueNodes(const FString& SoundCuePath, const FString& SourceNodeId, const FString& TargetNodeId, int32 SourcePinIndex, int32 TargetPinIndex, FString& OutError)
{
    USoundCue* SoundCue = FindSoundCue(SoundCuePath);
    if (!SoundCue)
    {
        OutError = FString::Printf(TEXT("Sound Cue not found: %s"), *SoundCuePath);
        return false;
    }

    SoundCue->Modify();

#if WITH_EDITORONLY_DATA
    // Find the source node
    USoundNode* SourceNode = nullptr;
    for (USoundNode* Node : SoundCue->AllNodes)
    {
        if (Node && Node->GetName() == SourceNodeId)
        {
            SourceNode = Node;
            break;
        }
    }

    if (!SourceNode)
    {
        OutError = FString::Printf(TEXT("Source node not found: %s"), *SourceNodeId);
        return false;
    }

    // Check if target is "Output" - meaning connect to FirstNode (root)
    if (TargetNodeId.Equals(TEXT("Output"), ESearchCase::IgnoreCase))
    {
        SoundCue->FirstNode = SourceNode;
        UE_LOG(LogSoundService, Log, TEXT("Connected '%s' to Sound Cue Output"), *SourceNodeId);
    }
    else
    {
        // Find the target node
        USoundNode* TargetNode = nullptr;
        for (USoundNode* Node : SoundCue->AllNodes)
        {
            if (Node && Node->GetName() == TargetNodeId)
            {
                TargetNode = Node;
                break;
            }
        }

        if (!TargetNode)
        {
            OutError = FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId);
            return false;
        }

        // Validate pin index against max children
        int32 MaxChildren = TargetNode->GetMaxChildNodes();
        if (TargetPinIndex >= MaxChildren)
        {
            OutError = FString::Printf(TEXT("Target pin index %d exceeds max children %d for node type %s"),
                TargetPinIndex, MaxChildren, *TargetNode->GetClass()->GetName());
            return false;
        }

        // Use InsertChildNode to properly add child slots with graph pin synchronization
        // This ensures InputPins.Num() == ChildNodes.Num() is maintained
        while (TargetNode->ChildNodes.Num() <= TargetPinIndex)
        {
            int32 NewChildIndex = TargetNode->ChildNodes.Num();
            TargetNode->InsertChildNode(NewChildIndex);
        }

        // Make the connection
        TargetNode->ChildNodes[TargetPinIndex] = SourceNode;
        UE_LOG(LogSoundService, Log, TEXT("Connected '%s' to '%s' at pin %d"), *SourceNodeId, *TargetNodeId, TargetPinIndex);
    }

#if WITH_EDITOR
    // Update the editor graph to reflect the new connection
    SoundCue->LinkGraphNodesFromSoundNodes();
#endif

    // Save the asset
    FString SaveError;
    if (!SaveAsset(SoundCue, SaveError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save Sound Cue after connecting nodes: %s"), *SaveError);
    }

    return true;
#else
    OutError = TEXT("Sound Cue node connection requires editor data");
    return false;
#endif
}

bool FSoundService::SetSoundCueNodeProperty(const FString& SoundCuePath, const FString& NodeId, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    USoundCue* SoundCue = FindSoundCue(SoundCuePath);
    if (!SoundCue)
    {
        OutError = FString::Printf(TEXT("Sound Cue not found: %s"), *SoundCuePath);
        return false;
    }

    SoundCue->Modify();

#if WITH_EDITORONLY_DATA
    // Find the node
    USoundNode* TargetNode = nullptr;
    for (USoundNode* Node : SoundCue->AllNodes)
    {
        if (Node && Node->GetName() == NodeId)
        {
            TargetNode = Node;
            break;
        }
    }

    if (!TargetNode)
    {
        OutError = FString::Printf(TEXT("Node not found: %s"), *NodeId);
        return false;
    }

    TargetNode->Modify();
    FString PropLower = PropertyName.ToLower();

    // Handle WavePlayer properties
    if (USoundNodeWavePlayer* WavePlayer = Cast<USoundNodeWavePlayer>(TargetNode))
    {
        if (PropLower == TEXT("looping") || PropLower == TEXT("blooping"))
        {
            WavePlayer->bLooping = PropertyValue->AsBool();
        }
        else if (PropLower == TEXT("sound_wave") || PropLower == TEXT("soundwave"))
        {
            FString WavePath = PropertyValue->AsString();
            USoundWave* SoundWave = FindSoundWave(WavePath);
            if (SoundWave)
            {
                WavePlayer->SetSoundWave(SoundWave);
            }
            else
            {
                OutError = FString::Printf(TEXT("Sound wave not found: %s"), *WavePath);
                return false;
            }
        }
        else
        {
            OutError = FString::Printf(TEXT("Unknown property '%s' for WavePlayer node"), *PropertyName);
            return false;
        }
    }
    // Handle Mixer properties
    else if (USoundNodeMixer* Mixer = Cast<USoundNodeMixer>(TargetNode))
    {
        if (PropLower == TEXT("input_volume") || PropLower == TEXT("inputvolume"))
        {
            // Expect an array or index:value pair
            if (PropertyValue->Type == EJson::Array)
            {
                Mixer->InputVolume.Empty();
                for (auto& Val : PropertyValue->AsArray())
                {
                    Mixer->InputVolume.Add(static_cast<float>(Val->AsNumber()));
                }
            }
            else
            {
                OutError = TEXT("input_volume expects an array of floats");
                return false;
            }
        }
        else
        {
            OutError = FString::Printf(TEXT("Unknown property '%s' for Mixer node"), *PropertyName);
            return false;
        }
    }
    // Handle Random properties
    else if (USoundNodeRandom* Random = Cast<USoundNodeRandom>(TargetNode))
    {
        if (PropLower == TEXT("weights"))
        {
            if (PropertyValue->Type == EJson::Array)
            {
                Random->Weights.Empty();
                for (auto& Val : PropertyValue->AsArray())
                {
                    Random->Weights.Add(static_cast<float>(Val->AsNumber()));
                }
            }
            else
            {
                OutError = TEXT("weights expects an array of floats");
                return false;
            }
        }
        else if (PropLower == TEXT("randomize_without_replacement") || PropLower == TEXT("brandomizewithoutreplacement"))
        {
            Random->bRandomizeWithoutReplacement = PropertyValue->AsBool();
        }
        else
        {
            OutError = FString::Printf(TEXT("Unknown property '%s' for Random node"), *PropertyName);
            return false;
        }
    }
    // Handle Modulator properties
    else if (USoundNodeModulator* Modulator = Cast<USoundNodeModulator>(TargetNode))
    {
        if (PropLower == TEXT("pitch_min") || PropLower == TEXT("pitchmin"))
        {
            Modulator->PitchMin = static_cast<float>(PropertyValue->AsNumber());
        }
        else if (PropLower == TEXT("pitch_max") || PropLower == TEXT("pitchmax"))
        {
            Modulator->PitchMax = static_cast<float>(PropertyValue->AsNumber());
        }
        else if (PropLower == TEXT("volume_min") || PropLower == TEXT("volumemin"))
        {
            Modulator->VolumeMin = static_cast<float>(PropertyValue->AsNumber());
        }
        else if (PropLower == TEXT("volume_max") || PropLower == TEXT("volumemax"))
        {
            Modulator->VolumeMax = static_cast<float>(PropertyValue->AsNumber());
        }
        else
        {
            OutError = FString::Printf(TEXT("Unknown property '%s' for Modulator node. Valid: pitch_min, pitch_max, volume_min, volume_max"), *PropertyName);
            return false;
        }
    }
    // Handle Looping properties
    else if (USoundNodeLooping* Looping = Cast<USoundNodeLooping>(TargetNode))
    {
        if (PropLower == TEXT("loop_count") || PropLower == TEXT("loopcount"))
        {
            Looping->LoopCount = static_cast<int32>(PropertyValue->AsNumber());
        }
        else if (PropLower == TEXT("loop_indefinitely") || PropLower == TEXT("bloopindefinitely"))
        {
            Looping->bLoopIndefinitely = PropertyValue->AsBool();
        }
        else
        {
            OutError = FString::Printf(TEXT("Unknown property '%s' for Looping node. Valid: loop_count, loop_indefinitely"), *PropertyName);
            return false;
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Node type '%s' does not support property setting via this interface"), *TargetNode->GetClass()->GetName());
        return false;
    }

    // Save the asset
    FString SaveError;
    if (!SaveAsset(SoundCue, SaveError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save Sound Cue after setting property: %s"), *SaveError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Set property '%s' on node '%s' in Sound Cue: %s"), *PropertyName, *NodeId, *SoundCuePath);
    return true;
#else
    OutError = TEXT("Sound Cue property setting requires editor data");
    return false;
#endif
}

bool FSoundService::RemoveSoundCueNode(const FString& SoundCuePath, const FString& NodeId, FString& OutError)
{
    USoundCue* SoundCue = FindSoundCue(SoundCuePath);
    if (!SoundCue)
    {
        OutError = FString::Printf(TEXT("Sound Cue not found: %s"), *SoundCuePath);
        return false;
    }

    SoundCue->Modify();

#if WITH_EDITORONLY_DATA
    // Find the node to remove
    USoundNode* NodeToRemove = nullptr;
    int32 NodeIndex = INDEX_NONE;
    for (int32 i = 0; i < SoundCue->AllNodes.Num(); i++)
    {
        if (SoundCue->AllNodes[i] && SoundCue->AllNodes[i]->GetName() == NodeId)
        {
            NodeToRemove = SoundCue->AllNodes[i];
            NodeIndex = i;
            break;
        }
    }

    if (!NodeToRemove)
    {
        OutError = FString::Printf(TEXT("Node not found: %s"), *NodeId);
        return false;
    }

    // If this is the first node, clear the reference
    if (SoundCue->FirstNode == NodeToRemove)
    {
        SoundCue->FirstNode = nullptr;
    }

    // Remove references to this node from all other nodes' ChildNodes
    for (USoundNode* OtherNode : SoundCue->AllNodes)
    {
        if (OtherNode && OtherNode != NodeToRemove)
        {
            for (int32 i = 0; i < OtherNode->ChildNodes.Num(); i++)
            {
                if (OtherNode->ChildNodes[i] == NodeToRemove)
                {
                    OtherNode->ChildNodes[i] = nullptr;
                }
            }
        }
    }

    // Remove from AllNodes
    SoundCue->AllNodes.RemoveAt(NodeIndex);

#if WITH_EDITOR
    // Update the editor graph
    SoundCue->LinkGraphNodesFromSoundNodes();
#endif

    // Save the asset
    FString SaveError;
    if (!SaveAsset(SoundCue, SaveError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save Sound Cue after removing node: %s"), *SaveError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Removed node '%s' from Sound Cue: %s"), *NodeId, *SoundCuePath);
    return true;
#else
    OutError = TEXT("Sound Cue node removal requires editor data");
    return false;
#endif
}

bool FSoundService::CompileSoundCue(const FString& SoundCuePath, FString& OutError)
{
    USoundCue* SoundCue = FindSoundCue(SoundCuePath);
    if (!SoundCue)
    {
        OutError = FString::Printf(TEXT("Sound Cue not found: %s"), *SoundCuePath);
        return false;
    }

#if WITH_EDITOR
    SoundCue->Modify();

    // Compile the sound nodes from graph nodes
    SoundCue->CompileSoundNodesFromGraphNodes();

    // Cache aggregate values (duration, max distance, etc.)
    SoundCue->CacheAggregateValues();

    // Validate - check if there's a valid first node
    if (!SoundCue->FirstNode)
    {
        UE_LOG(LogSoundService, Warning, TEXT("Sound Cue '%s' has no connected output (FirstNode is null)"), *SoundCuePath);
    }

    // Save the asset
    FString SaveError;
    if (!SaveAsset(SoundCue, SaveError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save Sound Cue after compile: %s"), *SaveError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Compiled Sound Cue: %s (Duration: %.2fs, MaxDistance: %.2f)"),
        *SoundCuePath, SoundCue->Duration, SoundCue->GetMaxDistance());
    return true;
#else
    OutError = TEXT("Sound Cue compilation requires editor");
    return false;
#endif
}

// ============================================================================
// Sound Class/Mix Operations (Phase 4 - Stubs)
// ============================================================================

USoundClass* FSoundService::CreateSoundClass(const FSoundClassParams& Params, FString& OutAssetPath, FString& OutError)
{
    // TODO: Implement in Phase 4
    OutError = TEXT("Sound Class creation not yet implemented");
    return nullptr;
}

USoundMix* FSoundService::CreateSoundMix(const FSoundMixParams& Params, FString& OutAssetPath, FString& OutError)
{
    // TODO: Implement in Phase 4
    OutError = TEXT("Sound Mix creation not yet implemented");
    return nullptr;
}

bool FSoundService::AddSoundMixModifier(const FString& SoundMixPath, const FString& SoundClassPath, float VolumeAdjust, float PitchAdjust, FString& OutError)
{
    // TODO: Implement in Phase 4
    OutError = TEXT("Sound Mix modifier not yet implemented");
    return false;
}

// ============================================================================
// MetaSound Operations (Phase 3)
// ============================================================================

UMetaSoundSource* FSoundService::CreateMetaSoundSource(const FMetaSoundSourceParams& Params, FString& OutAssetPath, FString& OutError)
{
    // Get the builder subsystem
    UMetaSoundBuilderSubsystem* BuilderSubsystem = UMetaSoundBuilderSubsystem::Get();
    if (!BuilderSubsystem)
    {
        OutError = TEXT("MetaSound Builder Subsystem not available");
        return nullptr;
    }

    // Determine output format
    EMetaSoundOutputAudioFormat OutputFormat = EMetaSoundOutputAudioFormat::Stereo;
    if (Params.OutputFormat.Equals(TEXT("Mono"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::Mono;
    }
    else if (Params.OutputFormat.Equals(TEXT("Quad"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::Quad;
    }
    else if (Params.OutputFormat.Equals(TEXT("FiveDotOne"), ESearchCase::IgnoreCase) || Params.OutputFormat.Equals(TEXT("5.1"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::FiveDotOne;
    }
    else if (Params.OutputFormat.Equals(TEXT("SevenDotOne"), ESearchCase::IgnoreCase) || Params.OutputFormat.Equals(TEXT("7.1"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::SevenDotOne;
    }

    // Create builder name
    FName BuilderName = FName(*FString::Printf(TEXT("MCP_Builder_%s"), *Params.AssetName));

    // Create the source builder
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeOutputHandle OnPlayOutput;
    FMetaSoundBuilderNodeInputHandle OnFinishedInput;
    TArray<FMetaSoundBuilderNodeInputHandle> AudioOutInputs;

    UMetaSoundSourceBuilder* SourceBuilder = BuilderSubsystem->CreateSourceBuilder(
        BuilderName,
        OnPlayOutput,
        OnFinishedInput,
        AudioOutInputs,
        Result,
        OutputFormat,
        Params.bIsOneShot
    );

    if (Result != EMetaSoundBuilderResult::Succeeded || !SourceBuilder)
    {
        OutError = TEXT("Failed to create MetaSound source builder");
        return nullptr;
    }

    // Prevent builder from being garbage collected during asset creation
    SourceBuilder->AddToRoot();

    // Use IAssetTools to create the asset properly (handles package creation and saving)
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    UE_LOG(LogSoundService, Log, TEXT("Creating MetaSound asset '%s' in folder '%s'"), *Params.AssetName, *Params.FolderPath);

    UObject* NewMetaSoundObject = AssetTools.CreateAsset(
        Params.AssetName,
        Params.FolderPath,
        UMetaSoundSource::StaticClass(),
        nullptr  // No factory needed
    );

    if (!NewMetaSoundObject)
    {
        SourceBuilder->RemoveFromRoot();
        BuilderSubsystem->UnregisterBuilder(BuilderName);
        OutError = TEXT("Failed to create MetaSound asset via AssetTools");
        return nullptr;
    }

    UMetaSoundSource* MetaSoundSource = Cast<UMetaSoundSource>(NewMetaSoundObject);
    if (!MetaSoundSource)
    {
        SourceBuilder->RemoveFromRoot();
        BuilderSubsystem->UnregisterBuilder(BuilderName);
        OutError = FString::Printf(TEXT("Created asset is not a MetaSound Source. Actual type: %s"),
            NewMetaSoundObject ? *NewMetaSoundObject->GetClass()->GetName() : TEXT("null"));
        return nullptr;
    }

    // Initialize node locations
    SourceBuilder->InitNodeLocations();

    // Build the MetaSound with the existing asset
    FMetaSoundBuilderOptions BuildOptions;
    BuildOptions.Name = FName(*Params.AssetName);
    BuildOptions.bForceUniqueClassName = true;
    BuildOptions.bAddToRegistry = true;
    BuildOptions.ExistingMetaSound = MetaSoundSource;

    UE_LOG(LogSoundService, Log, TEXT("Building MetaSound '%s'"), *Params.AssetName);

    SourceBuilder->Build(BuildOptions);

    // Get the builder for the new asset and inject template nodes
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundBuilderBase& NewDocBuilder = BuilderRegistry.FindOrBeginBuilding(*MetaSoundSource);

    EMetaSoundBuilderResult InjectResult = EMetaSoundBuilderResult::Failed;
    constexpr bool bForceNodeCreation = true;
    NewDocBuilder.InjectInputTemplateNodes(bForceNodeCreation, InjectResult);

    // Rebuild referenced asset classes
    FMetasoundAssetBase& Asset = NewDocBuilder.GetBuilder().GetMetasoundAsset();
    Asset.RebuildReferencedAssetClasses();

    // Mark asset as modified so it gets saved
    MetaSoundSource->MarkPackageDirty();

    // Save the asset
    if (!SaveAsset(MetaSoundSource, OutError))
    {
        SourceBuilder->RemoveFromRoot();
        BuilderSubsystem->UnregisterBuilder(BuilderName);
        return nullptr;
    }

    OutAssetPath = MetaSoundSource->GetPackage()->GetPathName();
    UE_LOG(LogSoundService, Log, TEXT("Created MetaSound Source: %s"), *OutAssetPath);

    // Clean up
    SourceBuilder->RemoveFromRoot();
    BuilderSubsystem->UnregisterBuilder(BuilderName);

    return MetaSoundSource;
}

bool FSoundService::GetMetaSoundMetadata(const FString& MetaSoundPath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), MetaSound->GetName());
    OutMetadata->SetStringField(TEXT("path"), MetaSoundPath);

    // Get the document interface
    const FMetasoundFrontendDocument& Document = MetaSound->GetConstDocument();

    // Metadata about the root graph
    OutMetadata->SetStringField(TEXT("class_name"), Document.RootGraph.Metadata.GetClassName().ToString());

    // Collect inputs
    TArray<TSharedPtr<FJsonValue>> InputsArray;
    for (const FMetasoundFrontendClassInput& Input : Document.RootGraph.Interface.Inputs)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
        InputObj->SetStringField(TEXT("name"), Input.Name.ToString());
        InputObj->SetStringField(TEXT("type"), Input.TypeName.ToString());
        InputObj->SetStringField(TEXT("node_id"), Input.NodeID.ToString());
        InputObj->SetStringField(TEXT("vertex_id"), Input.VertexID.ToString());
        InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
    }
    OutMetadata->SetArrayField(TEXT("inputs"), InputsArray);

    // Collect outputs
    TArray<TSharedPtr<FJsonValue>> OutputsArray;
    for (const FMetasoundFrontendClassOutput& Output : Document.RootGraph.Interface.Outputs)
    {
        TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
        OutputObj->SetStringField(TEXT("name"), Output.Name.ToString());
        OutputObj->SetStringField(TEXT("type"), Output.TypeName.ToString());
        OutputObj->SetStringField(TEXT("node_id"), Output.NodeID.ToString());
        OutputObj->SetStringField(TEXT("vertex_id"), Output.VertexID.ToString());
        OutputsArray.Add(MakeShared<FJsonValueObject>(OutputObj));
    }
    OutMetadata->SetArrayField(TEXT("outputs"), OutputsArray);

    // Get the default graph page (UE 5.5+ uses PagedGraphs instead of deprecated Graph field)
    const FMetasoundFrontendGraph& DefaultGraph = Document.RootGraph.GetConstDefaultGraph();

    // Collect nodes - need to look up class info from Dependencies
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FMetasoundFrontendNode& Node : DefaultGraph.Nodes)
    {
        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("id"), Node.GetID().ToString());
        NodeObj->SetStringField(TEXT("class_id"), Node.ClassID.ToString());
        NodeObj->SetStringField(TEXT("name"), Node.Name.ToString());

        // Look up class name from dependencies
        for (const FMetasoundFrontendClass& Dependency : Document.Dependencies)
        {
            if (Dependency.ID == Node.ClassID)
            {
                const FMetasoundFrontendClassName& ClassName = Dependency.Metadata.GetClassName();
                NodeObj->SetStringField(TEXT("class_name"), ClassName.Name.ToString());
                NodeObj->SetStringField(TEXT("class_namespace"), ClassName.Namespace.ToString());
                break;
            }
        }

        // Node inputs
        TArray<TSharedPtr<FJsonValue>> NodeInputsArray;
        for (const FMetasoundFrontendVertex& Vertex : Node.Interface.Inputs)
        {
            TSharedPtr<FJsonObject> VertexObj = MakeShared<FJsonObject>();
            VertexObj->SetStringField(TEXT("name"), Vertex.Name.ToString());
            VertexObj->SetStringField(TEXT("type"), Vertex.TypeName.ToString());
            VertexObj->SetStringField(TEXT("id"), Vertex.VertexID.ToString());
            NodeInputsArray.Add(MakeShared<FJsonValueObject>(VertexObj));
        }
        NodeObj->SetArrayField(TEXT("inputs"), NodeInputsArray);

        // Node outputs
        TArray<TSharedPtr<FJsonValue>> NodeOutputsArray;
        for (const FMetasoundFrontendVertex& Vertex : Node.Interface.Outputs)
        {
            TSharedPtr<FJsonObject> VertexObj = MakeShared<FJsonObject>();
            VertexObj->SetStringField(TEXT("name"), Vertex.Name.ToString());
            VertexObj->SetStringField(TEXT("type"), Vertex.TypeName.ToString());
            VertexObj->SetStringField(TEXT("id"), Vertex.VertexID.ToString());
            NodeOutputsArray.Add(MakeShared<FJsonValueObject>(VertexObj));
        }
        NodeObj->SetArrayField(TEXT("outputs"), NodeOutputsArray);

        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }
    OutMetadata->SetArrayField(TEXT("nodes"), NodesArray);

    // Collect edges
    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    for (const FMetasoundFrontendEdge& Edge : DefaultGraph.Edges)
    {
        TSharedPtr<FJsonObject> EdgeObj = MakeShared<FJsonObject>();
        EdgeObj->SetStringField(TEXT("from_node_id"), Edge.FromNodeID.ToString());
        EdgeObj->SetStringField(TEXT("from_vertex_id"), Edge.FromVertexID.ToString());
        EdgeObj->SetStringField(TEXT("to_node_id"), Edge.ToNodeID.ToString());
        EdgeObj->SetStringField(TEXT("to_vertex_id"), Edge.ToVertexID.ToString());
        EdgesArray.Add(MakeShared<FJsonValueObject>(EdgeObj));
    }
    OutMetadata->SetArrayField(TEXT("edges"), EdgesArray);

    OutMetadata->SetNumberField(TEXT("node_count"), DefaultGraph.Nodes.Num());
    OutMetadata->SetNumberField(TEXT("edge_count"), DefaultGraph.Edges.Num());

    UE_LOG(LogSoundService, Log, TEXT("Retrieved metadata for MetaSound: %s"), *MetaSoundPath);
    return true;
}

bool FSoundService::AddMetaSoundNode(const FMetaSoundNodeParams& Params, FString& OutNodeId, FString& OutError)
{
#if WITH_EDITORONLY_DATA
    using namespace Metasound::Frontend;

    UMetaSoundSource* MetaSound = FindMetaSoundSource(Params.MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *Params.MetaSoundPath);
        return false;
    }

    // Get the builder for this MetaSound
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);

    // Create the class name from namespace, name, and variant
    FMetasoundFrontendClassName ClassName;
    ClassName.Namespace = FName(*Params.NodeNamespace);
    ClassName.Name = FName(*Params.NodeClassName);
    if (!Params.NodeVariant.IsEmpty())
    {
        ClassName.Variant = FName(*Params.NodeVariant);
    }

    UE_LOG(LogSoundService, Log, TEXT("Adding node: Namespace='%s', Name='%s', Variant='%s'"),
        *ClassName.Namespace.ToString(), *ClassName.Name.ToString(), *ClassName.Variant.ToString());

    // Mark the MetaSound as modified before making changes
    MetaSound->Modify();

    // Add the node using the builder
    EMetaSoundBuilderResult Result;
    FMetaSoundNodeHandle NodeHandle = Builder.AddNodeByClassName(ClassName, Result, 1);

    if (Result != EMetaSoundBuilderResult::Succeeded || !NodeHandle.IsSet())
    {
        OutError = FString::Printf(TEXT("Failed to add node '%s::%s' (variant: '%s'). Use search_metasound_palette to find valid node names."),
            *Params.NodeNamespace, *Params.NodeClassName, *Params.NodeVariant);
        return false;
    }

    // Get the node ID from the handle
    OutNodeId = NodeHandle.NodeID.ToString();

    // CRITICAL: Set the node's location in the document so it appears in the editor graph
    // Without this, the editor's SynchronizeNodes won't visualize the node
    FVector2D NodeLocation(static_cast<float>(Params.PosX), static_cast<float>(Params.PosY));
    Builder.SetNodeLocation(NodeHandle, NodeLocation, Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to set node location for '%s::%s', node may not appear in editor graph"),
            *Params.NodeNamespace, *Params.NodeClassName);
        // Continue anyway - node was added, just location may be wrong
    }

    // CRITICAL: Mark the node as modified in the document's ModifyContext
    // This signals to the editor graph that a node was added and needs synchronization
    FMetasoundAssetBase* MetaSoundAsset = Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(MetaSound);
    if (MetaSoundAsset)
    {
        MetaSoundAsset->GetModifyContext().AddNodeIDModified(NodeHandle.NodeID);
    }

    // Register the graph with the frontend to synchronize the editor graph
    // This creates the visual editor node representation
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after adding node: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Added node '%s::%s' (ID: %s) to MetaSound: %s"),
        *Params.NodeNamespace, *Params.NodeClassName, *OutNodeId, *Params.MetaSoundPath);

    return true;
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif
}

bool FSoundService::ConnectMetaSoundNodes(const FString& MetaSoundPath, const FString& SourceNodeId, const FString& SourcePinName, const FString& TargetNodeId, const FString& TargetPinName, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Parse node IDs
    FGuid SourceGuid, TargetGuid;
    if (!FGuid::Parse(SourceNodeId, SourceGuid))
    {
        OutError = FString::Printf(TEXT("Invalid source node ID format: %s"), *SourceNodeId);
        return false;
    }
    if (!FGuid::Parse(TargetNodeId, TargetGuid))
    {
        OutError = FString::Printf(TEXT("Invalid target node ID format: %s"), *TargetNodeId);
        return false;
    }

    // Create node handles - use FMetaSoundNodeHandle for generic nodes
    FMetaSoundNodeHandle SourceHandle;
    SourceHandle.NodeID = SourceGuid;

    FMetaSoundNodeHandle TargetHandle;
    TargetHandle.NodeID = TargetGuid;

    // Get output handle from source node
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder.FindNodeOutputByName(SourceHandle, FName(*SourcePinName), Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Source pin '%s' not found on node %s"), *SourcePinName, *SourceNodeId);
        return false;
    }

    // Get input handle from target node
    FMetaSoundBuilderNodeInputHandle InputHandle = Builder.FindNodeInputByName(TargetHandle, FName(*TargetPinName), Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Target pin '%s' not found on node %s"), *TargetPinName, *TargetNodeId);
        return false;
    }

    // Mark the MetaSound as modified before making changes
    MetaSound->Modify();

    // Make the connection
    Builder.ConnectNodes(OutputHandle, InputHandle, Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to connect '%s.%s' to '%s.%s'"),
            *SourceNodeId, *SourcePinName, *TargetNodeId, *TargetPinName);
        return false;
    }

    // Mark both connected nodes as modified for synchronization
    FMetasoundAssetBase* MetaSoundAsset = Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(MetaSound);
    if (MetaSoundAsset)
    {
        MetaSoundAsset->GetModifyContext().AddNodeIDModified(SourceGuid);
        MetaSoundAsset->GetModifyContext().AddNodeIDModified(TargetGuid);
    }

    // Register the graph with the frontend to synchronize the editor graph
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after connecting nodes: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Connected '%s.%s' -> '%s.%s' in MetaSound: %s"),
        *SourceNodeId, *SourcePinName, *TargetNodeId, *TargetPinName, *MetaSoundPath);

    return true;
}

bool FSoundService::SetMetaSoundNodeInput(const FString& MetaSoundPath, const FString& NodeId, const FString& InputName, const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    // Get the builder subsystem for literal creation helpers
    UMetaSoundBuilderSubsystem* BuilderSubsystem = UMetaSoundBuilderSubsystem::Get();
    if (!BuilderSubsystem)
    {
        OutError = TEXT("MetaSound Builder Subsystem not available");
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Parse node ID
    FGuid NodeGuid;
    if (!FGuid::Parse(NodeId, NodeGuid))
    {
        OutError = FString::Printf(TEXT("Invalid node ID format: %s"), *NodeId);
        return false;
    }

    // Create node handle - use FMetaSoundNodeHandle for generic nodes
    FMetaSoundNodeHandle NodeHandle;
    NodeHandle.NodeID = NodeGuid;

    // Find the input
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeInputHandle InputHandle = Builder.FindNodeInputByName(NodeHandle, FName(*InputName), Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Input '%s' not found on node %s"), *InputName, *NodeId);
        return false;
    }

    // Set the value based on JSON type - need FName references for literal creation
    FName DataType;
    if (Value->Type == EJson::Number)
    {
        FMetasoundFrontendLiteral Literal = BuilderSubsystem->CreateFloatMetaSoundLiteral(static_cast<float>(Value->AsNumber()), DataType);
        Builder.SetNodeInputDefault(InputHandle, Literal, Result);
    }
    else if (Value->Type == EJson::Boolean)
    {
        FMetasoundFrontendLiteral Literal = BuilderSubsystem->CreateBoolMetaSoundLiteral(Value->AsBool(), DataType);
        Builder.SetNodeInputDefault(InputHandle, Literal, Result);
    }
    else if (Value->Type == EJson::String)
    {
        FMetasoundFrontendLiteral Literal = BuilderSubsystem->CreateStringMetaSoundLiteral(Value->AsString(), DataType);
        Builder.SetNodeInputDefault(InputHandle, Literal, Result);
    }
    else
    {
        OutError = TEXT("Unsupported value type. Supported: number, boolean, string");
        return false;
    }

    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to set input value for '%s' on node %s"), *InputName, *NodeId);
        return false;
    }

    // CRITICAL: Sync builder document changes to the MetaSound object
    MetaSound->ConformObjectToDocument();

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after setting input: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Set input '%s' on node '%s' in MetaSound: %s"), *InputName, *NodeId, *MetaSoundPath);

    return true;
}

bool FSoundService::AddMetaSoundInput(const FMetaSoundInputParams& Params, FString& OutInputNodeId, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(Params.MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *Params.MetaSoundPath);
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Map data type string to MetaSound type name
    FName DataTypeName;
    if (Params.DataType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Float"));
    }
    else if (Params.DataType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Int32"));
    }
    else if (Params.DataType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Bool"));
    }
    else if (Params.DataType.Equals(TEXT("Trigger"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Trigger"));
    }
    else if (Params.DataType.Equals(TEXT("Audio"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Audio"));
    }
    else if (Params.DataType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("String"));
    }
    else
    {
        // Use the type name as-is for custom types
        DataTypeName = FName(*Params.DataType);
    }

    // Create the default value literal based on data type
    FMetasoundFrontendLiteral DefaultLiteral;
    if (!Params.DefaultValue.IsEmpty())
    {
        if (DataTypeName == FName(TEXT("Float")))
        {
            DefaultLiteral.Set(FCString::Atof(*Params.DefaultValue));
        }
        else if (DataTypeName == FName(TEXT("Int32")))
        {
            DefaultLiteral.Set(FCString::Atoi(*Params.DefaultValue));
        }
        else if (DataTypeName == FName(TEXT("Bool")))
        {
            DefaultLiteral.Set(Params.DefaultValue.ToBool());
        }
        else if (DataTypeName == FName(TEXT("String")))
        {
            DefaultLiteral.Set(Params.DefaultValue);
        }
        // Trigger and Audio types don't have default values
    }

    // Add the input - AddGraphInputNode returns an OUTPUT handle (the output of the input node)
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder.AddGraphInputNode(
        FName(*Params.InputName),
        DataTypeName,
        DefaultLiteral,
        Result,
        false  // Not a constructor input
    );

    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to add input '%s' of type '%s'"), *Params.InputName, *Params.DataType);
        return false;
    }

    OutInputNodeId = OutputHandle.NodeID.ToString();

    // CRITICAL: Create the template input node for visual representation
    // The editor visualizes template nodes (FInputNodeTemplate), not the interface input nodes directly.
    // Without this, the input won't appear in the graph until the asset is reopened.
    FMetaSoundFrontendDocumentBuilder& DocBuilder = Builder.GetBuilder();
    const FMetasoundFrontendNode* TemplateNode = Metasound::Frontend::FInputNodeTemplate::CreateNode(
        DocBuilder,
        FName(*Params.InputName)
    );

    if (TemplateNode)
    {
        // Set location on the TEMPLATE node (this is what the editor visualizes)
        FVector2D NodeLocation(-200.0f, 200.0f);  // Default position for inputs
        DocBuilder.SetNodeLocation(TemplateNode->GetID(), NodeLocation);

        UE_LOG(LogSoundService, Log, TEXT("Created template input node for '%s' with ID: %s"),
            *Params.InputName, *TemplateNode->GetID().ToString());
    }
    else
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to create template input node for '%s' - input may not appear visually"), *Params.InputName);
    }

    // Register the graph with the frontend to synchronize the editor graph
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after adding input: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Added input '%s' (type: %s, ID: %s) to MetaSound: %s"),
        *Params.InputName, *Params.DataType, *OutInputNodeId, *Params.MetaSoundPath);

    return true;
}

bool FSoundService::AddMetaSoundOutput(const FMetaSoundOutputParams& Params, FString& OutOutputNodeId, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(Params.MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *Params.MetaSoundPath);
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Map data type string to MetaSound type name
    FName DataTypeName;
    if (Params.DataType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Float"));
    }
    else if (Params.DataType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Int32"));
    }
    else if (Params.DataType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Bool"));
    }
    else if (Params.DataType.Equals(TEXT("Trigger"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Trigger"));
    }
    else if (Params.DataType.Equals(TEXT("Audio"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Audio"));
    }
    else
    {
        // Use the type name as-is for custom types
        DataTypeName = FName(*Params.DataType);
    }

    // Add the output - AddGraphOutputNode returns an INPUT handle (the input of the output node)
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeInputHandle InputHandle = Builder.AddGraphOutputNode(
        FName(*Params.OutputName),
        DataTypeName,
        FMetasoundFrontendLiteral(),  // Default value (empty for now)
        Result,
        false  // Not a constructor output
    );

    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to add output '%s' of type '%s'"), *Params.OutputName, *Params.DataType);
        return false;
    }

    OutOutputNodeId = InputHandle.NodeID.ToString();

    // Output nodes (class type: Output) are visualized directly, unlike inputs which use template nodes.
    // Set the location using the document builder for the output node.
    FMetaSoundFrontendDocumentBuilder& DocBuilder = Builder.GetBuilder();
    FVector2D NodeLocation(400.0f, 200.0f);  // Default position for outputs (right side)
    DocBuilder.SetNodeLocation(InputHandle.NodeID, NodeLocation);

    UE_LOG(LogSoundService, Log, TEXT("Set location for output node '%s' at (%f, %f)"),
        *Params.OutputName, NodeLocation.X, NodeLocation.Y);

    // Register the graph with the frontend to synchronize the editor graph
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after adding output: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Added output '%s' (type: %s, ID: %s) to MetaSound: %s"),
        *Params.OutputName, *Params.DataType, *OutOutputNodeId, *Params.MetaSoundPath);

    return true;
}

bool FSoundService::CompileMetaSound(const FString& MetaSoundPath, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    // Get the asset base for registration
    FMetasoundAssetBase* AssetBase = Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(MetaSound);
    if (!AssetBase)
    {
        OutError = TEXT("Failed to get MetaSound asset base");
        return false;
    }

    // Compile by updating and registering for execution
    Metasound::Frontend::FMetaSoundAssetRegistrationOptions RegOptions;
    RegOptions.bForceReregister = true;
    AssetBase->UpdateAndRegisterForExecution(RegOptions);

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after compile: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Compiled MetaSound: %s"), *MetaSoundPath);

    return true;
}

bool FSoundService::SearchMetaSoundPalette(const FString& SearchQuery, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutResults, FString& OutError)
{
#if WITH_EDITORONLY_DATA
    using namespace Metasound::Frontend;

    // Get all registered MetaSound node classes
    ISearchEngine& SearchEngine = ISearchEngine::Get();
    TArray<FMetasoundFrontendClass> AllClasses = SearchEngine.FindAllClasses(false); // false = don't include deprecated versions

    FString LowerQuery = SearchQuery.ToLower();
    int32 ResultCount = 0;

    for (const FMetasoundFrontendClass& NodeClass : AllClasses)
    {
        if (MaxResults > 0 && ResultCount >= MaxResults)
        {
            break;
        }

        const FMetasoundFrontendClassMetadata& Metadata = NodeClass.Metadata;
        const FMetasoundFrontendClassName& ClassName = Metadata.GetClassName();

        // Build searchable strings
        FString NameStr = ClassName.Name.ToString();
        FString NamespaceStr = ClassName.Namespace.ToString();
        FString VariantStr = ClassName.Variant.ToString();
        FString DisplayNameStr = Metadata.GetDisplayName().ToString();
        FString DescriptionStr = Metadata.GetDescription().ToString();

        // Build category string from hierarchy
        FString CategoryStr;
        const TArray<FText>& CategoryHierarchy = Metadata.GetCategoryHierarchy();
        for (int32 i = 0; i < CategoryHierarchy.Num(); ++i)
        {
            if (i > 0) CategoryStr += TEXT(" > ");
            CategoryStr += CategoryHierarchy[i].ToString();
        }

        // Build keywords string
        FString KeywordsStr;
        const TArray<FText>& Keywords = Metadata.GetKeywords();
        for (const FText& Keyword : Keywords)
        {
            KeywordsStr += TEXT(" ") + Keyword.ToString();
        }

        // Check if any field matches the search query
        bool bMatches = LowerQuery.IsEmpty() ||
            NameStr.ToLower().Contains(LowerQuery) ||
            NamespaceStr.ToLower().Contains(LowerQuery) ||
            VariantStr.ToLower().Contains(LowerQuery) ||
            DisplayNameStr.ToLower().Contains(LowerQuery) ||
            DescriptionStr.ToLower().Contains(LowerQuery) ||
            CategoryStr.ToLower().Contains(LowerQuery) ||
            KeywordsStr.ToLower().Contains(LowerQuery);

        if (bMatches)
        {
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("namespace"), NamespaceStr);
            ResultObj->SetStringField(TEXT("name"), NameStr);
            ResultObj->SetStringField(TEXT("variant"), VariantStr);
            ResultObj->SetStringField(TEXT("display_name"), DisplayNameStr);
            ResultObj->SetStringField(TEXT("description"), DescriptionStr);
            ResultObj->SetStringField(TEXT("category"), CategoryStr);

            // Build full class name for use with add_metasound_node
            FString FullClassName = NamespaceStr + TEXT("::") + NameStr;
            if (!VariantStr.IsEmpty())
            {
                FullClassName += TEXT(" (") + VariantStr + TEXT(")");
            }
            ResultObj->SetStringField(TEXT("full_name"), FullClassName);

            // Include inputs/outputs info
            const FMetasoundFrontendClassInterface& Interface = NodeClass.GetDefaultInterface();
            TArray<TSharedPtr<FJsonValue>> InputsArray;
            for (const FMetasoundFrontendClassInput& Input : Interface.Inputs)
            {
                TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
                InputObj->SetStringField(TEXT("name"), Input.Name.ToString());
                InputObj->SetStringField(TEXT("type"), Input.TypeName.ToString());
                InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
            }
            ResultObj->SetArrayField(TEXT("inputs"), InputsArray);

            TArray<TSharedPtr<FJsonValue>> OutputsArray;
            for (const FMetasoundFrontendClassOutput& Output : Interface.Outputs)
            {
                TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
                OutputObj->SetStringField(TEXT("name"), Output.Name.ToString());
                OutputObj->SetStringField(TEXT("type"), Output.TypeName.ToString());
                OutputsArray.Add(MakeShared<FJsonValueObject>(OutputObj));
            }
            ResultObj->SetArrayField(TEXT("outputs"), OutputsArray);

            OutResults.Add(ResultObj);
            ResultCount++;
        }
    }

    UE_LOG(LogSoundService, Log, TEXT("MetaSound palette search for '%s' returned %d results"), *SearchQuery, ResultCount);
    return true;
#else
    OutError = TEXT("MetaSound palette search requires editor data");
    return false;
#endif
}

UMetaSoundSource* FSoundService::FindMetaSoundSource(const FString& MetaSoundPath)
{
    return Cast<UMetaSoundSource>(StaticLoadObject(UMetaSoundSource::StaticClass(), nullptr, *MetaSoundPath));
}

// ============================================================================
// Utility Methods
// ============================================================================

USoundWave* FSoundService::FindSoundWave(const FString& SoundWavePath)
{
    return Cast<USoundWave>(StaticLoadObject(USoundWave::StaticClass(), nullptr, *SoundWavePath));
}

USoundCue* FSoundService::FindSoundCue(const FString& SoundCuePath)
{
    return Cast<USoundCue>(StaticLoadObject(USoundCue::StaticClass(), nullptr, *SoundCuePath));
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

bool FSoundService::CreateAssetPackage(const FString& Path, const FString& Name, UPackage*& OutPackage, FString& OutError) const
{
    FString PackagePath = Path / Name;
    if (!PackagePath.StartsWith(TEXT("/")))
    {
        PackagePath = TEXT("/Game/") + PackagePath;
    }

    OutPackage = CreatePackage(*PackagePath);
    if (!OutPackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package: %s"), *PackagePath);
        return false;
    }

    OutPackage->FullyLoad();
    return true;
}

bool FSoundService::SaveAsset(UObject* Asset, FString& OutError) const
{
    if (!Asset)
    {
        OutError = TEXT("Cannot save null asset");
        return false;
    }

    UPackage* Package = Asset->GetOutermost();
    if (!Package)
    {
        OutError = TEXT("Asset has no package");
        return false;
    }

    Package->MarkPackageDirty();

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetPathName(),
        FPackageName::GetAssetPackageExtension()
    );

    // Ensure the directory exists
    FString PackageDir = FPaths::GetPath(PackageFileName);
    if (!IFileManager::Get().DirectoryExists(*PackageDir))
    {
        if (!IFileManager::Get().MakeDirectory(*PackageDir, true))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *PackageDir);
            return false;
        }
    }

    // Check if file is writable (not read-only)
    if (IFileManager::Get().FileExists(*PackageFileName))
    {
        if (IFileManager::Get().IsReadOnly(*PackageFileName))
        {
            OutError = FString::Printf(TEXT("File is read-only: %s"), *PackageFileName);
            return false;
        }
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    // Don't suppress errors so we can diagnose issues
    SaveArgs.SaveFlags = SAVE_None;
    SaveArgs.Error = GError;

    FSavePackageResultStruct SaveResult = UPackage::Save(Package, Asset, *PackageFileName, SaveArgs);
    if (SaveResult.Result != ESavePackageResult::Success)
    {
        FString ResultStr;
        switch (SaveResult.Result)
        {
            case ESavePackageResult::Canceled: ResultStr = TEXT("Save was canceled"); break;
            case ESavePackageResult::Error: ResultStr = TEXT("Save error occurred"); break;
            case ESavePackageResult::MissingFile: ResultStr = TEXT("Missing file"); break;
            case ESavePackageResult::ReplaceCompletely: ResultStr = TEXT("Replace completely failed"); break;
            case ESavePackageResult::ContainsEditorOnlyData: ResultStr = TEXT("Contains editor-only data"); break;
            case ESavePackageResult::ReferencedOnlyByEditorOnlyData: ResultStr = TEXT("Referenced only by editor-only data"); break;
            case ESavePackageResult::DifferentContent: ResultStr = TEXT("Different content"); break;
            default: ResultStr = FString::Printf(TEXT("Unknown error code: %d"), static_cast<int32>(SaveResult.Result)); break;
        }
        OutError = FString::Printf(TEXT("Failed to save package '%s': %s"), *PackageFileName, *ResultStr);
        return false;
    }

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(Asset);

    return true;
}

uint8 FSoundService::GetAttenuationFunction(const FString& FunctionName) const
{
    if (FunctionName.Equals(TEXT("Logarithmic"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::Logarithmic);
    }
    else if (FunctionName.Equals(TEXT("Inverse"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::Inverse);
    }
    else if (FunctionName.Equals(TEXT("LogReverse"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::LogReverse);
    }
    else if (FunctionName.Equals(TEXT("NaturalSound"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::NaturalSound);
    }
    // Default to Linear
    return static_cast<uint8>(EAttenuationDistanceModel::Linear);
}
