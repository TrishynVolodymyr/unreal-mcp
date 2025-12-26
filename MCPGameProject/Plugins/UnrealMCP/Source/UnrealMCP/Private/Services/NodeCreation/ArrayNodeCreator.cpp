// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArrayNodeCreator.h"
#include "K2Node_GetArrayItem.h"
#include "K2Node_CallArrayFunction.h"
#include "Kismet/KismetArrayLibrary.h"
#include "EdGraph/EdGraph.h"

bool FArrayNodeCreator::IsArrayGetOperation(const FString& FunctionName, const FString& ClassName)
{
    return FunctionName.Equals(TEXT("GET"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("Array_Get"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("Get (a ref)"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("Get (a copy)"), ESearchCase::IgnoreCase) ||
           (ClassName.Equals(TEXT("KismetArrayLibrary"), ESearchCase::IgnoreCase) &&
            FunctionName.Contains(TEXT("Get"), ESearchCase::IgnoreCase));
}

bool FArrayNodeCreator::IsArrayLengthOperation(const FString& FunctionName, const FString& ClassName)
{
    return FunctionName.Equals(TEXT("LENGTH"), ESearchCase::IgnoreCase) ||
           FunctionName.Equals(TEXT("Array_Length"), ESearchCase::IgnoreCase) ||
           (ClassName.Equals(TEXT("KismetArrayLibrary"), ESearchCase::IgnoreCase) &&
            FunctionName.Contains(TEXT("Length"), ESearchCase::IgnoreCase));
}

bool FArrayNodeCreator::TryCreateArrayGetNode(
    UEdGraph* EventGraph,
    float PositionX,
    float PositionY,
    UEdGraphNode*& NewNode,
    FString& NodeTitle,
    FString& NodeType
)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArrayGetNode: Creating UK2Node_GetArrayItem directly"));

    UK2Node_GetArrayItem* ArrayGetNode = NewObject<UK2Node_GetArrayItem>(EventGraph);
    if (ArrayGetNode)
    {
        ArrayGetNode->NodePosX = PositionX;
        ArrayGetNode->NodePosY = PositionY;
        ArrayGetNode->CreateNewGuid();
        EventGraph->AddNode(ArrayGetNode, true, true);
        ArrayGetNode->AllocateDefaultPins();
        ArrayGetNode->PostPlacedNewNode();

        NewNode = ArrayGetNode;
        NodeTitle = TEXT("GET");
        NodeType = TEXT("K2Node_GetArrayItem");

        UE_LOG(LogTemp, Warning, TEXT("TryCreateArrayGetNode: Successfully created UK2Node_GetArrayItem"));
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("TryCreateArrayGetNode: Failed to create UK2Node_GetArrayItem"));
    return false;
}

bool FArrayNodeCreator::TryCreateArrayLengthNode(
    UEdGraph* EventGraph,
    float PositionX,
    float PositionY,
    UEdGraphNode*& NewNode,
    FString& NodeTitle,
    FString& NodeType
)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArrayLengthNode: Creating UK2Node_CallArrayFunction directly"));

    // Find the Array_Length function
    UFunction* ArrayLengthFunc = UKismetArrayLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length));
    if (ArrayLengthFunc)
    {
        UK2Node_CallArrayFunction* ArrayLengthNode = NewObject<UK2Node_CallArrayFunction>(EventGraph);
        if (ArrayLengthNode)
        {
            ArrayLengthNode->SetFromFunction(ArrayLengthFunc);
            ArrayLengthNode->NodePosX = PositionX;
            ArrayLengthNode->NodePosY = PositionY;
            ArrayLengthNode->CreateNewGuid();
            EventGraph->AddNode(ArrayLengthNode, true, true);
            ArrayLengthNode->AllocateDefaultPins();
            ArrayLengthNode->PostPlacedNewNode();

            NewNode = ArrayLengthNode;
            NodeTitle = TEXT("LENGTH");
            NodeType = TEXT("K2Node_CallArrayFunction");

            UE_LOG(LogTemp, Warning, TEXT("TryCreateArrayLengthNode: Successfully created UK2Node_CallArrayFunction for Array_Length"));
            return true;
        }
    }

    UE_LOG(LogTemp, Error, TEXT("TryCreateArrayLengthNode: Failed to create Array_Length node"));
    return false;
}
