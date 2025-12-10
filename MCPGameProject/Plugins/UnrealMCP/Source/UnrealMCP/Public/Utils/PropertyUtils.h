#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

/**
 * Property reflection and manipulation utilities
 * Extracted from UnrealMCPCommonUtils for better code organization
 */
class UNREALMCP_API FPropertyUtils
{
public:
    /**
     * Set a UObject property from a JSON value
     * @param Object - UObject to modify
     * @param PropertyName - Name of the property to set
     * @param Value - JSON value to set
     * @param OutErrorMessage - Error message if operation fails
     * @return true if property was set successfully
     */
    static bool SetObjectProperty(UObject* Object, const FString& PropertyName,
                                  const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage);

    /**
     * Set a property value from JSON using FProperty reflection
     * @param Property - FProperty to set
     * @param ContainerPtr - Pointer to the container holding the property
     * @param JsonValue - JSON value to set
     * @return true if property was set successfully
     */
    static bool SetPropertyFromJson(FProperty* Property, void* ContainerPtr,
                                   const TSharedPtr<FJsonValue>& JsonValue);

private:
    // Helper methods used internally by the parsing methods
    static bool ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector);
    static bool ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor);
};
