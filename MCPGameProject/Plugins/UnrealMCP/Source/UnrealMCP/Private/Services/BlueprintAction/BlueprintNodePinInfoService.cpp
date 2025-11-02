// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/BlueprintAction/BlueprintNodePinInfoService.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectIterator.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FBlueprintNodePinInfoService::FBlueprintNodePinInfoService()
{
}

FBlueprintNodePinInfoService::~FBlueprintNodePinInfoService()
{
}

FString FBlueprintNodePinInfoService::GetNodePinInfo(
    const FString& NodeName,
    const FString& PinName
)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Looking for pin '%s' on node '%s' using runtime inspection"), *PinName, *NodeName);
    
    // Try to find node in currently loaded Blueprints
    UEdGraphNode* FoundNode = nullptr;
    
    // Search for the node in all loaded Blueprints using runtime inspection
    TArray<UBlueprint*> LoadedBlueprints;
    
    // Get all loaded Blueprint assets
    for (TObjectIterator<UBlueprint> BlueprintItr; BlueprintItr; ++BlueprintItr)
    {
        if (UBlueprint* Blueprint = *BlueprintItr)
        {
            LoadedBlueprints.Add(Blueprint);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Searching through %d loaded Blueprints"), LoadedBlueprints.Num());

    // Search for the node across all loaded Blueprints
    for (UBlueprint* Blueprint : LoadedBlueprints)
    {
        FoundNode = FUnrealMCPCommonUtils::FindNodeInBlueprint(Blueprint, NodeName);
        if (FoundNode)
        {
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Found node '%s' in Blueprint '%s'"), *NodeName, *Blueprint->GetName());
            break;
        }
    }

    if (FoundNode)
    {
        // Get pin information using runtime inspection
        TSharedPtr<FJsonObject> PinInfo = FUnrealMCPCommonUtils::GetNodePinInfoRuntime(FoundNode, PinName);
        
        if (PinInfo->HasField(TEXT("pin_type")))
        {
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), PinInfo);
            ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found pin information for '%s' on node '%s' using runtime inspection"), *PinName, *NodeName));
        }
        else
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), MakeShared<FJsonObject>());
            ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Pin '%s' not found on node '%s'"), *PinName, *NodeName));
            
            // Provide available pins for this node
            TArray<TSharedPtr<FJsonValue>> AvailablePins;
            for (UEdGraphPin* Pin : FoundNode->Pins)
            {
                if (Pin)
                {
                    AvailablePins.Add(MakeShared<FJsonValueString>(Pin->PinName.ToString()));
                }
            }
            ResultObj->SetArrayField(TEXT("available_pins"), AvailablePins);
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Provided %d available pins for node '%s'"), AvailablePins.Num(), *NodeName);
        }
    }
    else
    {
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("node_name"), NodeName);
        ResultObj->SetStringField(TEXT("pin_name"), PinName);
        ResultObj->SetObjectField(TEXT("pin_info"), MakeShared<FJsonObject>());
        ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Node '%s' not found in any loaded Blueprint"), *NodeName));
        
        // Provide list of available node types from loaded Blueprints
        TArray<TSharedPtr<FJsonValue>> AvailableNodes;
        TSet<FString> UniqueNodeNames;
        
        for (UBlueprint* Blueprint : LoadedBlueprints)
        {
            TArray<UEdGraph*> AllGraphs = FUnrealMCPCommonUtils::GetAllGraphsFromBlueprint(Blueprint);
            for (UEdGraph* Graph : AllGraphs)
            {
                if (Graph)
                {
                    for (UEdGraphNode* Node : Graph->Nodes)
                    {
                        if (Node)
                        {
                            FString NodeDisplayName = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                            if (!NodeDisplayName.IsEmpty() && !UniqueNodeNames.Contains(NodeDisplayName))
                            {
                                UniqueNodeNames.Add(NodeDisplayName);
                                AvailableNodes.Add(MakeShared<FJsonValueString>(NodeDisplayName));
                                
                                // Limit the number of examples to avoid overly large responses
                                if (AvailableNodes.Num() >= 50)
                                {
                                    break;
                                }
                            }
                        }
                    }
                    if (AvailableNodes.Num() >= 50)
                    {
                        break;
                    }
                }
            }
            if (AvailableNodes.Num() >= 50)
            {
                break;
            }
        }
        
        ResultObj->SetArrayField(TEXT("available_nodes"), AvailableNodes);
        UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Provided %d example node names from loaded Blueprints"), AvailableNodes.Num());
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Returning JSON response: %s"), *OutputString);
    
    return OutputString;
}

FString FBlueprintNodePinInfoService::BuildPinInfoResult(
    bool bSuccess,
    const FString& Message,
    TSharedPtr<FJsonObject> PinInfo
)
{
    // TODO: Move JSON result building for pin info
    // Pattern used throughout GetNodePinInfo
    
    return FString();
}

bool FBlueprintNodePinInfoService::GetKnownPinInfo(
    const FString& NodeName,
    const FString& PinName,
    TSharedPtr<FJsonObject>& OutPinInfo
)
{
    // TODO: Move the hardcoded pin info database (lines ~1190-1280)
    // This is a large switch/if-else structure with pin information for:
    // - Create Widget node
    // - Get Controller node
    // - Cast nodes
    // - etc.
    
    return false;
}

TArray<FString> FBlueprintNodePinInfoService::GetAvailablePinsForNode(const FString& NodeName)
{
    // TODO: Move logic to list available pins (lines ~1297-1305)
    // Returns array of known pin names for a given node
    
    TArray<FString> AvailablePins;
    return AvailablePins;
}
