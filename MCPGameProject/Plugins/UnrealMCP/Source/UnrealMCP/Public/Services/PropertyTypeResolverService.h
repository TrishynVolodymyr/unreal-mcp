#pragma once

#include "CoreMinimal.h"
#include "EdGraphSchema_K2.h"

// Forward declarations
class UScriptStruct;
class UEnum;
class FProperty;

/**
 * Service for resolving property type strings to Unreal Engine pin types.
 * Handles conversion between human-readable type names (like "Integer", "String", "Vector")
 * and Unreal's internal FEdGraphPinType representation.
 *
 * Supports:
 * - Basic types: Boolean, Integer, Float, String, Text, Name
 * - Struct types: Vector, Rotator, Transform, Color
 * - Object references: Texture2D, StaticMesh, Material, SkeletalMesh, SoundBase
 * - Soft object references: SoftObjectPtr<T>
 * - Array types: Type[], Array<Type>
 * - Custom user-defined structs (discovered via Asset Registry)
 */
class UNREALMCP_API FPropertyTypeResolverService
{
public:
    /**
     * Get the singleton instance
     */
    static FPropertyTypeResolverService& Get();

    /**
     * Convert a property type string to an FEdGraphPinType.
     * @param PropertyType - Human-readable type string (e.g., "Integer", "Vector", "MyStruct[]")
     * @param OutPinType - Output pin type
     * @return true if type was resolved successfully
     */
    bool ResolvePropertyType(const FString& PropertyType, FEdGraphPinType& OutPinType) const;

    /**
     * Convert an FProperty to a human-readable type string.
     * @param Property - The property to get type string for
     * @return Human-readable type name
     */
    FString GetPropertyTypeString(const FProperty* Property) const;

    /**
     * Find a custom user-defined struct by name using Asset Registry.
     * Searches multiple name variations (e.g., "MyStruct", "FMyStruct").
     * @param StructName - Name of the struct to find
     * @return Found struct or nullptr
     */
    UScriptStruct* FindCustomStruct(const FString& StructName) const;

    /**
     * Find a custom user-defined enum by name using Asset Registry.
     * Searches with and without E_ prefix (e.g., "E_QuestStatus", "QuestStatus").
     * @param EnumName - Name of the enum to find
     * @return Found enum or nullptr
     */
    UEnum* FindCustomEnum(const FString& EnumName) const;

private:
    /** Private constructor for singleton pattern */
    FPropertyTypeResolverService() = default;

    /**
     * Resolve a base type (non-array) to pin type.
     * @param BaseType - The base type name
     * @param OutPinType - Output pin type
     * @return true if resolved successfully
     */
    bool ResolveBaseType(const FString& BaseType, FEdGraphPinType& OutPinType) const;
};
