#include "Services/PropertyTypeResolverService.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "AssetRegistry/AssetRegistryModule.h"

FPropertyTypeResolverService& FPropertyTypeResolverService::Get()
{
    static FPropertyTypeResolverService Instance;
    return Instance;
}

FString FPropertyTypeResolverService::GetPropertyTypeString(const FProperty* Property) const
{
    if (!Property) return TEXT("Unknown");

    // Handle array properties first
    if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        FString ElementType = GetPropertyTypeString(ArrayProp->Inner);
        return ElementType + TEXT("[]");
    }

    if (Property->IsA<FBoolProperty>()) return TEXT("Boolean");
    if (Property->IsA<FIntProperty>()) return TEXT("Integer");
    if (Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>()) return TEXT("Float");
    if (Property->IsA<FStrProperty>()) return TEXT("String");
    if (Property->IsA<FTextProperty>()) return TEXT("Text");
    if (Property->IsA<FNameProperty>()) return TEXT("Name");

    if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (StructProp->Struct == TBaseStructure<FVector>::Get()) return TEXT("Vector");
        if (StructProp->Struct == TBaseStructure<FRotator>::Get()) return TEXT("Rotator");
        if (StructProp->Struct == TBaseStructure<FTransform>::Get()) return TEXT("Transform");
        if (StructProp->Struct == TBaseStructure<FLinearColor>::Get()) return TEXT("Color");

        // For custom structs, strip the 'F' prefix if present
        FString StructName = StructProp->Struct->GetName();
        if (StructName.StartsWith(TEXT("F")) && StructName.Len() > 1)
        {
            StructName = StructName.RightChop(1);
        }
        return StructName;
    }

    return TEXT("Unknown");
}

bool FPropertyTypeResolverService::ResolveBaseType(const FString& BaseType, FEdGraphPinType& OutPinType) const
{
    // Basic types
    if (BaseType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        return true;
    }
    if (BaseType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("Int"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        return true;
    }
    if (BaseType.Equals(TEXT("Int64"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int64;
        return true;
    }
    if (BaseType.Equals(TEXT("Float"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("Double"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("Real"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        OutPinType.PinSubCategory = TEXT("double");
        return true;
    }
    if (BaseType.Equals(TEXT("Byte"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("uint8"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
        return true;
    }
    if (BaseType.Equals(TEXT("String"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("FString"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        return true;
    }
    if (BaseType.Equals(TEXT("Text"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("FText"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        return true;
    }
    if (BaseType.Equals(TEXT("Name"), ESearchCase::IgnoreCase) ||
        BaseType.Equals(TEXT("FName"), ESearchCase::IgnoreCase))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
        return true;
    }

    // Dynamic struct resolution via UE reflection — handles ALL engine structs
    // without hardcoding (FVector, FLinearColor, FTransform, FGameplayTag, FSlateBrush, etc.)
    {
        UScriptStruct* FoundStruct = nullptr;

        // Try exact name first (handles "FVector", "FLinearColor", etc.)
        FoundStruct = FindObject<UScriptStruct>(nullptr, *BaseType);

        // Try with F prefix ("Vector" → "FVector", "LinearColor" → "FLinearColor")
        if (!FoundStruct && !BaseType.StartsWith(TEXT("F")))
        {
            FoundStruct = FindObject<UScriptStruct>(nullptr, *(TEXT("F") + BaseType));
        }

        // Try without F prefix ("FVector" → "Vector" — unlikely but defensive)
        if (!FoundStruct && BaseType.StartsWith(TEXT("F")) && BaseType.Len() > 1)
        {
            FoundStruct = FindObject<UScriptStruct>(nullptr, *BaseType.Mid(1));
        }

        // Common alias: "Color" → FLinearColor (UE uses FLinearColor, not FColor for BP)
        if (!FoundStruct && BaseType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
        {
            FoundStruct = TBaseStructure<FLinearColor>::Get();
        }

        // Fallback for core engine structs that FindObject can't locate
        // (they exist as C++ structs via TBaseStructure, not as UObject assets)
        if (!FoundStruct)
        {
            if (BaseType.Equals(TEXT("LinearColor"), ESearchCase::IgnoreCase) ||
                BaseType.Equals(TEXT("FLinearColor"), ESearchCase::IgnoreCase))
            {
                FoundStruct = TBaseStructure<FLinearColor>::Get();
            }
            else if (BaseType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase) ||
                     BaseType.Equals(TEXT("FVector"), ESearchCase::IgnoreCase))
            {
                FoundStruct = TBaseStructure<FVector>::Get();
            }
            else if (BaseType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase) ||
                     BaseType.Equals(TEXT("FRotator"), ESearchCase::IgnoreCase))
            {
                FoundStruct = TBaseStructure<FRotator>::Get();
            }
            else if (BaseType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase) ||
                     BaseType.Equals(TEXT("FTransform"), ESearchCase::IgnoreCase))
            {
                FoundStruct = TBaseStructure<FTransform>::Get();
            }
            else if (BaseType.Equals(TEXT("Vector2D"), ESearchCase::IgnoreCase) ||
                     BaseType.Equals(TEXT("FVector2D"), ESearchCase::IgnoreCase))
            {
                FoundStruct = TBaseStructure<FVector2D>::Get();
            }
            else if (BaseType.Equals(TEXT("Vector4"), ESearchCase::IgnoreCase) ||
                     BaseType.Equals(TEXT("FVector4"), ESearchCase::IgnoreCase))
            {
                FoundStruct = TBaseStructure<FVector4>::Get();
            }
        }

        if (FoundStruct)
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = FoundStruct;
            return true;
        }
    }

    // Dynamic UObject/UClass resolution via UE reflection — handles ALL object types
    // (UTexture2D, UStaticMesh, UMaterialInterface, USoundBase, UNiagaraSystem, etc.)
    {
        UClass* FoundClass = nullptr;

        // Try with U prefix ("Texture2D" → "UTexture2D", "StaticMesh" → "UStaticMesh")
        if (!BaseType.StartsWith(TEXT("U")))
        {
            FoundClass = FindObject<UClass>(nullptr, *(TEXT("U") + BaseType));
        }

        // Try exact name ("UTexture2D")
        if (!FoundClass)
        {
            FoundClass = FindObject<UClass>(nullptr, *BaseType);
        }

        // Common aliases: "Texture" → UTexture2D, "Sound" → USoundBase, "Material" → UMaterialInterface
        if (!FoundClass)
        {
            if (BaseType.Equals(TEXT("Texture"), ESearchCase::IgnoreCase))
                FoundClass = UTexture2D::StaticClass();
            else if (BaseType.Equals(TEXT("Sound"), ESearchCase::IgnoreCase))
                FoundClass = USoundBase::StaticClass();
            else if (BaseType.Equals(TEXT("Material"), ESearchCase::IgnoreCase))
                FoundClass = UMaterialInterface::StaticClass();
        }

        if (FoundClass && FoundClass->IsChildOf(UObject::StaticClass()))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            OutPinType.PinSubCategoryObject = FoundClass;
            return true;
        }
    }

    // Try to find a custom enum (E_ prefix convention or any user-defined enum)
    UEnum* FoundEnum = FindCustomEnum(BaseType);
    if (FoundEnum)
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
        OutPinType.PinSubCategoryObject = FoundEnum;
        return true;
    }

    // Try to find a custom struct
    UScriptStruct* FoundStruct = FindCustomStruct(BaseType);
    if (FoundStruct)
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = FoundStruct;
        return true;
    }

    // Default to string if type not recognized
    OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
    return true;
}

bool FPropertyTypeResolverService::ResolvePropertyType(const FString& PropertyType, FEdGraphPinType& OutPinType) const
{
    // Check if this is an array type (either "Array", "Array<Type>", or ends with "[]")
    if (PropertyType.Equals(TEXT("Array"), ESearchCase::IgnoreCase))
    {
        // Default to string array if no specific type is provided
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        OutPinType.ContainerType = EPinContainerType::Array;
        return true;
    }

    if (PropertyType.StartsWith(TEXT("Array<")) && PropertyType.EndsWith(TEXT(">")))
    {
        // Handle Array<Type> syntax
        FString BaseType = PropertyType.Mid(6, PropertyType.Len() - 7);
        FEdGraphPinType BasePinType;
        ResolveBaseType(BaseType, BasePinType);
        OutPinType = BasePinType;
        OutPinType.ContainerType = EPinContainerType::Array;
        return true;
    }

    if (PropertyType.EndsWith(TEXT("[]")))
    {
        // Handle Type[] syntax
        FString BaseType = PropertyType.LeftChop(2);
        FEdGraphPinType BasePinType;
        ResolveBaseType(BaseType, BasePinType);
        OutPinType = BasePinType;
        OutPinType.ContainerType = EPinContainerType::Array;
        return true;
    }

    // Handle soft object references
    if (PropertyType.StartsWith(TEXT("SoftObjectPtr<"), ESearchCase::IgnoreCase) ||
        PropertyType.StartsWith(TEXT("TSoftObjectPtr<"), ESearchCase::IgnoreCase))
    {
        // Extract the inner type
        FString InnerType = PropertyType;
        int32 OpenBracket = InnerType.Find(TEXT("<"));
        int32 CloseBracket = InnerType.Find(TEXT(">"));
        if (OpenBracket != INDEX_NONE && CloseBracket != INDEX_NONE)
        {
            InnerType = InnerType.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);
        }

        OutPinType.PinCategory = UEdGraphSchema_K2::PC_SoftObject;

        // Dynamic class resolution for soft references
        UClass* InnerClass = nullptr;
        if (!InnerType.StartsWith(TEXT("U")))
        {
            InnerClass = FindObject<UClass>(nullptr, *(TEXT("U") + InnerType));
        }
        if (!InnerClass)
        {
            InnerClass = FindObject<UClass>(nullptr, *InnerType);
        }
        OutPinType.PinSubCategoryObject = InnerClass ? InnerClass : UObject::StaticClass();
        return true;
    }

    // Handle non-array types
    return ResolveBaseType(PropertyType, OutPinType);
}

UScriptStruct* FPropertyTypeResolverService::FindCustomStruct(const FString& StructName) const
{
    UE_LOG(LogTemp, Display, TEXT("PropertyTypeResolver: Dynamic search for struct '%s'"), *StructName);

    // Use Asset Registry to search for UserDefinedStruct assets dynamically
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Search for UserDefinedStruct assets
    FARFilter Filter;
    Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> StructAssets;
    AssetRegistry.GetAssets(Filter, StructAssets);

    // Look for matching struct names (try multiple variations)
    TArray<FString> NameVariations;
    NameVariations.Add(StructName);
    NameVariations.Add(FString::Printf(TEXT("F%s"), *StructName));

    for (const FAssetData& AssetData : StructAssets)
    {
        FString AssetName = AssetData.AssetName.ToString();

        for (const FString& NameVariation : NameVariations)
        {
            if (AssetName.Equals(NameVariation, ESearchCase::IgnoreCase))
            {
                UObject* LoadedAsset = AssetData.GetAsset();
                if (UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(LoadedAsset))
                {
                    UE_LOG(LogTemp, Display, TEXT("PropertyTypeResolver: Found struct '%s'"), *UserStruct->GetPathName());
                    return UserStruct;
                }
            }
        }
    }

    // Also try direct loading for built-in structs
    for (const FString& NameVariation : NameVariations)
    {
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *NameVariation);
        if (FoundStruct)
        {
            return FoundStruct;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("PropertyTypeResolver: No struct found for '%s'"), *StructName);
    return nullptr;
}

UEnum* FPropertyTypeResolverService::FindCustomEnum(const FString& EnumName) const
{
    UE_LOG(LogTemp, Display, TEXT("PropertyTypeResolver: Dynamic search for enum '%s'"), *EnumName);

    // Use Asset Registry to search for UserDefinedEnum assets dynamically
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Search for UserDefinedEnum assets
    FARFilter Filter;
    Filter.ClassPaths.Add(UUserDefinedEnum::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> EnumAssets;
    AssetRegistry.GetAssets(Filter, EnumAssets);

    // Look for matching enum names (try multiple variations)
    TArray<FString> NameVariations;
    NameVariations.Add(EnumName);
    // Try without E_ prefix if present
    if (EnumName.StartsWith(TEXT("E_")))
    {
        NameVariations.Add(EnumName.Mid(2));
    }
    // Try with E_ prefix if not present
    else
    {
        NameVariations.Add(FString::Printf(TEXT("E_%s"), *EnumName));
    }

    for (const FAssetData& AssetData : EnumAssets)
    {
        FString AssetName = AssetData.AssetName.ToString();

        for (const FString& NameVariation : NameVariations)
        {
            if (AssetName.Equals(NameVariation, ESearchCase::IgnoreCase))
            {
                UObject* LoadedAsset = AssetData.GetAsset();
                if (UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(LoadedAsset))
                {
                    UE_LOG(LogTemp, Display, TEXT("PropertyTypeResolver: Found enum '%s'"), *UserEnum->GetPathName());
                    return UserEnum;
                }
            }
        }
    }

    // Also try direct loading for built-in enums
    for (const FString& NameVariation : NameVariations)
    {
        UEnum* FoundEnum = LoadObject<UEnum>(nullptr, *NameVariation);
        if (FoundEnum)
        {
            return FoundEnum;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("PropertyTypeResolver: No enum found for '%s'"), *EnumName);
    return nullptr;
}
