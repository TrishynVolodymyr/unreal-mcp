// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpawnActorNodeCreator.h"
#include "K2Node_SpawnActorFromClass.h"
#include "EdGraph/EdGraph.h"

bool FSpawnActorNodeCreator::IsSpawnActorFromClassRequest(const FString& FunctionName)
{
    // Match various ways users might request this node
    return FunctionName.Equals(TEXT("SpawnActorFromClass"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("Spawn Actor from Class"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("SpawnActor"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("Spawn Actor"), ESearchCase::IgnoreCase);
}

bool FSpawnActorNodeCreator::TryCreateSpawnActorFromClassNode(
    UEdGraph* EventGraph,
    float PositionX,
    float PositionY,
    UEdGraphNode*& NewNode,
    FString& NodeTitle,
    FString& NodeType
)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateSpawnActorFromClassNode: Creating Spawn Actor from Class node"));

    if (!EventGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("TryCreateSpawnActorFromClassNode: EventGraph is null"));
        return false;
    }

    // Create the UK2Node_SpawnActorFromClass node
    UK2Node_SpawnActorFromClass* SpawnNode = NewObject<UK2Node_SpawnActorFromClass>(EventGraph);
    if (!SpawnNode)
    {
        UE_LOG(LogTemp, Error, TEXT("TryCreateSpawnActorFromClassNode: Failed to create UK2Node_SpawnActorFromClass"));
        return false;
    }

    // Position the node
    SpawnNode->NodePosX = PositionX;
    SpawnNode->NodePosY = PositionY;

    // Create a unique GUID for this node
    SpawnNode->CreateNewGuid();

    // Add to graph
    EventGraph->AddNode(SpawnNode, true, true);

    // Initialize the node
    // IMPORTANT: For UK2Node_SpawnActorFromClass, AllocateDefaultPins() must be called BEFORE
    // PostPlacedNewNode() because PostPlacedNewNode() calls GetScaleMethodPin() which expects
    // pins to already exist.
    SpawnNode->AllocateDefaultPins();
    SpawnNode->PostPlacedNewNode();

    // Set output parameters
    NewNode = SpawnNode;
    NodeTitle = TEXT("Spawn Actor from Class");
    NodeType = TEXT("K2Node_SpawnActorFromClass");

    UE_LOG(LogTemp, Warning, TEXT("TryCreateSpawnActorFromClassNode: Successfully created Spawn Actor from Class node"));
    return true;
}
