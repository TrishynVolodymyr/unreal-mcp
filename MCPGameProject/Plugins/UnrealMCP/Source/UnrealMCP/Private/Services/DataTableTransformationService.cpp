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

    UE_LOG(LogTemp, Log, TEXT("AutoTransformToGuidNames: Processing %d fields for struct '%s'"), InJson->Values.Num(), *RowStruct->GetName());
    
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
                    if ((*PropIt)->GetName() == OutputKey)
                    {
                        ArrayProperty = *PropIt;
                        break;
                    }
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
    
    UE_LOG(LogTemp, Log, TEXT("AutoTransformToGuidNames: Output contains %d fields"), OutJson->Values.Num());
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

    UE_LOG(LogTemp, Log, TEXT("AutoTransformFromGuidNames: Processing %d fields for struct '%s'"), InJson->Values.Num(), *RowStruct->GetName());
    
    TSharedPtr<FJsonObject> OutJson = MakeShared<FJsonObject>();
    
    // Build mapping from GUID names to friendly names for the main struct
    TMap<FString, FString> GuidToFriendlyMap = BuildGuidToFriendlyMap(RowStruct);
    
    // Transform each field in the input JSON
    for (const auto& Pair : InJson->Values)
    {
        FString InputKey = Pair.Key;
        
        FString* FriendlyKeyPtr = GuidToFriendlyMap.Find(InputKey);
        FString OutputKey = FriendlyKeyPtr ? ConvertToCamelCase(*FriendlyKeyPtr) : InputKey;
        
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
                    if ((*PropIt)->GetName() == InputKey)
                    {
                        ArrayProperty = *PropIt;
                        break;
                    }
                }
                
                if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(ArrayProperty))
                {
                    if (const FStructProperty* StructProp = CastField<FStructProperty>(ArrProp->Inner))
                    {
                        // Transform each array element's struct fields
                        TArray<TSharedPtr<FJsonValue>> TransformedArray = TransformArrayFromGuidNames(*InputArray, StructProp->Struct);
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
    
    UE_LOG(LogTemp, Log, TEXT("AutoTransformFromGuidNames: Output contains %d friendly fields"), OutJson->Values.Num());
    return OutJson;
}

TMap<FString, FString> FDataTableTransformationService::BuildFriendlyToGuidMap(const UScriptStruct* Struct)
{
    TMap<FString, FString> FriendlyToGuidMap;
    
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
    }
    
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
    TArray<TSharedPtr<FJsonValue>> TransformedArray;
    
    // Build mapping for the struct fields
    TMap<FString, FString> StructGuidToFriendlyMap = BuildGuidToFriendlyMap(StructType);
    
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
                    FString* StructFriendlyKeyPtr = StructGuidToFriendlyMap.Find(StructInputKey);
                    FString StructOutputKey = StructFriendlyKeyPtr ? ConvertToCamelCase(*StructFriendlyKeyPtr) : StructInputKey;
                    
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