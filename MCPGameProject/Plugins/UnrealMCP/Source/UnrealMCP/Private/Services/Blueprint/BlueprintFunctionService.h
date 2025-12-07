#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"

// Forward declarations
class FBlueprintCache;

/**
 * Service for managing Blueprint custom functions and function execution
 * Handles custom function creation, blueprint actor spawning, and function calling
 */
class UNREALMCP_API FBlueprintFunctionService
{
public:
    /**
     * Create a custom blueprint function with parameters
     * @param Blueprint - Blueprint to add function to
     * @param FunctionName - Name of the function
     * @param FunctionParams - JSON object containing function parameters (inputs, outputs, is_pure, category)
     * @param Cache - Blueprint cache for invalidation
     * @param ConvertStringToPinTypeFunc - Function callback to convert type strings to pin types
     * @return true if function was created successfully
     */
    bool CreateCustomBlueprintFunction(
        UBlueprint* Blueprint,
        const FString& FunctionName,
        const TSharedPtr<FJsonObject>& FunctionParams,
        FBlueprintCache& Cache,
        TFunction<bool(const FString&, FEdGraphPinType&)> ConvertStringToPinTypeFunc);

    /**
     * Spawn a blueprint actor in the world
     * @param Blueprint - Blueprint to spawn
     * @param ActorName - Name for the spawned actor
     * @param Location - Spawn location
     * @param Rotation - Spawn rotation
     * @return true if actor was spawned successfully
     */
    bool SpawnBlueprintActor(
        UBlueprint* Blueprint,
        const FString& ActorName,
        const FVector& Location,
        const FRotator& Rotation);

    /**
     * Call a blueprint function
     * @param Blueprint - Blueprint containing the function
     * @param FunctionName - Name of the function to call
     * @param Parameters - Function parameters (simplified)
     * @return true if function was called successfully
     */
    bool CallBlueprintFunction(
        UBlueprint* Blueprint,
        const FString& FunctionName,
        const TArray<FString>& Parameters);
};
