#include "Utils/PropertyUtils.h"
#include "Utils/GeometryUtils.h"
#include "EditorAssetLibrary.h"
#include "JsonObjectConverter.h"
#include "Engine/Blueprint.h"

bool FPropertyUtils::SetObjectProperty(UObject* Object, const FString& PropertyName,
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return false;
    }

    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);

    // Handle different property types
    if (Property->IsA<FBoolProperty>())
    {
        ((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (Property->IsA<FIntProperty>())
    {
        int32 IntValue = static_cast<int32>(Value->AsNumber());
        FIntProperty* IntProperty = CastField<FIntProperty>(Property);
        if (IntProperty)
        {
            IntProperty->SetPropertyValue_InContainer(Object, IntValue);
            return true;
        }
    }
    else if (Property->IsA<FFloatProperty>())
    {
        ((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FStrProperty>())
    {
        ((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (Property->IsA<FByteProperty>())
    {
        FByteProperty* ByteProp = CastField<FByteProperty>(Property);
        UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;

        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);

                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"),
                      *PropertyName, ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();

                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"),
                          *PropertyName, *EnumValueName, ByteValue);
                    return true;
                }

                // Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }

                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }

                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"),
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"),
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }

                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    else if (Property->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
        UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
        FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;

        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);

                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"),
                      *PropertyName, EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();

                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"),
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }

                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }

                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }

                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"),
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"),
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }

                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
            bool bStructHandled = false;

            // Handle FVector2D
            if (StructProp->Struct == TBaseStructure<FVector2D>::Get())
            {
                if (Arr.Num() == 2)
                {
                    FVector2D Vec2D(Arr[0]->AsNumber(), Arr[1]->AsNumber());
                    StructProp->CopySingleValue(PropertyAddr, &Vec2D);
                    UE_LOG(LogTemp, Display, TEXT("Setting FVector2D property %s to (%f, %f)"),
                          *PropertyName, Vec2D.X, Vec2D.Y);
                    bStructHandled = true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("FVector2D property requires 2 values, got %d"), Arr.Num());
                }
            }
            // Handle FLinearColor
            else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
            {
                if (Arr.Num() == 4) // RGBA
                {
                    FLinearColor Color(
                        Arr[0]->AsNumber(),
                        Arr[1]->AsNumber(),
                        Arr[2]->AsNumber(),
                        Arr[3]->AsNumber()
                    );
                    StructProp->CopySingleValue(PropertyAddr, &Color);
                    UE_LOG(LogTemp, Display, TEXT("Setting FLinearColor property %s to (R=%f, G=%f, B=%f, A=%f)"),
                          *PropertyName, Color.R, Color.G, Color.B, Color.A);
                    bStructHandled = true;
                }
                 else if (Arr.Num() == 3) // RGB, assume A=1
                {
                    FLinearColor Color(
                        Arr[0]->AsNumber(),
                        Arr[1]->AsNumber(),
                        Arr[2]->AsNumber(),
                        1.0f // Default Alpha to 1
                    );
                    StructProp->CopySingleValue(PropertyAddr, &Color);
                    UE_LOG(LogTemp, Display, TEXT("Setting FLinearColor property %s to (R=%f, G=%f, B=%f, A=1.0)"),
                          *PropertyName, Color.R, Color.G, Color.B);
                    bStructHandled = true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("FLinearColor property requires 3 (RGB) or 4 (RGBA) values, got %d"), Arr.Num());
                }
            }
            // Handle FRotator
            else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
            {
                if (Arr.Num() == 3) // Pitch, Yaw, Roll
                {
                    FRotator Rotator(
                        Arr[0]->AsNumber(), // Pitch
                        Arr[1]->AsNumber(), // Yaw
                        Arr[2]->AsNumber()  // Roll
                    );
                    StructProp->CopySingleValue(PropertyAddr, &Rotator);
                    UE_LOG(LogTemp, Display, TEXT("Setting FRotator property %s to (P=%f, Y=%f, R=%f)"),
                          *PropertyName, Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
                    bStructHandled = true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("FRotator property requires 3 values (Pitch, Yaw, Roll), got %d"), Arr.Num());
                }
            }
            // NOTE: FVector is handled specifically in HandleSetComponentProperty currently,
            // but could be moved here for consistency if desired.

            if (bStructHandled)
            {
                return true; // Successfully handled the struct
            }
        }
        else
        {
             OutErrorMessage = FString::Printf(TEXT("Struct property %s requires a JSON array value"), *PropertyName);
        }
        // If we reach here, the struct type wasn't handled or input was wrong
        if (OutErrorMessage.IsEmpty())
        {
            OutErrorMessage = FString::Printf(TEXT("Unsupported struct type '%s' for property %s"),
                StructProp->Struct ? *StructProp->Struct->GetName() : TEXT("Unknown"), *PropertyName);
        }
        return false;
    }
    // Handle TSubclassOf<T> properties (FClassProperty) - MUST be checked BEFORE FObjectProperty
    // because FClassProperty inherits from FObjectProperty
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FString ClassPath = Value->AsString();

            if (ClassPath.IsEmpty())
            {
                // Set to nullptr for empty string
                ClassProp->SetObjectPropertyValue(PropertyAddr, nullptr);
                UE_LOG(LogTemp, Display, TEXT("Set class property %s to nullptr"), *PropertyName);
                return true;
            }

            UClass* ClassValue = nullptr;

            // If path looks like a Blueprint path (starts with /Game/), handle specially
            if (ClassPath.StartsWith(TEXT("/Game/")))
            {
                FString BlueprintPath = ClassPath;

                // Remove _C suffix if present to get the Blueprint asset path
                if (BlueprintPath.EndsWith(TEXT("_C")))
                {
                    // Path format: /Game/Path/Name.Name_C -> /Game/Path/Name
                    int32 DotIndex;
                    if (BlueprintPath.FindLastChar('.', DotIndex))
                    {
                        BlueprintPath = BlueprintPath.Left(DotIndex);
                    }
                }

                // Try loading as Blueprint first
                UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
                if (Blueprint && Blueprint->GeneratedClass)
                {
                    ClassValue = Blueprint->GeneratedClass;
                    UE_LOG(LogTemp, Display, TEXT("Loaded Blueprint class for %s: %s -> %s"),
                          *PropertyName, *ClassPath, *ClassValue->GetName());
                }
                else
                {
                    // Try loading the _C path directly as a class
                    ClassValue = LoadClass<UObject>(nullptr, *ClassPath);
                    if (ClassValue)
                    {
                        UE_LOG(LogTemp, Display, TEXT("Loaded class directly for %s: %s"),
                              *PropertyName, *ClassPath);
                    }
                }
            }
            else
            {
                // Try loading as a native class path (e.g., /Script/Engine.Actor)
                ClassValue = LoadClass<UObject>(nullptr, *ClassPath);
                if (ClassValue)
                {
                    UE_LOG(LogTemp, Display, TEXT("Loaded native class for %s: %s"),
                          *PropertyName, *ClassPath);
                }
            }

            if (!ClassValue)
            {
                OutErrorMessage = FString::Printf(TEXT("Could not load class from path: %s"), *ClassPath);
                return false;
            }

            // Validate class compatibility with TSubclassOf constraint
            UClass* MetaClass = ClassProp->MetaClass;
            if (MetaClass && !ClassValue->IsChildOf(MetaClass))
            {
                OutErrorMessage = FString::Printf(TEXT("Class '%s' is not a subclass of '%s'"),
                                                 *ClassValue->GetName(), *MetaClass->GetName());
                return false;
            }

            ClassProp->SetObjectPropertyValue(PropertyAddr, ClassValue);
            UE_LOG(LogTemp, Display, TEXT("Set class property %s to %s"),
                  *PropertyName, *ClassValue->GetPathName());
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Class property %s requires a string path value"), *PropertyName);
            return false;
        }
    }
    // Handle object references (like DataTable, Blueprint classes, etc.)
    // NOTE: This MUST come after FClassProperty check because FClassProperty inherits from FObjectProperty
    else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FString AssetPath = Value->AsString();

            // Remove _C suffix if present (for blueprint classes)
            AssetPath.RemoveFromEnd(TEXT("_C"));

            UObject* LoadedObject = nullptr;

            // Use UEditorAssetLibrary to search and load the asset
            if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
            {
                LoadedObject = UEditorAssetLibrary::LoadAsset(AssetPath);
            }
            else
            {
                // If direct path doesn't exist, search for it
                TArray<FString> FoundAssets = UEditorAssetLibrary::ListAssets(TEXT("/Game"), true, false);

                for (const FString& AssetPathIter : FoundAssets)
                {
                    FString AssetName = FPaths::GetBaseFilename(AssetPathIter);
                    if (AssetName.Equals(AssetPath, ESearchCase::IgnoreCase))
                    {
                        LoadedObject = UEditorAssetLibrary::LoadAsset(AssetPathIter);
                        if (LoadedObject)
                        {
                            UE_LOG(LogTemp, Display, TEXT("Found asset by name search: %s at %s"),
                                  *AssetPath, *AssetPathIter);
                            break;
                        }
                    }
                }
            }

            if (LoadedObject)
            {
                ObjectProp->SetObjectPropertyValue(PropertyAddr, LoadedObject);
                UE_LOG(LogTemp, Display, TEXT("Set object property %s to %s"),
                      *PropertyName, *LoadedObject->GetPathName());
                return true;
            }
            else
            {
                OutErrorMessage = FString::Printf(TEXT("Failed to load object from path: %s"), *AssetPath);
                return false;
            }
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Object property %s requires a string path value"), *PropertyName);
            return false;
        }
    }

    OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"),
                                    *Property->GetClass()->GetName(), *PropertyName);
    return false;
}

// Implementation for the new helper function
bool FPropertyUtils::SetPropertyFromJson(FProperty* Property, void* ContainerPtr, const TSharedPtr<FJsonValue>& JsonValue)
{
    if (!Property || !ContainerPtr || !JsonValue.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson: Invalid input parameter(s)."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Property Name: %s, Type: %s"), *Property->GetName(), *Property->GetCPPType());

    // Handle different property types
    if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        bool Value;
        if (JsonValue->TryGetBool(Value))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting Bool property to: %s"), Value ? TEXT("true") : TEXT("false"));
            BoolProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to set Bool property, incompatible value type"));
        }
    }
    else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        int32 Value;
        if (JsonValue->TryGetNumber(Value))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting Int property to: %d"), Value);
            IntProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to set Int property, incompatible value type"));
        }
    }
    else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        double Value; // JSON numbers are doubles
        if (JsonValue->TryGetNumber(Value))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting Float property to: %f"), static_cast<float>(Value));
            FloatProperty->SetPropertyValue(ContainerPtr, static_cast<float>(Value));
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to set Float property, incompatible value type"));
        }
    }
    else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        double Value;
        if (JsonValue->TryGetNumber(Value))
        {
            DoubleProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
    }
    else if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            StrProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
    }
    else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            NameProperty->SetPropertyValue(ContainerPtr, FName(*Value));
            return true;
        }
    }
    else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            TextProperty->SetPropertyValue(ContainerPtr, FText::FromString(Value));
            return true;
        }
    }
    else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProperty->GetEnum();
        if (!Enum) return false;

        FString StringValue;
        int64 IntValue;

        if (JsonValue->TryGetString(StringValue)) // Try setting by string name first
        {
            IntValue = Enum->GetValueByNameString(StringValue);
            if (IntValue != INDEX_NONE)
            {
                EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ContainerPtr, IntValue);
                return true;
            }
        }
        else if (JsonValue->TryGetNumber(IntValue)) // Try setting by integer index
        {
             if (Enum->IsValidEnumValue(IntValue))
             {
                 EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ContainerPtr, IntValue);
                 return true;
             }
        }
    }
    else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Found Struct property: %s"),
            StructProperty->Struct ? *StructProperty->Struct->GetName() : TEXT("NULL"));

        const TSharedPtr<FJsonObject>* JsonObject;
        if (JsonValue->TryGetObject(JsonObject))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Processing JsonObject for struct"));
            // Use JsonObjectConverter to convert the JSON object to the struct
            if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject->ToSharedRef(), StructProperty->Struct, ContainerPtr, 0, 0))
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Successfully converted JsonObject to struct"));
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to convert JsonObject to struct"));
            }
        }
        // Handle common structs specifically if needed (e.g., FVector, FLinearColor from array)
        else if (StructProperty->Struct == TBaseStructure<FVector>::Get())
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Handling FVector struct"));
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (JsonValue->TryGetArray(JsonArray))
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Got array for FVector with %d elements"), JsonArray->Num());
                FVector VecValue;
                if (ParseVector(*JsonArray, VecValue))
                {
                    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting FVector to (%f, %f, %f)"),
                        VecValue.X, VecValue.Y, VecValue.Z);
                    *static_cast<FVector*>(ContainerPtr) = VecValue;
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to parse Vector from array"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Expected array for Vector but got different type"));
            }
        }
        else if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Handling FLinearColor struct"));
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (JsonValue->TryGetArray(JsonArray))
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Got array for FLinearColor with %d elements"), JsonArray->Num());
                FLinearColor ColorValue;
                if (ParseLinearColor(*JsonArray, ColorValue))
                {
                    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting FLinearColor to (%f, %f, %f, %f)"),
                        ColorValue.R, ColorValue.G, ColorValue.B, ColorValue.A);
                    *static_cast<FLinearColor*>(ContainerPtr) = ColorValue;
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to parse LinearColor from array"));
                }
            }
            // Additional check for string format, like when a string representation of an array is passed
            else if (JsonValue->Type == EJson::String)
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Got string for FLinearColor: %s"), *JsonValue->AsString());
                FString ColorString = JsonValue->AsString();

                // Check if the string looks like an array: "[r, g, b, a]"
                if (ColorString.StartsWith(TEXT("[")) && ColorString.EndsWith(TEXT("]")))
                {
                    // Remove brackets
                    ColorString = ColorString.Mid(1, ColorString.Len() - 2);

                    // Split by commas
                    TArray<FString> ColorComponents;
                    ColorString.ParseIntoArray(ColorComponents, TEXT(","), true);

                    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Parsed %d color components from string"), ColorComponents.Num());

                    if (ColorComponents.Num() >= 3)
                    {
                        float R = FCString::Atof(*ColorComponents[0].TrimStart());
                        float G = FCString::Atof(*ColorComponents[1].TrimStart());
                        float B = FCString::Atof(*ColorComponents[2].TrimStart());
                        float A = ColorComponents.Num() >= 4 ? FCString::Atof(*ColorComponents[3].TrimStart()) : 1.0f;

                        FLinearColor ColorValue(R, G, B, A);
                        UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting FLinearColor from string to (%f, %f, %f, %f)"),
                            ColorValue.R, ColorValue.G, ColorValue.B, ColorValue.A);
                        *static_cast<FLinearColor*>(ContainerPtr) = ColorValue;
                        return true;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Not enough color components in string: %s"), *ColorString);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Color string is not in expected format: %s"), *ColorString);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Expected array or string for LinearColor but got different type"));
            }
        }
        // ... existing code ...
    }
    else if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            UClass* LoadedClass = LoadObject<UClass>(nullptr, *Value);
            if (LoadedClass && LoadedClass->IsChildOf(ClassProperty->MetaClass))
            {
                ClassProperty->SetPropertyValue(ContainerPtr, LoadedClass);
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Set UClass* property '%s' to '%s'"), *Property->GetName(), *Value);
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to load or type mismatch for UClass* property '%s' with value '%s'"), *Property->GetName(), *Value);
            }
        }
    }
    // ... existing code ...

    // Log failure if no suitable type handler was found
    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson: Unsupported property type '%s' or invalid JSON value type."), *Property->GetClass()->GetName());
    return false;
}

// Helper methods (private)
bool FPropertyUtils::ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector)
{
    return FGeometryUtils::ParseVector(JsonArray, OutVector);
}

bool FPropertyUtils::ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor)
{
    return FGeometryUtils::ParseLinearColor(JsonArray, OutColor);
}
