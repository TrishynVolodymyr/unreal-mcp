#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint-related MCP commands
 */
class UNREALMCP_API FUnrealMCPBlueprintCommands
{
public:
    FUnrealMCPBlueprintCommands();

    // Handle blueprint commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Specific blueprint command handlers
    TSharedPtr<FJsonObject> HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params);
    /**
     * Set one or more properties on a component in a Blueprint.
     * Params must include:
     *   - blueprint_name (string): Name of the Blueprint
     *   - component_name (string): Name of the component
     *   - kwargs (object): Dictionary of property names and values to set
     * Returns a response with lists of success_properties and failed_properties.
     */
    TSharedPtr<FJsonObject> HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetBlueprintProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetPawnProperties(const TSharedPtr<FJsonObject>& Params);
    // Handler for generic Blueprint function call
    TSharedPtr<FJsonObject> HandleCallFunctionByName(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddInterfaceToBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateBlueprintInterface(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListBlueprintComponents(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateCustomBlueprintFunction(const TSharedPtr<FJsonObject>& Params);

    // Helper functions
    TSharedPtr<FJsonObject> AddComponentToBlueprint(const FString& BlueprintName, const FString& ComponentType, 
                                                   const FString& ComponentName, const FString& MeshType,
                                                   const TArray<float>& Location, const TArray<float>& Rotation,
                                                   const TArray<float>& Scale, const TSharedPtr<FJsonObject>& ComponentProperties);
}; 