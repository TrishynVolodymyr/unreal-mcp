#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Dom/JsonObject.h"

/**
 * Helper class for DataTable GUID field name transformations
 */
class UNREALMCP_API FDataTableTransformationHelper
{
public:
    /**
     * Transform friendly field names to GUID names for UStruct compatibility
     * Currently unused in Variant A approach but kept for potential future use
     */
    static TSharedPtr<FJsonObject> TransformToGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct);
};