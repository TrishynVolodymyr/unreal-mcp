#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"

// Forward declarations
class FBlueprintCache;

/**
 * Service for managing Blueprint properties, variables, and component properties
 * Handles property setting, variable creation, physics properties, static mesh properties, and pawn properties
 */
class UNREALMCP_API FBlueprintPropertyService
{
public:
    /**
     * Add a variable to a blueprint
     * @param Blueprint - Blueprint to add variable to
     * @param VariableName - Name of the variable
     * @param VariableType - Type of the variable (e.g., "Float", "Actor", "Vector")
     * @param bIsExposed - Whether the variable should be exposed (BlueprintVisible)
     * @param Cache - Blueprint cache for invalidation
     * @return true if variable was added successfully
     */
    bool AddVariableToBlueprint(
        UBlueprint* Blueprint,
        const FString& VariableName,
        const FString& VariableType,
        bool bIsExposed,
        FBlueprintCache& Cache);

    /**
     * Set a blueprint property value
     * @param Blueprint - Blueprint to set property on
     * @param PropertyName - Name of the property
     * @param PropertyValue - JSON value to set
     * @param OutErrorMessage - Error message if operation fails
     * @param Cache - Blueprint cache for invalidation
     * @return true if property was set successfully
     */
    bool SetBlueprintProperty(
        UBlueprint* Blueprint,
        const FString& PropertyName,
        const TSharedPtr<FJsonValue>& PropertyValue,
        FString& OutErrorMessage,
        FBlueprintCache& Cache);

    /**
     * Set physics properties on a component
     * @param Blueprint - Blueprint containing the component
     * @param ComponentName - Name of the component
     * @param PhysicsParams - Physics parameters (mass, linear damping, etc.)
     * @param Cache - Blueprint cache for invalidation
     * @return true if physics properties were set successfully
     */
    bool SetPhysicsProperties(
        UBlueprint* Blueprint,
        const FString& ComponentName,
        const TMap<FString, float>& PhysicsParams,
        FBlueprintCache& Cache);

    /**
     * Set static mesh properties on a component
     * @param Blueprint - Blueprint containing the component
     * @param ComponentName - Name of the component
     * @param StaticMeshPath - Path to the static mesh asset
     * @param Cache - Blueprint cache for invalidation
     * @return true if static mesh properties were set successfully
     */
    bool SetStaticMeshProperties(
        UBlueprint* Blueprint,
        const FString& ComponentName,
        const FString& StaticMeshPath,
        FBlueprintCache& Cache);

    /**
     * Set pawn-specific properties
     * @param Blueprint - Blueprint to set pawn properties on
     * @param PawnParams - Pawn parameters (auto_possess_player, use_controller_rotation_*, etc.)
     * @param Cache - Blueprint cache for invalidation
     * @return true if pawn properties were set successfully
     */
    bool SetPawnProperties(
        UBlueprint* Blueprint,
        const TMap<FString, FString>& PawnParams,
        FBlueprintCache& Cache);

    /**
     * Get all components from a blueprint
     * @param Blueprint - Blueprint to get components from
     * @param OutComponents - Output array of component name/type pairs
     * @return true if components were retrieved successfully
     */
    bool GetBlueprintComponents(
        UBlueprint* Blueprint,
        TArray<TPair<FString, FString>>& OutComponents);

    /**
     * Resolve variable type from string representation
     * @param TypeString - String representation of the type
     * @return UObject pointer for the type, or nullptr if not found
     */
    UObject* ResolveVariableType(const FString& TypeString) const;
};
