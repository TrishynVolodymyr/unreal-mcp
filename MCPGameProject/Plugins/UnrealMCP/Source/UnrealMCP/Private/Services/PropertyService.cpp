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
    
    // Handle different property types
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool BoolValue;
        if (JsonValue->TryGetBool(BoolValue))
        {
            BoolProp->SetPropertyValue(PropertyData, BoolValue);
            return true;
        }
        OutError = TEXT("Expected boolean value");
        return false;
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        int32 IntValue;
        if (JsonValue->TryGetNumber(IntValue))
        {
            IntProp->SetPropertyValue(PropertyData, IntValue);
            return true;
        }
        OutError = TEXT("Expected integer value");
        return false;
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        double DoubleValue;
        if (JsonValue->TryGetNumber(DoubleValue))
        {
            FloatProp->SetPropertyValue(PropertyData, static_cast<float>(DoubleValue));
            return true;
        }
        OutError = TEXT("Expected float value");
        return false;
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        double DoubleValue;
        if (JsonValue->TryGetNumber(DoubleValue))
        {
            DoubleProp->SetPropertyValue(PropertyData, DoubleValue);
            return true;
        }
        OutError = TEXT("Expected double value");
        return false;
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString StringValue;
        if (JsonValue->TryGetString(StringValue))
        {
            StrProp->SetPropertyValue(PropertyData, StringValue);
            return true;
        }
        OutError = TEXT("Expected string value");
        return false;
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        // Handle common struct types
        if (StructProp->Struct == TBaseStructure<FVector>::Get())
        {
            // Try array format [X, Y, Z]
            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
            if (JsonValue->TryGetArray(ArrayValue) && ArrayValue->Num() == 3)
            {
                double X, Y, Z;
                if ((*ArrayValue)[0]->TryGetNumber(X) && 
                    (*ArrayValue)[1]->TryGetNumber(Y) && 
                    (*ArrayValue)[2]->TryGetNumber(Z))
                {
                    FVector VectorValue(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
                    StructProp->CopyCompleteValue(PropertyData, &VectorValue);
                    return true;
                }
            }
            // Try object format {"X": 1.0, "Y": 2.0, "Z": 3.0}
            const TSharedPtr<FJsonObject>* ObjectValue;
            if (JsonValue->TryGetObject(ObjectValue))
            {
                double X = 0.0, Y = 0.0, Z = 0.0;
                (*ObjectValue)->TryGetNumberField(TEXT("X"), X);
                (*ObjectValue)->TryGetNumberField(TEXT("Y"), Y);
                (*ObjectValue)->TryGetNumberField(TEXT("Z"), Z);
                
                FVector VectorValue(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
                StructProp->CopyCompleteValue(PropertyData, &VectorValue);
                return true;
            }
            OutError = TEXT("Expected array [X,Y,Z] or object {X,Y,Z} for Vector");
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
        {
            // Try array format [Pitch, Yaw, Roll]
            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
            if (JsonValue->TryGetArray(ArrayValue) && ArrayValue->Num() == 3)
            {
                double Pitch, Yaw, Roll;
                if ((*ArrayValue)[0]->TryGetNumber(Pitch) && 
                    (*ArrayValue)[1]->TryGetNumber(Yaw) && 
                    (*ArrayValue)[2]->TryGetNumber(Roll))
                {
                    FRotator RotatorValue(static_cast<float>(Pitch), static_cast<float>(Yaw), static_cast<float>(Roll));
                    StructProp->CopyCompleteValue(PropertyData, &RotatorValue);
                    return true;
                }
            }
            // Try object format {"Pitch": 0.0, "Yaw": 45.0, "Roll": 0.0}
            const TSharedPtr<FJsonObject>* ObjectValue;
            if (JsonValue->TryGetObject(ObjectValue))
            {
                double Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
                (*ObjectValue)->TryGetNumberField(TEXT("Pitch"), Pitch);
                (*ObjectValue)->TryGetNumberField(TEXT("Yaw"), Yaw);
                (*ObjectValue)->TryGetNumberField(TEXT("Roll"), Roll);
                
                FRotator RotatorValue(static_cast<float>(Pitch), static_cast<float>(Yaw), static_cast<float>(Roll));
                StructProp->CopyCompleteValue(PropertyData, &RotatorValue);
                return true;
            }
            OutError = TEXT("Expected array [Pitch,Yaw,Roll] or object {Pitch,Yaw,Roll} for Rotator");
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
        {
            const TSharedPtr<FJsonObject>* ObjectValue;
            if (JsonValue->TryGetObject(ObjectValue))
            {
                double R = 0.0, G = 0.0, B = 0.0, A = 1.0;
                (*ObjectValue)->TryGetNumberField(TEXT("R"), R);
                (*ObjectValue)->TryGetNumberField(TEXT("G"), G);
                (*ObjectValue)->TryGetNumberField(TEXT("B"), B);
                (*ObjectValue)->TryGetNumberField(TEXT("A"), A);
                
                FLinearColor ColorValue(static_cast<float>(R), static_cast<float>(G), static_cast<float>(B), static_cast<float>(A));
                StructProp->CopyCompleteValue(PropertyData, &ColorValue);
                return true;
            }
            OutError = TEXT("Expected object with R, G, B, A fields for LinearColor");
            return false;
        }
        
        // Universal struct handling using reflection
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
    
    OutError = FString::Printf(TEXT("Unsupported property type: %s"), *Property->GetClass()->GetName());
    return false;
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

