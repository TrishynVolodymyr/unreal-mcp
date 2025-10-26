#include "Services/PropertyService.h"
#include "UObject/Field.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"

FPropertyService& FPropertyService::Get()
{
    static FPropertyService Instance;
    return Instance;
}

bool FPropertyService::SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                        const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    if (!Object)
    {
        OutError = TEXT("Invalid object");
        return false;
    }
    
    if (!PropertyValue.IsValid())
    {
        OutError = TEXT("Invalid property value");
        return false;
    }
    
    // Handle special collision properties
    if (HandleCollisionProperty(Object, PropertyName, PropertyValue))
    {
        return true;
    }
    
    // Find the property
    FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), *PropertyName);
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on object '%s' (Class: %s)"), 
                                  *PropertyName, *Object->GetName(), *Object->GetClass()->GetName());
        return false;
    }
    
    // Get property data pointer
    void* PropertyData = Property->ContainerPtrToValuePtr<void>(Object);
    
    // Set the property value
    return SetPropertyFromJson(Property, PropertyData, PropertyValue, OutError);
}

bool FPropertyService::SetObjectProperties(UObject* Object, const TSharedPtr<FJsonObject>& Properties,
                                          TArray<FString>& OutSuccessProperties,
                                          TMap<FString, FString>& OutFailedProperties)
{
    if (!Object || !Properties.IsValid())
    {
        return false;
    }
    
    // Clear output arrays
    OutSuccessProperties.Empty();
    OutFailedProperties.Empty();
    
    // Iterate through all properties
    for (const auto& PropertyPair : Properties->Values)
    {
        const FString& PropertyName = PropertyPair.Key;
        const TSharedPtr<FJsonValue>& PropertyValue = PropertyPair.Value;
        
        FString ErrorMessage;
        if (SetObjectProperty(Object, PropertyName, PropertyValue, ErrorMessage))
        {
            OutSuccessProperties.Add(PropertyName);
        }
        else
        {
            OutFailedProperties.Add(PropertyName, ErrorMessage);
        }
    }
    
    return OutSuccessProperties.Num() > 0;
}

bool FPropertyService::GetObjectProperty(UObject* Object, const FString& PropertyName,
                                        TSharedPtr<FJsonValue>& OutValue, FString& OutError)
{
    if (!Object)
    {
        OutError = TEXT("Invalid object");
        return false;
    }
    
    // Find the property
    FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), *PropertyName);
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on object '%s' (Class: %s)"), 
                                  *PropertyName, *Object->GetName(), *Object->GetClass()->GetName());
        return false;
    }
    
    // Get property data pointer
    const void* PropertyData = Property->ContainerPtrToValuePtr<void>(Object);
    
    // Get the property value as JSON
    return GetPropertyAsJson(Property, PropertyData, OutValue, OutError);
}

bool FPropertyService::HasProperty(UObject* Object, const FString& PropertyName)
{
    if (!Object)
    {
        return false;
    }
    
    FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), *PropertyName);
    return Property != nullptr;
}

TArray<FString> FPropertyService::GetObjectPropertyNames(UObject* Object)
{
    TArray<FString> PropertyNames;
    
    if (!Object)
    {
        return PropertyNames;
    }
    
    // Iterate through all properties of the object's class
    for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property)
        {
            PropertyNames.Add(Property->GetName());
        }
    }
    
    return PropertyNames;
}

bool FPropertyService::SetPropertyFromJson(FProperty* Property, void* PropertyData, 
                                          const TSharedPtr<FJsonValue>& JsonValue, FString& OutError) const
{
    if (!Property || !PropertyData || !JsonValue.IsValid())
    {
        OutError = TEXT("Invalid parameters for property setting");
        return false;
    }
    
    // Special handling for structs and enums that may come as JSON objects
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        return SetStructPropertyFromJson(StructProp, PropertyData, JsonValue, OutError);
    }
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        return SetEnumPropertyFromJson(EnumProp, PropertyData, JsonValue, OutError);
    }
    else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        // Check if it's an enum
        if (ByteProp->Enum)
        {
            return SetByteEnumPropertyFromJson(ByteProp, PropertyData, JsonValue, OutError);
        }
        
        // Regular byte property
        int32 ByteValue;
        if (JsonValue->TryGetNumber(ByteValue))
        {
            ByteProp->SetPropertyValue(PropertyData, static_cast<uint8>(ByteValue));
            return true;
        }
        OutError = TEXT("Expected number value for byte property");
        return false;
    }
    else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
        if (JsonValue->TryGetArray(ArrayValue))
        {
            // This would need more complex implementation for different array element types
            OutError = TEXT("Array property setting not fully implemented");
            return false;
        }
        OutError = TEXT("Expected array value");
        return false;
    }
    
    // Universal fallback: Use Unreal's ImportText for ALL other property types
    // This handles Bool, Int, Float, Double, String, Text, Name, Object references, etc.
    FString ValueString;
    
    // Convert JSON value to string for ImportText
    if (JsonValue->Type == EJson::String)
    {
        JsonValue->TryGetString(ValueString);
    }
    else if (JsonValue->Type == EJson::Number)
    {
        double NumberValue;
        if (JsonValue->TryGetNumber(NumberValue))
        {
            // For integer types, format without decimal point
            if (Property->IsA<FIntProperty>() || 
                Property->IsA<FInt64Property>() ||
                Property->IsA<FInt16Property>() ||
                Property->IsA<FInt8Property>() ||
                Property->IsA<FUInt32Property>() ||
                Property->IsA<FUInt64Property>() ||
                Property->IsA<FUInt16Property>() ||
                Property->IsA<FByteProperty>())
            {
                // Format as integer without decimal
                ValueString = FString::Printf(TEXT("%lld"), static_cast<int64>(NumberValue));
            }
            else
            {
                // Format as float (for Float, Double properties)
                ValueString = FString::SanitizeFloat(NumberValue);
            }
        }
    }
    else if (JsonValue->Type == EJson::Boolean)
    {
        bool BoolValue;
        if (JsonValue->TryGetBool(BoolValue))
        {
            ValueString = BoolValue ? TEXT("True") : TEXT("False");
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Cannot convert JSON type %d to string for ImportText"), 
                                  static_cast<int32>(JsonValue->Type));
        return false;
    }
    
    // Use Unreal's reflection-based ImportText
    const TCHAR* Result = Property->ImportText_Direct(*ValueString, PropertyData, nullptr, PPF_None);
    
    if (Result == nullptr || *Result != '\0')
    {
        OutError = FString::Printf(TEXT("Failed to import value '%s' for property type '%s'"), 
                                  *ValueString, *Property->GetClass()->GetName());
        return false;
    }
    
    return true;
}

bool FPropertyService::GetPropertyAsJson(FProperty* Property, const void* PropertyData,
                                        TSharedPtr<FJsonValue>& OutJsonValue, FString& OutError) const
{
    if (!Property || !PropertyData)
    {
        OutError = TEXT("Invalid parameters for property getting");
        return false;
    }
    
    // Handle different property types
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool BoolValue = BoolProp->GetPropertyValue(PropertyData);
        OutJsonValue = MakeShared<FJsonValueBoolean>(BoolValue);
        return true;
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        int32 IntValue = IntProp->GetPropertyValue(PropertyData);
        OutJsonValue = MakeShared<FJsonValueNumber>(IntValue);
        return true;
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        float FloatValue = FloatProp->GetPropertyValue(PropertyData);
        OutJsonValue = MakeShared<FJsonValueNumber>(FloatValue);
        return true;
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString StringValue = StrProp->GetPropertyValue(PropertyData);
        OutJsonValue = MakeShared<FJsonValueString>(StringValue);
        return true;
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        // Handle common struct types
        if (StructProp->Struct == TBaseStructure<FVector>::Get())
        {
            const FVector* VectorValue = static_cast<const FVector*>(PropertyData);
            TArray<TSharedPtr<FJsonValue>> ArrayValue;
            ArrayValue.Add(MakeShared<FJsonValueNumber>(VectorValue->X));
            ArrayValue.Add(MakeShared<FJsonValueNumber>(VectorValue->Y));
            ArrayValue.Add(MakeShared<FJsonValueNumber>(VectorValue->Z));
            OutJsonValue = MakeShared<FJsonValueArray>(ArrayValue);
            return true;
        }
        else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
        {
            const FRotator* RotatorValue = static_cast<const FRotator*>(PropertyData);
            TArray<TSharedPtr<FJsonValue>> ArrayValue;
            ArrayValue.Add(MakeShared<FJsonValueNumber>(RotatorValue->Pitch));
            ArrayValue.Add(MakeShared<FJsonValueNumber>(RotatorValue->Yaw));
            ArrayValue.Add(MakeShared<FJsonValueNumber>(RotatorValue->Roll));
            OutJsonValue = MakeShared<FJsonValueArray>(ArrayValue);
            return true;
        }
        
        OutError = FString::Printf(TEXT("Unsupported struct type for getting: %s"), *StructProp->Struct->GetName());
        return false;
    }
    
    OutError = FString::Printf(TEXT("Unsupported property type for getting: %s"), *Property->GetClass()->GetName());
    return false;
}

bool FPropertyService::HandleCollisionProperty(UObject* Object, const FString& PropertyName,
                                              const TSharedPtr<FJsonValue>& PropertyValue) const
{
    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(Object);
    if (!PrimComponent)
    {
        return false;
    }
    
    if (PropertyName == TEXT("CollisionEnabled"))
    {
        FString ValueString;
        if (PropertyValue->TryGetString(ValueString))
        {
            ECollisionEnabled::Type CollisionType = ECollisionEnabled::NoCollision;
            if (ValueString == TEXT("NoCollision"))
                CollisionType = ECollisionEnabled::NoCollision;
            else if (ValueString == TEXT("QueryOnly"))
                CollisionType = ECollisionEnabled::QueryOnly;
            else if (ValueString == TEXT("PhysicsOnly"))
                CollisionType = ECollisionEnabled::PhysicsOnly;
            else if (ValueString == TEXT("QueryAndPhysics"))
                CollisionType = ECollisionEnabled::QueryAndPhysics;
            
            PrimComponent->SetCollisionEnabled(CollisionType);
            return true;
        }
    }
    else if (PropertyName == TEXT("CollisionProfileName"))
    {
        FString ValueString;
        if (PropertyValue->TryGetString(ValueString))
        {
            PrimComponent->SetCollisionProfileName(*ValueString);
            return true;
        }
    }
    
    return false;
}

bool FPropertyService::SetEnumPropertyFromJson(FEnumProperty* EnumProp, void* PropertyData,
                                              const TSharedPtr<FJsonValue>& JsonValue, FString& OutError) const
{
    if (!EnumProp || !PropertyData || !JsonValue.IsValid())
    {
        OutError = TEXT("Invalid parameters for enum property setting");
        return false;
    }
    
    UEnum* EnumType = EnumProp->GetEnum();
    if (!EnumType)
    {
        OutError = TEXT("Enum type not found");
        return false;
    }
    
    int64 EnumValue = 0;
    
    // Try as string (e.g., "World", "Screen")
    if (JsonValue->Type == EJson::String)
    {
        FString EnumValueName = JsonValue->AsString();
        
        // Try direct name match
        EnumValue = EnumType->GetValueByNameString(EnumValueName);
        
        // If not found, try with enum prefix (e.g., "World" -> "EWidgetSpace::World")
        if (EnumValue == INDEX_NONE)
        {
            FString FullEnumName = FString::Printf(TEXT("%s::%s"), *EnumType->GetName(), *EnumValueName);
            EnumValue = EnumType->GetValueByNameString(FullEnumName);
        }
        
        if (EnumValue == INDEX_NONE)
        {
            OutError = FString::Printf(TEXT("Invalid enum value '%s' for enum '%s'"), *EnumValueName, *EnumType->GetName());
            return false;
        }
    }
    // Try as number (e.g., 0, 1)
    else if (JsonValue->Type == EJson::Number)
    {
        EnumValue = static_cast<int64>(JsonValue->AsNumber());
        
        // Validate that the number is a valid enum value
        if (!EnumType->IsValidEnumValue(EnumValue))
        {
            OutError = FString::Printf(TEXT("Invalid enum numeric value %lld for enum '%s'"), EnumValue, *EnumType->GetName());
            return false;
        }
    }
    else
    {
        OutError = TEXT("Expected string or number for enum value");
        return false;
    }
    
    // Set the enum value
    FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
    UnderlyingProp->SetIntPropertyValue(PropertyData, EnumValue);
    
    return true;
}

bool FPropertyService::SetByteEnumPropertyFromJson(FByteProperty* ByteProp, void* PropertyData,
                                                   const TSharedPtr<FJsonValue>& JsonValue, FString& OutError) const
{
    if (!ByteProp || !PropertyData || !JsonValue.IsValid())
    {
        OutError = TEXT("Invalid parameters for byte enum property setting");
        return false;
    }
    
    UEnum* EnumType = ByteProp->Enum;
    if (!EnumType)
    {
        OutError = TEXT("Byte property has no associated enum");
        return false;
    }
    
    int64 EnumValue = 0;
    
    // Try as string
    if (JsonValue->Type == EJson::String)
    {
        FString EnumValueName = JsonValue->AsString();
        
        EnumValue = EnumType->GetValueByNameString(EnumValueName);
        
        if (EnumValue == INDEX_NONE)
        {
            FString FullEnumName = FString::Printf(TEXT("%s::%s"), *EnumType->GetName(), *EnumValueName);
            EnumValue = EnumType->GetValueByNameString(FullEnumName);
        }
        
        if (EnumValue == INDEX_NONE)
        {
            OutError = FString::Printf(TEXT("Invalid enum value '%s' for enum '%s'"), *EnumValueName, *EnumType->GetName());
            return false;
        }
    }
    // Try as number
    else if (JsonValue->Type == EJson::Number)
    {
        EnumValue = static_cast<int64>(JsonValue->AsNumber());
        
        if (!EnumType->IsValidEnumValue(EnumValue))
        {
            OutError = FString::Printf(TEXT("Invalid enum numeric value %lld for enum '%s'"), EnumValue, *EnumType->GetName());
            return false;
        }
    }
    else
    {
        OutError = TEXT("Expected string or number for enum value");
        return false;
    }
    
    // Set the byte value
    ByteProp->SetPropertyValue(PropertyData, static_cast<uint8>(EnumValue));
    
    return true;
}

bool FPropertyService::SetStructPropertyFromJson(FStructProperty* StructProp, void* PropertyData,
                                                 const TSharedPtr<FJsonValue>& JsonValue, FString& OutError) const
{
    if (!StructProp || !PropertyData || !JsonValue.IsValid())
    {
        OutError = TEXT("Invalid parameters for struct property setting");
        return false;
    }
    
    UScriptStruct* Struct = StructProp->Struct;
    if (!Struct)
    {
        OutError = TEXT("Struct type not found");
        return false;
    }
    
    FString StructName = Struct->GetName();
    
    // Handle as JSON object {"X": 512, "Y": 512}
    if (JsonValue->Type == EJson::Object)
    {
        TSharedPtr<FJsonObject> StructJson = JsonValue->AsObject();
        
        // Iterate through all fields of the struct and set them
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            FProperty* StructField = *It;
            FString FieldName = StructField->GetName();
            
            if (StructJson->HasField(FieldName))
            {
                void* FieldData = StructField->ContainerPtrToValuePtr<void>(PropertyData);
                FString FieldError;
                
                if (!SetPropertyFromJson(StructField, FieldData, StructJson->Values[FieldName], FieldError))
                {
                    OutError = FString::Printf(TEXT("Failed to set struct field '%s': %s"), *FieldName, *FieldError);
                    return false;
                }
            }
        }
        
        return true;
    }
    // Handle as JSON array for common 2D/3D structs
    else if (JsonValue->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& ArrayValue = JsonValue->AsArray();
        
        // FIntPoint: [X, Y]
        if (StructName == TEXT("IntPoint") && ArrayValue.Num() >= 2)
        {
            FIntPoint TempValue;
            TempValue.X = static_cast<int32>(ArrayValue[0]->AsNumber());
            TempValue.Y = static_cast<int32>(ArrayValue[1]->AsNumber());
            StructProp->CopyCompleteValue(PropertyData, &TempValue);
            return true;
        }
        // FVector2D: [X, Y]
        else if (StructName == TEXT("Vector2D") || StructName == TEXT("Vector2d"))
        {
            if (ArrayValue.Num() >= 2)
            {
                FVector2D TempValue;
                TempValue.X = ArrayValue[0]->AsNumber();
                TempValue.Y = ArrayValue[1]->AsNumber();
                StructProp->CopyCompleteValue(PropertyData, &TempValue);
                return true;
            }
        }
        // FIntVector: [X, Y, Z]
        else if (StructName == TEXT("IntVector") && ArrayValue.Num() >= 3)
        {
            FIntVector TempValue;
            TempValue.X = static_cast<int32>(ArrayValue[0]->AsNumber());
            TempValue.Y = static_cast<int32>(ArrayValue[1]->AsNumber());
            TempValue.Z = static_cast<int32>(ArrayValue[2]->AsNumber());
            StructProp->CopyCompleteValue(PropertyData, &TempValue);
            return true;
        }
        
        OutError = FString::Printf(TEXT("Struct '%s' cannot be set from array format"), *StructName);
        return false;
    }
    
    OutError = FString::Printf(TEXT("Unsupported struct type '%s' or invalid JSON format (expected object or array)"), *StructName);
    return false;
}

