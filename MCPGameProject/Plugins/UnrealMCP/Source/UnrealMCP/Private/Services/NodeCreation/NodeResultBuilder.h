// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UEdGraphNode;
class UClass;

/**
 * Service for building JSON result objects for node creation operations.
 * Handles formatting of success/error responses.
 */
class FNodeResultBuilder
{
public:
    /**
     * Build a JSON result for node creation operation.
     * 
     * @param bSuccess - Whether the operation succeeded
     * @param Message - Success or error message
     * @param BlueprintName - Name of the Blueprint
     * @param FunctionName - Name of the function
     * @param NewNode - Created node (if successful)
     * @param NodeTitle - Title of the created node
     * @param NodeType - Type of the created node
     * @param TargetClass - Target class for function calls
     * @param PositionX - X position of the node
     * @param PositionY - Y position of the node
     * @return JSON string with operation result
     */
    static FString BuildNodeResult(
        bool bSuccess,
        const FString& Message,
        const FString& BlueprintName = TEXT(""),
        const FString& FunctionName = TEXT(""),
        UEdGraphNode* NewNode = nullptr,
        const FString& NodeTitle = TEXT(""),
        const FString& NodeType = TEXT(""),
        UClass* TargetClass = nullptr,
        int32 PositionX = 0,
        int32 PositionY = 0,
        const FString& Warning = TEXT("")
    );
};
