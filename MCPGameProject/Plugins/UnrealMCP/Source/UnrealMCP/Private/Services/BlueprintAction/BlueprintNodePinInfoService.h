// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UFunction;

/**
 * Service for inspecting Blueprint node pin information
 * Provides details about pin types, compatibility, and requirements
 *
 * Supports:
 * 1. Runtime inspection of nodes in loaded Blueprints
 * 2. Library function lookup via Blueprint Action Database (Map_Add, Array_Add, etc.)
 */
class FBlueprintNodePinInfoService
{
public:
    FBlueprintNodePinInfoService();
    ~FBlueprintNodePinInfoService();

    /**
     * Get detailed information about a specific pin on a Blueprint node
     *
     * @param NodeName Name of the Blueprint node (e.g., "Create Widget", "Get Controller", "Map Add")
     * @param PinName Name of the specific pin (e.g., "Owning Player", "Class", "TargetMap", "Key", "Value")
     * @param ClassName Optional class name to disambiguate (e.g., "BlueprintMapLibrary", "KismetArrayLibrary")
     * @return JSON string with pin information including:
     *         - pin_type: Type category (object, class, exec, wildcard, etc.)
     *         - expected_type: Specific type expected
     *         - description: Pin's purpose
     *         - is_required: Whether pin must be connected
     *         - is_input: Whether it's input (true) or output (false)
     *         - is_reference: Whether the parameter is passed by reference
     *         - is_wildcard: Whether this is a wildcard pin that resolves on connection
     *
     * Example usage:
     * - NodeName="Create Widget", PinName="Class" -> returns Class<UserWidget> info
     * - NodeName="Map Add", PinName="TargetMap", ClassName="BlueprintMapLibrary" -> returns Map reference info
     * - NodeName="Array Add", PinName="TargetArray", ClassName="KismetArrayLibrary" -> returns Array reference info
     */
    FString GetNodePinInfo(
        const FString& NodeName,
        const FString& PinName,
        const FString& ClassName = TEXT("")
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

    /**
     * Look up pin information for a library function from Blueprint Action Database
     * Used for functions like Map_Add, Array_Add, Set_Add that aren't instantiated in Blueprints
     *
     * @param FunctionName Name of the function (e.g., "Map_Add", "Map Add", "Add")
     * @param PinName Name of the pin/parameter to find
     * @param ClassName Optional class name to filter (e.g., "BlueprintMapLibrary")
     * @param OutPinInfo Output JSON object with pin details
     * @param OutAvailablePins Optional output array of all pin names on the function
     * @return true if function and pin were found
     */
    bool GetLibraryFunctionPinInfo(
        const FString& FunctionName,
        const FString& PinName,
        const FString& ClassName,
        TSharedPtr<class FJsonObject>& OutPinInfo,
        TArray<FString>* OutAvailablePins = nullptr
    );

    /**
     * Extract pin information from a UFunction parameter
     * Converts UE property types to Blueprint pin type information
     */
    TSharedPtr<class FJsonObject> BuildPinInfoFromFunctionParam(
        UFunction* Function,
        FProperty* Property,
        bool bIsReturnValue = false
    );
};
