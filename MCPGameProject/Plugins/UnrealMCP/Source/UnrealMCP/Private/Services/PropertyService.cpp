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
    
    // Support dot-notation for nested struct properties (e.g., "WidgetStyle.Normal.TintColor")
    if (PropertyName.Contains(TEXT(".")))
    {
        TArray<FString> PathSegments;
        PropertyName.ParseIntoArray(PathSegments, TEXT("."), true);
        
        if (PathSegments.Num() < 2)
        {
            OutError = FString::Printf(TEXT("Invalid dot-notation path: '%s'"), *PropertyName);
            return false;
        }
        
        // Find the root property on the object
        FProperty* CurrentProp = FindFProperty<FProperty>(Object->GetClass(), *PathSegments[0]);
        if (!CurrentProp)
        {
            OutError = FString::Printf(TEXT("Root property '%s' not found on object '%s' (Class: %s)"), 
                                      *PathSegments[0], *Object->GetName(), *Object->GetClass()->GetName());
            return false;
        }
        
        void* CurrentData = CurrentProp->ContainerPtrToValuePtr<void>(Object);
        
        // Navigate through intermediate struct fields
        for (int32 i = 1; i < PathSegments.Num() - 1; ++i)
        {
            FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp);
            if (!StructProp)
            {
                OutError = FString::Printf(TEXT("Property '%s' in path '%s' is not a struct (cannot navigate deeper)"),
                                          *PathSegments[i-1], *PropertyName);
                return false;
            }
            
            FProperty* NextProp = FindFProperty<FProperty>(StructProp->Struct, *PathSegments[i]);
            if (!NextProp)
            {
                OutError = FString::Printf(TEXT("Field '%s' not found in struct '%s' (path: '%s')"),
                                          *PathSegments[i], *StructProp->Struct->GetName(), *PropertyName);
                return false;
            }
            
            CurrentData = NextProp->ContainerPtrToValuePtr<void>(CurrentData);
            CurrentProp = NextProp;
        }
        
        // Final segment — find the leaf property and set it
        FStructProperty* ParentStruct = CastField<FStructProperty>(CurrentProp);
        if (!ParentStruct)
        {
            OutError = FString::Printf(TEXT("Property '%s' in path '%s' is not a struct"),
                                      *PathSegments[PathSegments.Num()-2], *PropertyName);
            return false;
        }
        
        const FString& LeafName = PathSegments.Last();
        FProperty* LeafProp = FindFProperty<FProperty>(ParentStruct->Struct, *LeafName);
        if (!LeafProp)
        {
            OutError = FString::Printf(TEXT("Field '%s' not found in struct '%s' (path: '%s')"),
                                      *LeafName, *ParentStruct->Struct->GetName(), *PropertyName);
            return false;
        }
        
        void* LeafData = LeafProp->ContainerPtrToValuePtr<void>(CurrentData);
        UE_LOG(LogTemp, Log, TEXT("PropertyService: Setting nested property via dot-notation: %s"), *PropertyName);
        return SetPropertyFromJson(LeafProp, LeafData, PropertyValue, OutError, Object);
    }
    
    // Find the property (simple, non-dotted name)
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
    
    // Support dot-notation for nested struct properties (e.g., "WidgetStyle.Normal.TintColor")
    if (PropertyName.Contains(TEXT(".")))
    {
        TArray<FString> PathSegments;
        PropertyName.ParseIntoArray(PathSegments, TEXT("."), true);
        
        FProperty* CurrentProp = FindFProperty<FProperty>(Object->GetClass(), *PathSegments[0]);
        if (!CurrentProp)
        {
            OutError = FString::Printf(TEXT("Root property '%s' not found on object '%s'"), 
                                      *PathSegments[0], *Object->GetName());
            return false;
        }
        
        const void* CurrentData = CurrentProp->ContainerPtrToValuePtr<void>(Object);
        
        // Navigate to the target field
        for (int32 i = 1; i < PathSegments.Num(); ++i)
        {
            FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp);
            if (!StructProp)
            {
                OutError = FString::Printf(TEXT("Property '%s' in path '%s' is not a struct"),
                                          *PathSegments[i-1], *PropertyName);
                return false;
            }
            
            FProperty* NextProp = FindFProperty<FProperty>(StructProp->Struct, *PathSegments[i]);
            if (!NextProp)
            {
                OutError = FString::Printf(TEXT("Field '%s' not found in struct '%s' (path: '%s')"),
                                          *PathSegments[i], *StructProp->Struct->GetName(), *PropertyName);
                return false;
            }
            
            CurrentData = NextProp->ContainerPtrToValuePtr<void>(CurrentData);
            CurrentProp = NextProp;
        }
        
        return GetPropertyAsJson(CurrentProp, CurrentData, OutValue, OutError);
    }
    
    // Find the property (simple name)
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

    // Special handling for object properties (UObject*, TObjectPtr<T>)
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        FString ObjectPath;
        if (!JsonValue->TryGetString(ObjectPath))
        {
            // Also accept null to clear the reference
            if (JsonValue->IsNull())
            {
                ObjectProp->SetObjectPropertyValue(PropertyData, nullptr);
                return true;
            }
            OutError = TEXT("Expected string path or null for object property");
            return false;
        }

        if (ObjectPath.IsEmpty())
        {
            ObjectProp->SetObjectPropertyValue(PropertyData, nullptr);
            return true;
        }

        // Try loading the object from path
        UObject* LoadedObject = StaticLoadObject(ObjectProp->PropertyClass, nullptr, *ObjectPath);
        if (!LoadedObject)
        {
            // Try with _C suffix stripped or added
            LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
        }
        if (!LoadedObject)
        {
            OutError = FString::Printf(TEXT("Could not load object from path: %s"), *ObjectPath);
            return false;
        }

        // Validate class compatibility
        if (!LoadedObject->IsA(ObjectProp->PropertyClass))
        {
            OutError = FString::Printf(TEXT("Loaded object '%s' is not a '%s'"),
                                      *LoadedObject->GetName(), *ObjectProp->PropertyClass->GetName());
            return false;
        }

        ObjectProp->SetObjectPropertyValue(PropertyData, LoadedObject);
        UE_LOG(LogTemp, Log, TEXT("PropertyService: Set object property to: %s"), *LoadedObject->GetPathName());
        return true;
    }

    // Special handling for soft object properties (TSoftObjectPtr<T>)
    if (FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(Property))
    {
        FString ObjectPath;
        if (!JsonValue->TryGetString(ObjectPath))
        {
            OutError = TEXT("Expected string path for soft object property");
            return false;
        }

        FSoftObjectPath SoftPath(ObjectPath);
        FSoftObjectPtr SoftPtr(SoftPath);
        *static_cast<FSoftObjectPtr*>(PropertyData) = SoftPtr;
        UE_LOG(LogTemp, Log, TEXT("PropertyService: Set soft object property to: %s"), *ObjectPath);
        return true;
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

    // Get reference to BodyInstance - this works for both runtime components and Blueprint templates
    FBodyInstance& BodyInstance = PrimComponent->BodyInstance;

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
            else if (ValueString == TEXT("QueryAndProbe"))
                CollisionType = ECollisionEnabled::QueryAndProbe;
            else if (ValueString == TEXT("ProbeOnly"))
                CollisionType = ECollisionEnabled::ProbeOnly;

            // Set directly on BodyInstance for Blueprint template persistence
            BodyInstance.SetCollisionEnabled(CollisionType);
            UE_LOG(LogTemp, Log, TEXT("Set CollisionEnabled to %s on %s"), *ValueString, *PrimComponent->GetName());
            return true;
        }
    }
    else if (PropertyName == TEXT("CollisionProfileName"))
    {
        FString ValueString;
        if (PropertyValue->TryGetString(ValueString))
        {
            // SetCollisionProfileName on BodyInstance sets the profile and updates collision responses
            BodyInstance.SetCollisionProfileName(FName(*ValueString));
            UE_LOG(LogTemp, Log, TEXT("Set CollisionProfileName to %s on %s"), *ValueString, *PrimComponent->GetName());
            return true;
        }
    }
    else if (PropertyName == TEXT("bNotifyRigidBodyCollision"))
    {
        bool bValue = false;
        if (PropertyValue->TryGetBool(bValue))
        {
            // Set directly on BodyInstance - this is the "Simulation Generates Hit Events" checkbox
            BodyInstance.bNotifyRigidBodyCollision = bValue;
            UE_LOG(LogTemp, Log, TEXT("Set bNotifyRigidBodyCollision to %s on %s"), bValue ? TEXT("true") : TEXT("false"), *PrimComponent->GetName());
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
        
        // Apply field name aliases for common UE structs
        // FVector4 CornerRadii shows as TopLeft/TopRight/BottomRight/BottomLeft in Details panel
        // but the actual FVector4 fields are X/Y/Z/W
        static const TMap<FString, FString> Vector4Aliases = {
            {TEXT("TopLeft"), TEXT("X")},
            {TEXT("TopRight"), TEXT("Y")},
            {TEXT("BottomRight"), TEXT("Z")},
            {TEXT("BottomLeft"), TEXT("W")}
        };
        
        if (Struct == TBaseStructure<FVector4>::Get() || StructName == TEXT("Vector4"))
        {
            TSharedPtr<FJsonObject> NormalizedJson = MakeShareable(new FJsonObject);
            for (const auto& FieldPair : StructJson->Values)
            {
                const FString* Alias = Vector4Aliases.Find(FieldPair.Key);
                if (Alias)
                {
                    UE_LOG(LogTemp, Log, TEXT("PropertyService: Mapping FVector4 alias '%s' -> '%s'"), *FieldPair.Key, **Alias);
                    NormalizedJson->SetField(*Alias, FieldPair.Value);
                }
                else
                {
                    NormalizedJson->SetField(FieldPair.Key, FieldPair.Value);
                }
            }
            StructJson = NormalizedJson;
        }
        
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
    // Handle as JSON array — dynamic positional mapping via reflection
    else if (JsonValue->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& ArrayValue = JsonValue->AsArray();
        
        // Collect struct fields in order
        TArray<FProperty*> StructFields;
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            StructFields.Add(*It);
        }
        
        if (ArrayValue.Num() > StructFields.Num())
        {
            OutError = FString::Printf(TEXT("Array has %d elements but struct '%s' only has %d fields"),
                ArrayValue.Num(), *StructName, StructFields.Num());
            return false;
        }
        
        if (ArrayValue.Num() == 0)
        {
            OutError = FString::Printf(TEXT("Empty array cannot be mapped to struct '%s'"), *StructName);
            return false;
        }
        
        // Map array elements to struct fields by position
        for (int32 i = 0; i < ArrayValue.Num(); ++i)
        {
            FProperty* StructField = StructFields[i];
            void* FieldData = StructField->ContainerPtrToValuePtr<void>(PropertyData);
            FString FieldError;
            
            if (!SetPropertyFromJson(StructField, FieldData, ArrayValue[i], FieldError))
            {
                OutError = FString::Printf(TEXT("Failed to set struct '%s' field '%s' (index %d): %s"),
                    *StructName, *StructField->GetName(), i, *FieldError);
                return false;
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("PropertyService: Set struct '%s' from array with %d elements (dynamic mapping)"),
            *StructName, ArrayValue.Num());
        return true;
    }
    // Handle single numeric value — fill all numeric fields with the same value
    else if (JsonValue->Type == EJson::Number)
    {
        double Value = JsonValue->AsNumber();
        
        // Verify all fields are numeric before setting
        bool bAllNumeric = true;
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            if (!CastField<FNumericProperty>(*It))
            {
                bAllNumeric = false;
                break;
            }
        }
        
        if (!bAllNumeric)
        {
            OutError = FString::Printf(TEXT("Cannot set struct '%s' from single number — not all fields are numeric"), *StructName);
            return false;
        }
        
        // Fill all numeric fields with the same value
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            FProperty* StructField = *It;
            void* FieldData = StructField->ContainerPtrToValuePtr<void>(PropertyData);
            
            if (FFloatProperty* FloatProp = CastField<FFloatProperty>(StructField))
            {
                FloatProp->SetPropertyValue(FieldData, static_cast<float>(Value));
            }
            else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(StructField))
            {
                DoubleProp->SetPropertyValue(FieldData, Value);
            }
            else if (FIntProperty* IntProp = CastField<FIntProperty>(StructField))
            {
                IntProp->SetPropertyValue(FieldData, static_cast<int32>(Value));
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("PropertyService: Set struct '%s' — all fields to uniform value %f"), *StructName, Value);
        return true;
    }
    
    OutError = FString::Printf(TEXT("Unsupported format for struct '%s' — expected object {}, array [], or single number"), *StructName);
    return false;
}

// Array and instanced object operations (SetArrayPropertyFromJson, SetInstancedObjectArrayFromJson, CreateInstancedObjectFromJson)
// are implemented in PropertyServiceArrayOps.cpp

