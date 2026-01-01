#pragma once

#include "CoreMinimal.h"

/**
 * Interface for Blueprint Action services
 * Provides methods for discovering and managing Blueprint actions
 */
class UNREALMCP_API IBlueprintActionService
{
public:
    virtual ~IBlueprintActionService() = default;

    /**
     * Get all available Blueprint actions for a specific pin type
     * @param PinType - The type of pin (object, int, float, bool, string, struct, etc.)
     * @param PinSubCategory - The subcategory/class name for object pins
     * @param SearchFilter - Optional search string to filter results
     * @param MaxResults - Maximum number of results to return
     * @return JSON string containing the actions
     */
    virtual FString GetActionsForPin(const FString& PinType, const FString& PinSubCategory, const FString& SearchFilter, int32 MaxResults) = 0;

    /**
     * Get all available Blueprint actions for a specific class
     * @param ClassName - Name or path of the class to get actions for
     * @param SearchFilter - Optional search string to filter results
     * @param MaxResults - Maximum number of results to return
     * @return JSON string containing the actions
     */
    virtual FString GetActionsForClass(const FString& ClassName, const FString& SearchFilter, int32 MaxResults) = 0;

    /**
     * Get all available Blueprint actions for a class and its entire inheritance hierarchy
     * @param ClassName - Name or path of the class to get actions for
     * @param SearchFilter - Optional search string to filter results
     * @param MaxResults - Maximum number of results to return
     * @return JSON string containing the actions
     */
    virtual FString GetActionsForClassHierarchy(const FString& ClassName, const FString& SearchFilter, int32 MaxResults) = 0;

    /**
     * Search for Blueprint actions using keywords
     * @param SearchQuery - Search string to find actions
     * @param Category - Optional category filter
     * @param MaxResults - Maximum number of results to return
     * @param BlueprintName - Optional name of the Blueprint asset for local variable discovery
     * @return JSON string containing the actions
     */
    virtual FString SearchBlueprintActions(const FString& SearchQuery, const FString& Category, int32 MaxResults, const FString& BlueprintName) = 0;

    /**
     * Get specific information about a Blueprint node's pin including expected types
     * @param NodeName - Name of the Blueprint node (e.g., "Create Widget", "Map Add")
     * @param PinName - Name of the specific pin (e.g., "Class", "TargetMap", "Key")
     * @param ClassName - Optional class name to disambiguate library functions (e.g., "BlueprintMapLibrary")
     * @return JSON string containing pin information including:
     *         - pin_type: Type category (object, class, exec, wildcard, etc.)
     *         - expected_type: Specific type expected
     *         - is_reference: Whether parameter is by-reference
     *         - is_wildcard: Whether pin type resolves on connection
     *         - hint: Usage hints for special pins (wildcard, container)
     */
    virtual FString GetNodePinInfo(const FString& NodeName, const FString& PinName, const FString& ClassName = TEXT("")) = 0;

    /**
     * Create a blueprint node by discovered action/function name
     * @param BlueprintName - Name of the target Blueprint
     * @param FunctionName - Name of the function to create a node for
     * @param ClassName - Optional class name
     * @param NodePosition - Optional position in the graph
     * @param JsonParams - Additional parameters for special nodes
     * @return JSON string containing the result
     */
    virtual FString CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams) = 0;
};