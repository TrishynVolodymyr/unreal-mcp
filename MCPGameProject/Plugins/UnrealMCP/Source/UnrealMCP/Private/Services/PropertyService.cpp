#include "Services/PropertyService.h"
#include "UObject/Field.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

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

    // Set the property value, passing Object as the outer for instanced subobjects
    return SetPropertyFromJson(Property, PropertyData, PropertyValue, OutError, Object);
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
                                          const TSharedPtr<FJsonValue>& JsonValue, FString& OutError,
                                          UObject* Outer) const
{
    if (!Property || !PropertyData || !JsonValue.IsValid())
    {
        OutError = TEXT("Invalid parameters for property setting");
        return false;
    }

    // Special handling for class properties (TSubclassOf<T>)
    if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        FString ClassPath;
        if (!JsonValue->TryGetString(ClassPath))
        {
            OutError = TEXT("Expected string value for class property");
            return false;
        }

        if (ClassPath.IsEmpty())
        {
            // Set to nullptr
            ClassProp->SetObjectPropertyValue(PropertyData, nullptr);
            return true;
        }

        UClass* ClassValue = nullptr;

        // If path starts with /Game/, it's likely a Blueprint - need to get its GeneratedClass
        if (ClassPath.StartsWith(TEXT("/Game/")))
        {
            // Try loading as Blueprint first
            FString BlueprintPath = ClassPath;
            // Remove _C suffix if present to get the Blueprint asset path
            if (BlueprintPath.EndsWith(TEXT("_C")))
            {
                // Path format: /Game/Path/Name.Name_C -> /Game/Path/Name.Name
                int32 DotIndex;
                if (BlueprintPath.FindLastChar('.', DotIndex))
                {
                    BlueprintPath = BlueprintPath.Left(DotIndex);
                }
            }

            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
            if (Blueprint && Blueprint->GeneratedClass)
            {
                ClassValue = Blueprint->GeneratedClass;
                UE_LOG(LogTemp, Log, TEXT("Loaded Blueprint class: %s -> %s"), *ClassPath, *ClassValue->GetName());
            }
            else
            {
                // Try loading the _C path directly as a class
                ClassValue = LoadClass<UObject>(nullptr, *ClassPath);
            }
        }
        else
        {
            // Try loading as a native class path
            ClassValue = LoadClass<UObject>(nullptr, *ClassPath);
        }

        if (!ClassValue)
        {
            OutError = FString::Printf(TEXT("Could not load class from path: %s"), *ClassPath);
            return false;
        }

        // Validate class compatibility with TSubclassOf constraint
        UClass* MetaClass = ClassProp->MetaClass;
        if (MetaClass && !ClassValue->IsChildOf(MetaClass))
        {
            OutError = FString::Printf(TEXT("Class '%s' is not a subclass of '%s'"),
                                      *ClassValue->GetName(), *MetaClass->GetName());
            return false;
        }

        ClassProp->SetObjectPropertyValue(PropertyData, ClassValue);
        return true;
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
            return SetArrayPropertyFromJson(ArrayProp, PropertyData, *ArrayValue, OutError, Outer);
        }
        OutError = TEXT("Expected array value for array property");
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

    // Handle FGameplayTag (accepts string like "DamageType.Physical.Slash")
    if (StructName == TEXT("GameplayTag"))
    {
        if (JsonValue->Type == EJson::String)
        {
            FString TagString = JsonValue->AsString();

            // Handle empty string as invalid/none tag
            if (TagString.IsEmpty())
            {
                FGameplayTag EmptyTag;
                StructProp->CopyCompleteValue(PropertyData, &EmptyTag);
                return true;
            }

            // Request the tag - bErrorIfNotFound=false to avoid assertion
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
            if (Tag.IsValid())
            {
                StructProp->CopyCompleteValue(PropertyData, &Tag);
                UE_LOG(LogTemp, Log, TEXT("Set GameplayTag: %s"), *TagString);
                return true;
            }
            else
            {
                OutError = FString::Printf(TEXT("GameplayTag '%s' is not a valid registered tag"), *TagString);
                return false;
            }
        }
        OutError = TEXT("GameplayTag expected a string value (e.g., \"DamageType.Physical.Slash\")");
        return false;
    }

    // Handle FGameplayTagContainer (accepts array of strings)
    if (StructName == TEXT("GameplayTagContainer"))
    {
        if (JsonValue->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& TagArray = JsonValue->AsArray();
            FGameplayTagContainer TagContainer;

            for (const TSharedPtr<FJsonValue>& TagValue : TagArray)
            {
                if (TagValue->Type == EJson::String)
                {
                    FString TagString = TagValue->AsString();
                    if (!TagString.IsEmpty())
                    {
                        FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
                        if (Tag.IsValid())
                        {
                            TagContainer.AddTag(Tag);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Skipping invalid GameplayTag: %s"), *TagString);
                        }
                    }
                }
            }

            StructProp->CopyCompleteValue(PropertyData, &TagContainer);
            UE_LOG(LogTemp, Log, TEXT("Set GameplayTagContainer with %d tags"), TagContainer.Num());
            return true;
        }
        // Also support single string for single-tag container
        else if (JsonValue->Type == EJson::String)
        {
            FString TagString = JsonValue->AsString();
            FGameplayTagContainer TagContainer;

            if (!TagString.IsEmpty())
            {
                FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
                if (Tag.IsValid())
                {
                    TagContainer.AddTag(Tag);
                }
                else
                {
                    OutError = FString::Printf(TEXT("GameplayTag '%s' is not a valid registered tag"), *TagString);
                    return false;
                }
            }

            StructProp->CopyCompleteValue(PropertyData, &TagContainer);
            return true;
        }
        OutError = TEXT("GameplayTagContainer expected an array of strings or a single string");
        return false;
    }

    // Handle FInstancedStruct (polymorphic struct container)
    // Format: {"StructType": "/Script/Module.StructName", "Field1": value1, "Field2": value2, ...}
    if (StructName == TEXT("InstancedStruct"))
    {
        if (JsonValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> InstancedStructJson = JsonValue->AsObject();

            // Get the struct type
            FString StructTypePath;
            if (!InstancedStructJson->TryGetStringField(TEXT("StructType"), StructTypePath))
            {
                OutError = TEXT("FInstancedStruct requires 'StructType' field specifying the struct path (e.g., '/Script/MyModule.MyStruct')");
                return false;
            }

            // Find the struct type
            UScriptStruct* TargetStruct = FindObject<UScriptStruct>(nullptr, *StructTypePath);
            if (!TargetStruct)
            {
                // Try loading it
                TargetStruct = LoadObject<UScriptStruct>(nullptr, *StructTypePath);
            }
            if (!TargetStruct)
            {
                OutError = FString::Printf(TEXT("Could not find struct type: %s"), *StructTypePath);
                return false;
            }

            // Get the FInstancedStruct pointer
            FInstancedStruct* InstancedStructPtr = static_cast<FInstancedStruct*>(PropertyData);

            // Initialize with the target struct type (allocates memory for the struct)
            InstancedStructPtr->InitializeAs(TargetStruct);

            // Get pointer to the struct data
            void* StructData = InstancedStructPtr->GetMutableMemory();
            if (!StructData)
            {
                OutError = TEXT("Failed to get mutable memory from FInstancedStruct");
                return false;
            }

            // Set each field in the struct (skip the StructType field)
            for (const auto& FieldPair : InstancedStructJson->Values)
            {
                if (FieldPair.Key == TEXT("StructType"))
                {
                    continue; // Skip the type specifier
                }

                // Find the property on the target struct
                FProperty* StructField = FindFProperty<FProperty>(TargetStruct, *FieldPair.Key);
                if (!StructField)
                {
                    UE_LOG(LogTemp, Warning, TEXT("FInstancedStruct: Field '%s' not found on struct '%s', skipping"),
                           *FieldPair.Key, *TargetStruct->GetName());
                    continue;
                }

                void* FieldData = StructField->ContainerPtrToValuePtr<void>(StructData);
                FString FieldError;

                if (!SetPropertyFromJson(StructField, FieldData, FieldPair.Value, FieldError))
                {
                    OutError = FString::Printf(TEXT("Failed to set FInstancedStruct field '%s': %s"), *FieldPair.Key, *FieldError);
                    return false;
                }
            }

            UE_LOG(LogTemp, Log, TEXT("Set FInstancedStruct with type: %s"), *TargetStruct->GetName());
            return true;
        }
        OutError = TEXT("FInstancedStruct expected a JSON object with 'StructType' and field values");
        return false;
    }

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

bool FPropertyService::SetArrayPropertyFromJson(FArrayProperty* ArrayProp, void* PropertyData,
                                                const TArray<TSharedPtr<FJsonValue>>& JsonArray, FString& OutError,
                                                UObject* Outer) const
{
    if (!ArrayProp || !PropertyData)
    {
        OutError = TEXT("Invalid parameters for array property setting");
        return false;
    }

    FProperty* InnerProp = ArrayProp->Inner;
    if (!InnerProp)
    {
        OutError = TEXT("Array inner property not found");
        return false;
    }

    // Check if this is an array of UObject pointers that might need instanced subobject creation
    if (FObjectProperty* InnerObjProp = CastField<FObjectProperty>(InnerProp))
    {
        // Check if the first element is a JSON object with "_class" - indicates instanced object creation
        if (JsonArray.Num() > 0 && JsonArray[0].IsValid() && JsonArray[0]->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>* FirstObject;
            if (JsonArray[0]->TryGetObject(FirstObject) && (*FirstObject)->HasField(TEXT("_class")))
            {
                // This is an instanced object array - delegate to specialized handler
                return SetInstancedObjectArrayFromJson(ArrayProp, PropertyData, JsonArray, Outer, OutError);
            }
        }
    }

    // Use FScriptArrayHelper to manipulate the array
    FScriptArrayHelper ArrayHelper(ArrayProp, PropertyData);

    // Resize the array to match JSON array size
    ArrayHelper.EmptyAndAddValues(JsonArray.Num());

    // Special handling for TArray<FGameplayTag> - very common use case
    FStructProperty* InnerStructProp = CastField<FStructProperty>(InnerProp);
    if (InnerStructProp && InnerStructProp->Struct && InnerStructProp->Struct->GetName() == TEXT("GameplayTag"))
    {
        for (int32 i = 0; i < JsonArray.Num(); ++i)
        {
            const TSharedPtr<FJsonValue>& JsonElement = JsonArray[i];

            if (JsonElement->Type == EJson::String)
            {
                FString TagString = JsonElement->AsString();
                FGameplayTag Tag;

                if (!TagString.IsEmpty())
                {
                    Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
                    if (!Tag.IsValid())
                    {
                        OutError = FString::Printf(TEXT("GameplayTag '%s' at index %d is not a valid registered tag"), *TagString, i);
                        return false;
                    }
                }

                // Get pointer to the array element and copy the tag
                void* ElementData = ArrayHelper.GetRawPtr(i);
                InnerStructProp->CopyCompleteValue(ElementData, &Tag);
            }
            else
            {
                OutError = FString::Printf(TEXT("Expected string value for GameplayTag at index %d"), i);
                return false;
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Set TArray<FGameplayTag> with %d elements"), JsonArray.Num());
        return true;
    }

    // Generic array element setting - recursively use SetPropertyFromJson
    for (int32 i = 0; i < JsonArray.Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& JsonElement = JsonArray[i];

        if (!JsonElement.IsValid())
        {
            OutError = FString::Printf(TEXT("Invalid JSON value at array index %d"), i);
            return false;
        }

        void* ElementData = ArrayHelper.GetRawPtr(i);
        FString ElementError;

        if (!SetPropertyFromJson(InnerProp, ElementData, JsonElement, ElementError))
        {
            OutError = FString::Printf(TEXT("Failed to set array element at index %d: %s"), i, *ElementError);
            return false;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Set array property with %d elements (type: %s)"),
           JsonArray.Num(), *InnerProp->GetClass()->GetName());
    return true;
}

bool FPropertyService::SetInstancedObjectArrayFromJson(FArrayProperty* ArrayProp, void* PropertyData,
                                                       const TArray<TSharedPtr<FJsonValue>>& JsonArray,
                                                       UObject* Outer, FString& OutError) const
{
    if (!ArrayProp || !PropertyData)
    {
        OutError = TEXT("Invalid parameters for instanced object array setting");
        return false;
    }

    if (!Outer)
    {
        OutError = TEXT("Outer object required for creating instanced subobjects");
        return false;
    }

    FObjectProperty* InnerObjProp = CastField<FObjectProperty>(ArrayProp->Inner);
    if (!InnerObjProp)
    {
        OutError = TEXT("Array inner property is not an object property");
        return false;
    }

    // Create array of new objects
    TArray<UObject*> NewObjects;
    NewObjects.Reserve(JsonArray.Num());

    for (int32 i = 0; i < JsonArray.Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& JsonElement = JsonArray[i];

        if (!JsonElement.IsValid() || JsonElement->Type != EJson::Object)
        {
            OutError = FString::Printf(TEXT("Expected JSON object at array index %d for instanced object"), i);
            return false;
        }

        const TSharedPtr<FJsonObject>* ElementObject;
        if (!JsonElement->TryGetObject(ElementObject))
        {
            OutError = FString::Printf(TEXT("Failed to get JSON object at array index %d"), i);
            return false;
        }

        FString ElementError;
        UObject* NewObject = CreateInstancedObjectFromJson(*ElementObject, Outer, ElementError);
        if (!NewObject)
        {
            OutError = FString::Printf(TEXT("Failed to create instanced object at index %d: %s"), i, *ElementError);
            return false;
        }

        NewObjects.Add(NewObject);
    }

    // Use FScriptArrayHelper to set the array
    FScriptArrayHelper ArrayHelper(ArrayProp, PropertyData);
    ArrayHelper.EmptyAndAddValues(NewObjects.Num());

    for (int32 i = 0; i < NewObjects.Num(); ++i)
    {
        void* ElementData = ArrayHelper.GetRawPtr(i);
        InnerObjProp->SetObjectPropertyValue(ElementData, NewObjects[i]);
    }

    UE_LOG(LogTemp, Log, TEXT("Set instanced object array with %d elements on '%s'"),
           NewObjects.Num(), *Outer->GetName());
    return true;
}

UObject* FPropertyService::CreateInstancedObjectFromJson(const TSharedPtr<FJsonObject>& JsonObject,
                                                         UObject* Outer, FString& OutError) const
{
    if (!JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON object for instanced object creation");
        return nullptr;
    }

    if (!Outer)
    {
        OutError = TEXT("Outer object required for creating instanced subobject");
        return nullptr;
    }

    // Get the class path from "_class" field
    FString ClassPath;
    if (!JsonObject->TryGetStringField(TEXT("_class"), ClassPath))
    {
        OutError = TEXT("Missing '_class' field in JSON object for instanced object creation");
        return nullptr;
    }

    // Resolve the class
    UClass* ObjectClass = nullptr;

    // Try loading as a class directly
    ObjectClass = LoadClass<UObject>(nullptr, *ClassPath);

    // If that failed, try with /Script/ prefix variations
    if (!ObjectClass)
    {
        // Try common module paths
        // Dynamically get the game module path from project name
        FString GameModulePath = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());

        TArray<FString> ModulePaths = {
            TEXT("/Script/Engine"),
            TEXT("/Script/CoreUObject"),
            GameModulePath,  // Project module - dynamically resolved
            TEXT("/Script/GameplayAbilities"),
        };

        // Extract just the class name if it has a module path already
        FString ClassName = ClassPath;
        if (ClassPath.Contains(TEXT(".")))
        {
            ClassPath.Split(TEXT("."), nullptr, &ClassName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        }

        for (const FString& ModulePath : ModulePaths)
        {
            FString FullPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *ClassName);
            ObjectClass = LoadClass<UObject>(nullptr, *FullPath);
            if (ObjectClass)
            {
                UE_LOG(LogTemp, Log, TEXT("Resolved class '%s' to '%s'"), *ClassPath, *FullPath);
                break;
            }
        }
    }

    if (!ObjectClass)
    {
        OutError = FString::Printf(TEXT("Could not resolve class '%s'"), *ClassPath);
        return nullptr;
    }

    // Create a unique name for the subobject
    FName SubObjectName = MakeUniqueObjectName(Outer, ObjectClass, *ObjectClass->GetName());

    // Create the new object as a subobject of Outer
    UObject* CreatedObject = ::NewObject<UObject>(Outer, ObjectClass, SubObjectName, RF_DefaultSubObject);
    if (!CreatedObject)
    {
        OutError = FString::Printf(TEXT("Failed to create instance of class '%s'"), *ObjectClass->GetName());
        return nullptr;
    }

    // Set properties on the new object (skip "_class" field)
    TArray<FString> SuccessProps;
    TMap<FString, FString> FailedProps;

    // Use a non-const reference to this service for setting properties
    FPropertyService& MutableService = const_cast<FPropertyService&>(*this);

    for (const auto& PropertyPair : JsonObject->Values)
    {
        if (PropertyPair.Key == TEXT("_class"))
        {
            continue; // Skip the class specifier
        }

        FString PropError;
        if (MutableService.SetObjectProperty(CreatedObject, PropertyPair.Key, PropertyPair.Value, PropError))
        {
            SuccessProps.Add(PropertyPair.Key);
        }
        else
        {
            FailedProps.Add(PropertyPair.Key, PropError);
        }
    }

    // Log results
    if (SuccessProps.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Created instanced object '%s' with properties: %s"),
               *CreatedObject->GetName(), *FString::Join(SuccessProps, TEXT(", ")));
    }

    if (FailedProps.Num() > 0)
    {
        FString FailedList;
        for (const auto& Pair : FailedProps)
        {
            FailedList += FString::Printf(TEXT("%s: %s; "), *Pair.Key, *Pair.Value);
        }
        UE_LOG(LogTemp, Warning, TEXT("Some properties failed on '%s': %s"), *CreatedObject->GetName(), *FailedList);
    }

    return CreatedObject;
}

