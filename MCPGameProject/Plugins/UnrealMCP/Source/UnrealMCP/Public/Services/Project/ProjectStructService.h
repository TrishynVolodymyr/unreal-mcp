#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UUserDefinedStruct;

/**
 * Service for creating and managing user-defined structs.
 * Handles struct creation, property management, and metadata queries.
 */
class UNREALMCP_API FProjectStructService
{
public:
    /**
     * Get the singleton instance
     */
    static FProjectStructService& Get();

    /**
     * Create a new user-defined struct.
     * @param StructName - Name of the struct to create
     * @param Path - Content browser path for the struct
     * @param Description - Optional description for the struct
     * @param Properties - Array of property definitions as JSON objects
     * @param OutFullPath - Output: full path of created struct
     * @param OutError - Output: error message if failed
     * @return true if struct was created successfully
     */
    bool CreateStruct(
        const FString& StructName,
        const FString& Path,
        const FString& Description,
        const TArray<TSharedPtr<FJsonObject>>& Properties,
        FString& OutFullPath,
        FString& OutError);

    /**
     * Update an existing user-defined struct.
     * @param StructName - Name of the struct to update
     * @param Path - Content browser path where the struct exists
     * @param Description - Optional new description
     * @param Properties - New array of property definitions
     * @param OutError - Output: error message if failed
     * @return true if struct was updated successfully
     */
    bool UpdateStruct(
        const FString& StructName,
        const FString& Path,
        const FString& Description,
        const TArray<TSharedPtr<FJsonObject>>& Properties,
        FString& OutError);

    /**
     * Get struct variable information including GUID-based pin names.
     * @param StructName - Name or path of the struct
     * @param Path - Optional path hint for struct discovery
     * @param bOutSuccess - Output: whether operation succeeded
     * @param OutError - Output: error message if failed
     * @return Array of JSON objects describing struct fields
     */
    TArray<TSharedPtr<FJsonObject>> ShowStructVariables(
        const FString& StructName,
        const FString& Path,
        bool& bOutSuccess,
        FString& OutError);

private:
    FProjectStructService() = default;

    /**
     * Create a property on a struct.
     * @param Struct - The struct to add property to
     * @param PropertyObj - JSON object with property definition
     * @return true if property was created successfully
     */
    bool CreateStructProperty(
        UUserDefinedStruct* Struct,
        const TSharedPtr<FJsonObject>& PropertyObj) const;
};
