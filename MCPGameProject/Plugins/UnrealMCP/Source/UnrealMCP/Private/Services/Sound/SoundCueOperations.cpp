// SoundCueOperations.cpp - Sound Cue methods for FSoundService
// This file implements the Sound Cue operations of FSoundService

#include "Services/SoundService.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundNode.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundNodeMixer.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeDelay.h"
#include "Sound/SoundNodeConcatenator.h"
#include "Sound/SoundNodeAttenuation.h"
#include "Dom/JsonValue.h"

// ============================================================================
// Sound Cue Operations
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
