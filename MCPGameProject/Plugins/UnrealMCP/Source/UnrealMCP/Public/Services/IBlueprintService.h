#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"
#include "Commands/Blueprint/ComponentCreationParams.h"

/**
 * Parameters for Blueprint creation operations
 */
struct UNREALMCP_API FBlueprintCreationParams
{
    /** Name of the Blueprint to create */
    FString Name;
    
    /** Folder path where the Blueprint should be created */
    FString FolderPath;
    
    /** Parent class for the Blueprint */
    UClass* ParentClass;
    
    /** Whether to compile the Blueprint after creation */
    bool bCompileOnCreation = true;
    
    /** Default constructor */
    FBlueprintCreationParams()
        : ParentClass(nullptr)
    {
    }
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Interface for Blueprint service operations
 * Provides abstraction for Blueprint creation, modification, and management
 */
class UNREALMCP_API IBlueprintService
{
public:
    virtual ~IBlueprintService() = default;
    
    /**
     * Create a new Blueprint
     * @param Params - Blueprint creation parameters
     * @return Created Blueprint or nullptr if failed
     */
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) = 0;
    
    /**
     * Add a component to an existing Blueprint
     * @param Blueprint - Target Blueprint
     * @param Params - Component creation parameters
     * @param OutErrorMessage - Detailed error message if operation fails
     * @return true if component was added successfully
     */
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const struct FComponentCreationParams& Params, FString& OutErrorMessage) = 0;
    
    /**
     * Compile a Blueprint
     * @param Blueprint - Blueprint to compile
     * @param OutError - Error message if compilation fails
     * @return true if compilation succeeded
     */
    virtual bool CompileBlueprint(UBlueprint* Blueprint, FString& OutError) = 0;
    
    /**
     * Find a Blueprint by name
     * @param BlueprintName - Name of the Blueprint to find
     * @return Found Blueprint or nullptr
     */
    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) = 0;
    
    /**
     * Add a variable to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param VariableName - Name of the variable
     * @param VariableType - Type of the variable
     * @param bIsExposed - Whether the variable should be exposed to the editor
     * @return true if variable was added successfully
     */
    virtual bool AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, const FString& VariableType, bool bIsExposed = false) = 0;
    
    /**
     * Set a property on a Blueprint's default object
     * @param Blueprint - Target Blueprint
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value to set (as JSON)
     * @param OutErrorMessage - Detailed error message if operation fails
     * @return true if property was set successfully
     */
    virtual bool SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutErrorMessage) = 0;
    
    /**
     * Set physics properties on a component in a Blueprint
     * @param Blueprint - Target Blueprint
     * @param ComponentName - Name of the component
     * @param PhysicsParams - Physics parameters to set
     * @return true if physics properties were set successfully
     */
    virtual bool SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TMap<FString, float>& PhysicsParams) = 0;
    
    /**
     * Get list of components in a Blueprint
     * @param Blueprint - Target Blueprint
     * @param OutComponents - Array of component name/type pairs
     * @return true if components were retrieved successfully
     */
    virtual bool GetBlueprintComponents(UBlueprint* Blueprint, TArray<TPair<FString, FString>>& OutComponents) = 0;
    
    /**
     * Set static mesh properties on a component
     * @param Blueprint - Target Blueprint
     * @param ComponentName - Name of the component
     * @param StaticMeshPath - Path to the static mesh asset
     * @return true if static mesh was set successfully
     */
    virtual bool SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath) = 0;
    
    /**
     * Set pawn properties on a Blueprint
     * @param Blueprint - Target Blueprint (must be a Pawn or Character)
     * @param PawnParams - Pawn parameters to set
     * @return true if pawn properties were set successfully
     */
    virtual bool SetPawnProperties(UBlueprint* Blueprint, const TMap<FString, FString>& PawnParams) = 0;
    
    /**
     * Add an interface to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param InterfaceName - Name or path of the interface to add
     * @return true if interface was added successfully
     */
    virtual bool AddInterfaceToBlueprint(UBlueprint* Blueprint, const FString& InterfaceName) = 0;
    
    /**
     * Create a Blueprint interface
     * @param InterfaceName - Name of the interface to create
     * @param FolderPath - Folder path where the interface should be created
     * @return Created Blueprint interface or nullptr if failed
     */
    virtual UBlueprint* CreateBlueprintInterface(const FString& InterfaceName, const FString& FolderPath) = 0;
    
    /**
     * Create a custom function in a Blueprint
     * @param Blueprint - Target Blueprint
     * @param FunctionName - Name of the function to create
     * @param FunctionParams - Function parameters (inputs, outputs, etc.)
     * @return true if function was created successfully
     */
    virtual bool CreateCustomBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TSharedPtr<FJsonObject>& FunctionParams) = 0;
    
    /**
     * Spawn an actor from a Blueprint
     * @param Blueprint - Blueprint to spawn from
     * @param ActorName - Name for the spawned actor
     * @param Location - World location to spawn at
     * @param Rotation - World rotation to spawn with
     * @return true if actor was spawned successfully
     */
    virtual bool SpawnBlueprintActor(UBlueprint* Blueprint, const FString& ActorName, const FVector& Location, const FRotator& Rotation) = 0;
    
    /**
     * Call a function by name on a Blueprint
     * @param Blueprint - Target Blueprint
     * @param FunctionName - Name of the function to call
     * @param Parameters - Function parameters
     * @return true if function was called successfully
     */
    virtual bool CallBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TArray<FString>& Parameters) = 0;
};
