// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Service for inspecting Blueprint node pin information
 * Provides details about pin types, compatibility, and requirements
 */
class FBlueprintNodePinInfoService
{
public:
    FBlueprintNodePinInfoService();
    ~FBlueprintNodePinInfoService();

    /**
     * Get detailed information about a specific pin on a Blueprint node
     * 
     * @param NodeName Name of the Blueprint node (e.g., "Create Widget", "Get Controller")
     * @param PinName Name of the specific pin (e.g., "Owning Player", "Class", "Return Value")
     * @return JSON string with pin information including:
     *         - pin_type: Type category (object, class, exec, etc.)
     *         - expected_type: Specific type expected
     *         - description: Pin's purpose
     *         - is_required: Whether pin must be connected
     *         - is_input: Whether it's input (true) or output (false)
     * 
     * Example usage:
     * - NodeName="Create Widget", PinName="Class" -> returns Class<UserWidget> info
     * - NodeName="Get Controller", PinName="Target" -> returns Pawn object reference info
     */
    FString GetNodePinInfo(
        const FString& NodeName,
        const FString& PinName
    );

private:
    /**
     * Build JSON result for pin info
     * @param bSuccess Success status
     * @param Message Result message
     * @param PinInfo Optional pin information object
     * @return JSON string
     */
    FString BuildPinInfoResult(
        bool bSuccess,
        const FString& Message,
        TSharedPtr<class FJsonObject> PinInfo = nullptr
    );

    /**
     * Get known pin information from predefined node database
     * @param NodeName Node name to look up
     * @param PinName Pin name to look up
     * @param OutPinInfo Output pin information if found
     * @return true if node and pin are in database
     */
    bool GetKnownPinInfo(
        const FString& NodeName,
        const FString& PinName,
        TSharedPtr<class FJsonObject>& OutPinInfo
    );

    /**
     * List all available pins for a known node
     * @param NodeName Node name to look up
     * @return Array of pin names for the node
     */
    TArray<FString> GetAvailablePinsForNode(const FString& NodeName);
};
