#include "Services/DataTableTransformationService.h"
#include "JsonObjectConverter.h"

TSharedPtr<FJsonObject> FDataTableTransformationService::AutoTransformToGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct)
{
    if (!InJson.IsValid() || !RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: AutoTransformToGuidNames - Invalid input: InJson=%s, RowStruct=%s"), 
               InJson.IsValid() ? TEXT("Valid") : TEXT("Invalid"), 
               RowStruct ? *RowStruct->GetName() : TEXT("Null"));
        return InJson;
    }

    UE_LOG(LogTemp, Warning, TEXT("AutoTransformToGuidNames: Processing %d fields for struct '%s'"), InJson->Values.Num(), *RowStruct->GetName());
    
    TSharedPtr<FJsonObject> OutJson = MakeShared<FJsonObject>();
    
    // Build mapping from friendly names to GUID names for the main struct
    TMap<FString, FString> FriendlyToGuidMap = BuildFriendlyToGuidMap(RowStruct);
    
    // Transform each field in the input JSON
    for (const auto& Pair : InJson->Values)
    {
        FString InputKey = Pair.Key;
        
        // Handle both GUID and friendly field names
        FString OutputKey;
        if (IsGuidField(InputKey))
        {
            // This is already a GUID field, use it directly
            OutputKey = InputKey;
            UE_LOG(LogTemp, Warning, TEXT("AutoTransformToGuidNames: GUID field '%s' -> '%s'"), *InputKey, *OutputKey);
        }
        else
        {
            // This is a friendly field, try to map it to GUID
            FString* GuidKeyPtr = FriendlyToGuidMap.Find(InputKey);
            OutputKey = GuidKeyPtr ? *GuidKeyPtr : InputKey;
            UE_LOG(LogTemp, Warning, TEXT("AutoTransformToGuidNames: Friendly field '%s' -> '%s' (found: %s)"), *InputKey, *OutputKey, GuidKeyPtr ? TEXT("Yes") : TEXT("No"));
        }
        
        if (Pair.Value->Type == EJson::Array)
        {
            // Handle arrays that might contain structs
            const TArray<TSharedPtr<FJsonValue>>* InputArray;
            if (Pair.Value->TryGetArray(InputArray))
            {
                // Find the array property to get its inner struct type
                FProperty* ArrayProperty = nullptr;
                for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
                {
                    // Compare using the property's actual name (which is the GUID name)
                    if ((*PropIt)->GetName() == OutputKey)
                    {
                        ArrayProperty = *PropIt;
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformToGuidNames: Found array property '%s' for output key '%s'"), *(*PropIt)->GetName(), *OutputKey);
                        break;
                    }
                }
                
                if (!ArrayProperty)
                {
                    UE_LOG(LogTemp, Error, TEXT("AutoTransformToGuidNames: Could not find property for output key '%s'"), *OutputKey);
                }
                
                if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(ArrayProperty))
                {
                    if (const FStructProperty* StructProp = CastField<FStructProperty>(ArrProp->Inner))
                    {
                        // Transform each array element's struct fields
                        TArray<TSharedPtr<FJsonValue>> TransformedArray = TransformArrayToGuidNames(*InputArray, StructProp->Struct);
                        OutJson->SetArrayField(OutputKey, TransformedArray);
                    }
                    else
                    {
                        // Non-struct array, copy as-is
                        OutJson->SetArrayField(OutputKey, *InputArray);
                    }
                }
                else
                {
                    // Fallback: copy array as-is
                    OutJson->SetArrayField(OutputKey, *InputArray);
                }
            }
        }
        else
        {
            // Copy non-array fields as-is
            OutJson->SetField(OutputKey, Pair.Value);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AutoTransformToGuidNames: Output contains %d fields"), OutJson->Values.Num());
    return OutJson;
}

TSharedPtr<FJsonObject> FDataTableTransformationService::AutoTransformFromGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct)
{
    if (!InJson.IsValid() || !RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: AutoTransformFromGuidNames - Invalid input: InJson=%s, RowStruct=%s"), 
               InJson.IsValid() ? TEXT("Valid") : TEXT("Invalid"), 
               RowStruct ? *RowStruct->GetName() : TEXT("Null"));
        return InJson;
    }

    UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Processing %d fields for struct '%s'"), InJson->Values.Num(), *RowStruct->GetName());
    
    TSharedPtr<FJsonObject> OutJson = MakeShared<FJsonObject>();
    
    // Build mapping from GUID names to friendly names for the main struct
    TMap<FString, FString> GuidToFriendlyMap = BuildGuidToFriendlyMap(RowStruct);
    TMap<FString, FString> FriendlyToGuidMap = BuildFriendlyToGuidMap(RowStruct);
    
    // Track processed friendly field names to avoid duplicates
    TSet<FString> ProcessedFriendlyFields;
    
    // Transform each field in the input JSON
    for (const auto& Pair : InJson->Values)
    {
        FString InputKey = Pair.Key;
        FString OutputKey;
        bool bShouldProcess = false;
        
        if (IsGuidField(InputKey))
        {
            // This is a GUID field - convert to friendly name
            FString* FriendlyKeyPtr = GuidToFriendlyMap.Find(InputKey);
            OutputKey = FriendlyKeyPtr ? ConvertToCamelCase(*FriendlyKeyPtr) : InputKey;
            bShouldProcess = true;
            
            UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Converting GUID field '%s' -> '%s' (JSON type: %d)"), *InputKey, *OutputKey, (int32)Pair.Value->Type);
            
            // Mark this friendly field as processed to avoid double processing
            if (FriendlyKeyPtr)
            {
                ProcessedFriendlyFields.Add(ConvertToCamelCase(*FriendlyKeyPtr));
            }
        }
        else
        {
            // This might be a friendly field - check if it has a corresponding GUID field in the input
            FString* GuidKeyPtr = FriendlyToGuidMap.Find(InputKey);
            if (GuidKeyPtr && InJson->Values.Contains(*GuidKeyPtr))
            {
                // Both GUID and friendly versions exist - skip friendly to avoid duplicates
                UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Skipping friendly field '%s' - GUID version '%s' already processed"), *InputKey, **GuidKeyPtr);
                continue;
            }
            else if (GuidKeyPtr)
            {
                // Friendly field exists but no GUID version - use it
                OutputKey = InputKey;
                bShouldProcess = true;
                UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Using friendly field '%s' (no GUID version found)"), *InputKey);
            }
            else
            {
                // Unknown field - pass through as-is
                OutputKey = InputKey;
                bShouldProcess = true;
                UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Passing through unknown field '%s'"), *InputKey);
            }
        }
        
        if (!bShouldProcess)
        {
            continue;
        }
        
        if (Pair.Value->Type == EJson::Array)
        {
            UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Processing array field '%s' (confirmed type 5)"), *InputKey);
            // Handle arrays that might contain structs
            const TArray<TSharedPtr<FJsonValue>>* InputArray;
            if (Pair.Value->TryGetArray(InputArray))
            {
                UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Array field '%s' has %d elements"), *InputKey, InputArray->Num());
                
                // Find the array property to get its inner struct type
                FProperty* ArrayProperty = nullptr;
                UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Searching for property matching InputKey '%s'"), *InputKey);
                for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
                {
                    FString PropName = (*PropIt)->GetName();
                    UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Checking property '%s' vs InputKey '%s'"), *PropName, *InputKey);
                    if (PropName == InputKey)
                    {
                        ArrayProperty = *PropIt;
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Found matching property for field '%s'"), *InputKey);
                        break;
                    }
                }
                
                if (!ArrayProperty)
                {
                    UE_LOG(LogTemp, Error, TEXT("AutoTransformFromGuidNames: Could NOT find property for InputKey '%s'"), *InputKey);
                }
                
                if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(ArrayProperty))
                {
                    UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Field '%s' is array property"), *InputKey);
                    if (const FStructProperty* StructProp = CastField<FStructProperty>(ArrProp->Inner))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Array '%s' contains struct '%s'"), *InputKey, *StructProp->Struct->GetName());
                        // Transform each array element's struct fields
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: About to call TransformArrayFromGuidNames with %d input elements for struct '%s'"), InputArray->Num(), *StructProp->Struct->GetName());
                        TArray<TSharedPtr<FJsonValue>> TransformedArray = TransformArrayFromGuidNames(*InputArray, StructProp->Struct);
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: TransformArrayFromGuidNames returned %d elements"), TransformedArray.Num());
                        OutJson->SetArrayField(OutputKey, TransformedArray);
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Transformed struct array '%s' -> '%s' with %d elements"), *InputKey, *OutputKey, TransformedArray.Num());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Array '%s' is primitive type, copying as-is"), *InputKey);
                        // Non-struct array, copy as-is
                        OutJson->SetArrayField(OutputKey, *InputArray);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Could not cast to array property for field '%s', copying as-is"), *InputKey);
                    // Fallback: copy array as-is
                    OutJson->SetArrayField(OutputKey, *InputArray);
                }
            }
        }
        else
        {
            // Copy non-array fields as-is
            UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Copying non-array field '%s' (type: %d) as-is"), *InputKey, (int32)Pair.Value->Type);
            OutJson->SetField(OutputKey, Pair.Value);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AutoTransformFromGuidNames: Output contains %d friendly fields"), OutJson->Values.Num());
    return OutJson;
}

TMap<FString, FString> FDataTableTransformationService::BuildFriendlyToGuidMap(const UScriptStruct* Struct)
{
    TMap<FString, FString> FriendlyToGuidMap;
    
    UE_LOG(LogTemp, Error, TEXT("BuildFriendlyToGuidMap: Processing struct '%s'"), Struct ? *Struct->GetName() : TEXT("NULL"));
    
    for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString FriendlyName = Property->GetDisplayNameText().ToString();
        if (FriendlyName.IsEmpty())
        {
            // Fallback to property name without GUID
            FriendlyName = ExtractFriendlyName(Property->GetName());
        }
        
        FString GuidName = Property->GetName();
        FriendlyToGuidMap.Add(FriendlyName, GuidName);
        
        UE_LOG(LogTemp, Warning, TEXT("BuildFriendlyToGuidMap: Added mapping '%s' -> '%s'"), *FriendlyName, *GuidName);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("BuildFriendlyToGuidMap: Created %d mappings for struct '%s'"), FriendlyToGuidMap.Num(), *Struct->GetName());
    return FriendlyToGuidMap;
}

TMap<FString, FString> FDataTableTransformationService::BuildGuidToFriendlyMap(const UScriptStruct* Struct)
{
    TMap<FString, FString> GuidToFriendlyMap;
    
    for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString FriendlyName = Property->GetDisplayNameText().ToString();
        if (FriendlyName.IsEmpty())
        {
            // Fallback to property name without GUID
            FriendlyName = ExtractFriendlyName(Property->GetName());
        }
        
        FString GuidName = Property->GetName();
        GuidToFriendlyMap.Add(GuidName, FriendlyName);
    }
    
    return GuidToFriendlyMap;
}

FString FDataTableTransformationService::ConvertToCamelCase(const FString& PascalCase)
{
    if (PascalCase.IsEmpty())
    {
        return PascalCase;
    }
    
    FString Result = PascalCase;
    Result[0] = FChar::ToLower(Result[0]);
    return Result;
}

bool FDataTableTransformationService::IsGuidField(const FString& FieldName)
{
    int32 GuidUnderscoreIndex;
    if (FieldName.FindLastChar('_', GuidUnderscoreIndex) && GuidUnderscoreIndex > 0)
    {
        FString Suffix = FieldName.Mid(GuidUnderscoreIndex + 1);
        return Suffix.IsNumeric() || Suffix.Len() > 30; // GUID pattern
    }
    return false;
}

FString FDataTableTransformationService::ExtractFriendlyName(const FString& GuidFieldName)
{
    FString FriendlyName = GuidFieldName;
    
    // Remove GUID part (everything after last underscore)
    int32 LastUnderscoreIndex;
    if (FriendlyName.FindLastChar('_', LastUnderscoreIndex))
    {
        FString BaseName = FriendlyName.Left(LastUnderscoreIndex);
        // Check if this is a GUID pattern (underscore followed by number)
        FString Suffix = FriendlyName.Mid(LastUnderscoreIndex + 1);
        if (Suffix.IsNumeric() || (Suffix.Len() > 30)) // GUID is long
        {
            FriendlyName = BaseName;
        }
    }
    
    return FriendlyName;
}

TArray<TSharedPtr<FJsonValue>> FDataTableTransformationService::TransformArrayToGuidNames(const TArray<TSharedPtr<FJsonValue>>& InputArray, const UScriptStruct* StructType)
{
    UE_LOG(LogTemp, Warning, TEXT("=== TransformArrayToGuidNames: Processing %d elements for struct '%s' ==="), 
           InputArray.Num(), StructType ? *StructType->GetName() : TEXT("NULL"));
    
    TArray<TSharedPtr<FJsonValue>> TransformedArray;
    
    // Build mapping for the struct fields
    TMap<FString, FString> StructFriendlyToGuidMap = BuildFriendlyToGuidMap(StructType);
    
    for (const auto& ArrayElement : InputArray)
    {
        if (ArrayElement->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>* ElementObj;
            if (ArrayElement->TryGetObject(ElementObj))
            {
                TSharedPtr<FJsonObject> TransformedElement = MakeShared<FJsonObject>();
                
                // Transform each field in the struct
                for (const auto& StructPair : (*ElementObj)->Values)
                {
                    FString StructInputKey = StructPair.Key;
                    FString* StructGuidKeyPtr = StructFriendlyToGuidMap.Find(StructInputKey);
                    FString StructOutputKey = StructGuidKeyPtr ? *StructGuidKeyPtr : StructInputKey;
                    
                    TransformedElement->SetField(StructOutputKey, StructPair.Value);
                }
                
                TransformedArray.Add(MakeShared<FJsonValueObject>(TransformedElement));
            }
        }
        else
        {
            // Keep non-object elements as-is
            TransformedArray.Add(ArrayElement);
        }
    }
    
    return TransformedArray;
}

TArray<TSharedPtr<FJsonValue>> FDataTableTransformationService::TransformArrayFromGuidNames(const TArray<TSharedPtr<FJsonValue>>& InputArray, const UScriptStruct* StructType)
{
    UE_LOG(LogTemp, Warning, TEXT("=== TransformArrayFromGuidNames: Processing array with %d elements ==="), InputArray.Num());
    if (StructType)
    {
        UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Struct type: '%s'"), *StructType->GetName());
    }
    
    TArray<TSharedPtr<FJsonValue>> TransformedArray;
    
    // Build mapping for the struct fields
    TMap<FString, FString> StructGuidToFriendlyMap = BuildGuidToFriendlyMap(StructType);
    
    UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Built GUID mapping with %d entries"), StructGuidToFriendlyMap.Num());
    for (const auto& MapPair : StructGuidToFriendlyMap)
    {
        UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: GUID '%s' -> Friendly '%s'"), *MapPair.Key, *MapPair.Value);
    }
    
    for (int32 i = 0; i < InputArray.Num(); ++i)
    {
        const auto& ArrayElement = InputArray[i];
        UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Processing element %d, type: %d"), i, (int32)ArrayElement->Type);
        
        if (ArrayElement->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>* ElementObj;
            if (ArrayElement->TryGetObject(ElementObj))
            {
                UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Element %d is object with %d fields"), i, (*ElementObj)->Values.Num());
                
                TSharedPtr<FJsonObject> TransformedElement = MakeShared<FJsonObject>();
                
                // Transform each field in the struct
                for (const auto& StructPair : (*ElementObj)->Values)
                {
                    FString StructInputKey = StructPair.Key;
                    FString* StructFriendlyKeyPtr = StructGuidToFriendlyMap.Find(StructInputKey);
                    FString StructOutputKey = StructFriendlyKeyPtr ? ConvertToCamelCase(*StructFriendlyKeyPtr) : StructInputKey;
                    
                    UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Field '%s' -> '%s' (found mapping: %s)"), 
                           *StructInputKey, *StructOutputKey, StructFriendlyKeyPtr ? TEXT("Yes") : TEXT("No"));
                    
                    TransformedElement->SetField(StructOutputKey, StructPair.Value);
                }
                
                UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Transformed element %d has %d fields"), i, TransformedElement->Values.Num());
                TransformedArray.Add(MakeShared<FJsonValueObject>(TransformedElement));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("TransformArrayFromGuidNames: Failed to get object from element %d"), i);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Element %d is not object, copying as-is"), i);
            // Keep non-object elements as-is
            TransformedArray.Add(ArrayElement);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TransformArrayFromGuidNames: Returning array with %d elements"), TransformedArray.Num());
    return TransformedArray;
}