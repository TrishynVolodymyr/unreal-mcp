// PropertyServiceArrayOps.cpp
// Array and instanced object operations for FPropertyService
// This file is part of the PropertyService implementation

#include "Services/PropertyService.h"
#include "GameplayTagContainer.h"
#include "Engine/Engine.h"

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
