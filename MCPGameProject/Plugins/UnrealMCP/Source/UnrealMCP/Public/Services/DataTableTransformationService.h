#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Dom/JsonObject.h"

/**
 * Service responsible for transforming DataTable field names between GUID and friendly formats
 */
class UNREALMCP_API FDataTableTransformationService
{
public:
    /**
     * Transform friendly field names to GUID names for UStruct compatibility
     * Currently unused in Variant A approach but kept for potential future use
     */
    static TSharedPtr<FJsonObject> AutoTransformToGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct);

    /**
     * Transform GUID field names back to friendly names for display/API compatibility
     * Used when reading DataTable data to provide user-friendly field names
     */
    static TSharedPtr<FJsonObject> AutoTransformFromGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct);

    /**
     * Check if field name follows GUID pattern (underscore followed by numbers/long string)
     */
    static bool IsGuidField(const FString& FieldName);

private:
    /**
     * Build mapping from friendly names to GUID names for a struct
     */
    static TMap<FString, FString> BuildFriendlyToGuidMap(const UScriptStruct* Struct);

    /**
     * Build mapping from GUID names to friendly names for a struct
     */
    static TMap<FString, FString> BuildGuidToFriendlyMap(const UScriptStruct* Struct);

    /**
     * Convert PascalCase to camelCase for better API compatibility
     */
    static FString ConvertToCamelCase(const FString& PascalCase);

    /**
     * Extract friendly name from GUID field name by removing GUID suffix
     */
    static FString ExtractFriendlyName(const FString& GuidFieldName);

    /**
     * Transform array elements recursively for nested structs
     */
    static TArray<TSharedPtr<FJsonValue>> TransformArrayToGuidNames(const TArray<TSharedPtr<FJsonValue>>& InputArray, const UScriptStruct* StructType);
    
    /**
     * Transform array elements recursively from GUID names
     */
    static TArray<TSharedPtr<FJsonValue>> TransformArrayFromGuidNames(const TArray<TSharedPtr<FJsonValue>>& InputArray, const UScriptStruct* StructType);
};