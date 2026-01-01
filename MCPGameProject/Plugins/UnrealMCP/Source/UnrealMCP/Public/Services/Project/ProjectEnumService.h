#pragma once

#include "CoreMinimal.h"

class UUserDefinedEnum;

/**
 * Service for creating and managing user-defined enums.
 */
class UNREALMCP_API FProjectEnumService
{
public:
    /**
     * Get the singleton instance
     */
    static FProjectEnumService& Get();

    /**
     * Create a new user-defined enum.
     * @param EnumName - Name of the enum to create
     * @param Path - Content browser path for the enum
     * @param Description - Optional description for the enum
     * @param Values - Array of enum value names
     * @param ValueDescriptions - Map of value name to description
     * @param OutFullPath - Output: full path of created enum
     * @param OutError - Output: error message if failed
     * @return true if enum was created successfully
     */
    bool CreateEnum(
        const FString& EnumName,
        const FString& Path,
        const FString& Description,
        const TArray<FString>& Values,
        const TMap<FString, FString>& ValueDescriptions,
        FString& OutFullPath,
        FString& OutError);

    /**
     * Update an existing user-defined enum.
     * Replaces all values with the new set of values.
     * @param EnumName - Name of the enum to update
     * @param Path - Content browser path where the enum exists
     * @param Description - Optional new description for the enum
     * @param Values - New array of enum value names
     * @param ValueDescriptions - Map of value name to description
     * @param OutError - Output: error message if failed
     * @return true if enum was updated successfully
     */
    bool UpdateEnum(
        const FString& EnumName,
        const FString& Path,
        const FString& Description,
        const TArray<FString>& Values,
        const TMap<FString, FString>& ValueDescriptions,
        FString& OutError);

private:
    FProjectEnumService() = default;
};
