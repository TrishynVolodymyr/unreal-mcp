// Copyright Epic Games, Inc. All Rights Reserved.

#include "EnhancedInputActionNodeCreator.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"

bool FEnhancedInputActionNodeCreator::TryCreateEnhancedInputActionNode(
    const FString& ActionName,
    UEdGraph* EventGraph,
    float PositionX,
    float PositionY,
    UEdGraphNode*& NewNode,
    FString& NodeTitle,
    FString& NodeType
)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateEnhancedInputActionNode: Enhanced Input Action requested for '%s'"), *ActionName);

    // Search for the Input Action asset in the Asset Registry
    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FAssetData> ActionAssets;
    AssetRegistry.GetAssetsByClass(UInputAction::StaticClass()->GetClassPathName(), ActionAssets, true);

    for (const FAssetData& ActionAsset : ActionAssets)
    {
        FString AssetName = ActionAsset.AssetName.ToString();

        // Check if this is the action we're looking for
        if (AssetName.Equals(ActionName, ESearchCase::IgnoreCase))
        {
            if (const UInputAction* Action = Cast<const UInputAction>(ActionAsset.GetAsset()))
            {
                UE_LOG(LogTemp, Warning, TEXT("TryCreateEnhancedInputActionNode: Found Enhanced Input Action '%s', creating node"), *AssetName);

                // Create Enhanced Input Action node
                // Note: We can't use the spawner directly because UK2Node_EnhancedInputAction
                // is created dynamically based on available Input Actions
                UK2Node_EnhancedInputAction* InputActionNode = NewObject<UK2Node_EnhancedInputAction>(EventGraph);
                if (InputActionNode)
                {
                    InputActionNode->InputAction = const_cast<UInputAction*>(Action);
                    InputActionNode->NodePosX = PositionX;
                    InputActionNode->NodePosY = PositionY;
                    InputActionNode->CreateNewGuid();
                    EventGraph->AddNode(InputActionNode, true, true);
                    InputActionNode->PostPlacedNewNode();
                    InputActionNode->AllocateDefaultPins();

                    NewNode = InputActionNode;
                    NodeTitle = FString::Printf(TEXT("EnhancedInputAction %s"), *AssetName);
                    NodeType = TEXT("K2Node_EnhancedInputAction");

                    UE_LOG(LogTemp, Warning, TEXT("TryCreateEnhancedInputActionNode: Successfully created Enhanced Input Action node for '%s'"), *AssetName);
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("TryCreateEnhancedInputActionNode: Failed to create UK2Node_EnhancedInputAction for '%s'"), *AssetName);
                    return false;
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("TryCreateEnhancedInputActionNode: Enhanced Input Action '%s' not found in asset registry"), *ActionName);
    return false;
}
