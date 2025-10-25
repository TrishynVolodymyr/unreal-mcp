#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Commands/Blueprint/ComponentCreationParams.h"

/**
 * Result of component operation with detailed error message
 */
struct UNREALMCP_API FComponentOperationResult
{
    bool bSuccess;
    FString ErrorMessage;
    
    FComponentOperationResult() 
        : bSuccess(false)
        , ErrorMessage(TEXT(""))
    {}
    
    static FComponentOperationResult Success()
    {
        FComponentOperationResult Result;
        Result.bSuccess = true;
        return Result;
    }
    
    static FComponentOperationResult Failure(const FString& InErrorMessage)
    {
        FComponentOperationResult Result;
        Result.bSuccess = false;
        Result.ErrorMessage = InErrorMessage;
        return Result;
    }
};

/**
 * Interface for component-related operations
 * Provides abstraction for component creation, modification, and management
 */
class UNREALMCP_API IComponentService
{
public:
    virtual ~IComponentService() = default;
    
    /**
     * Add a component to an existing Blueprint
     * @param Blueprint - Target Blueprint
     * @param Params - Component creation parameters
     * @param OutErrorMessage - Detailed error message if operation fails
     * @return true if component was added successfully
     */
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params, FString& OutErrorMessage) = 0;
    
    /**
     * Remove a component from a Blueprint
     * @param Blueprint - Target Blueprint
     * @param ComponentName - Name of the component to remove
     * @return true if component was removed successfully
     */
    virtual bool RemoveComponentFromBlueprint(UBlueprint* Blueprint, const FString& ComponentName) = 0;
    
    /**
     * Find a component in a Blueprint by name
     * @param Blueprint - Target Blueprint
     * @param ComponentName - Name of the component to find
     * @return Component template object or nullptr if not found
     */
    virtual UObject* FindComponentInBlueprint(UBlueprint* Blueprint, const FString& ComponentName) = 0;
    
    /**
     * Get all components in a Blueprint
     * @param Blueprint - Target Blueprint
     * @return Array of component names and types
     */
    virtual TArray<TPair<FString, FString>> GetBlueprintComponents(UBlueprint* Blueprint) = 0;
    
    /**
     * Validate if a component type is supported
     * @param ComponentType - Type of component to validate
     * @return true if component type is supported
     */
    virtual bool IsValidComponentType(const FString& ComponentType) = 0;
    
    /**
     * Get the UClass for a component type string
     * @param ComponentType - String representation of component type
     * @return UClass pointer or nullptr if not found
     */
    virtual UClass* GetComponentClass(const FString& ComponentType) = 0;
};
