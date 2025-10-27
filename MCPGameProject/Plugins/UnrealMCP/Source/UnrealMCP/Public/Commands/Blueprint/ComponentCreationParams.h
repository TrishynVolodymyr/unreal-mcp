#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Parameters for component creation operations
 */
struct UNREALMCP_API FComponentCreationParams
{
    /** Name of the component to create */
    FString ComponentName;
    
    /** Type of component to create */
    FString ComponentType;
    
    /** Location of the component */
    TArray<float> Location;
    
    /** Rotation of the component */
    TArray<float> Rotation;
    
    /** Scale of the component */
    TArray<float> Scale;
    
    /** Additional properties for the component */
    TSharedPtr<FJsonObject> ComponentProperties;
    
    /** Name of the parent component to attach to (optional, defaults to RootComponent) */
    FString ParentComponentName;
    
    /** Default constructor */
    FComponentCreationParams()
        : Location({0.0f, 0.0f, 0.0f})
        , Rotation({0.0f, 0.0f, 0.0f})
        , Scale({1.0f, 1.0f, 1.0f})
        , ParentComponentName(TEXT(""))
    {
    }
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};
