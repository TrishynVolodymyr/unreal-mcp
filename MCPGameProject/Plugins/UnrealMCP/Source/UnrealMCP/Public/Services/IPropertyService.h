#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Interface for property-related operations
 * Provides abstraction for setting and getting properties on UObjects
 */
class UNREALMCP_API IPropertyService
{
public:
    virtual ~IPropertyService() = default;
    
    /**
     * Set a property on a UObject from JSON value
     * @param Object - Target object
     * @param PropertyName - Name of the property to set
     * @param PropertyValue - JSON value to set
     * @param OutError - Error message if operation fails
     * @return true if property was set successfully
     */
    virtual bool SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                  const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;
    
    /**
     * Set multiple properties on a UObject from JSON object
     * @param Object - Target object
     * @param Properties - JSON object containing property name-value pairs
     * @param OutSuccessProperties - List of successfully set properties
     * @param OutFailedProperties - Map of failed properties to error messages
     * @return true if at least one property was set successfully
     */
    virtual bool SetObjectProperties(UObject* Object, const TSharedPtr<FJsonObject>& Properties,
                                   TArray<FString>& OutSuccessProperties,
                                   TMap<FString, FString>& OutFailedProperties) = 0;
    
    /**
     * Get a property value from a UObject as JSON
     * @param Object - Source object
     * @param PropertyName - Name of the property to get
     * @param OutValue - JSON value of the property
     * @param OutError - Error message if operation fails
     * @return true if property was retrieved successfully
     */
    virtual bool GetObjectProperty(UObject* Object, const FString& PropertyName,
                                  TSharedPtr<FJsonValue>& OutValue, FString& OutError) = 0;
    
    /**
     * Validate if a property exists on an object
     * @param Object - Target object
     * @param PropertyName - Name of the property to check
     * @return true if property exists
     */
    virtual bool HasProperty(UObject* Object, const FString& PropertyName) = 0;
    
    /**
     * Get all property names for an object
     * @param Object - Target object
     * @return Array of property names
     */
    virtual TArray<FString> GetObjectPropertyNames(UObject* Object) = 0;
};
