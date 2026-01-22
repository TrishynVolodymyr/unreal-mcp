#include "Services/Project/ProjectDataAssetService.h"
#include "EditorAssetLibrary.h"
#include "Engine/DataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "Serialization/ObjectReader.h"

FProjectDataAssetService& FProjectDataAssetService::Get()
{
    static FProjectDataAssetService Instance;
    return Instance;
}

// Forward declaration for folder creation helper
namespace ProjectDataAssetServiceHelpers
{
    static bool EnsureFolderExists(const FString& FolderPath)
    {
        if (!UEditorAssetLibrary::DoesDirectoryExist(FolderPath))
        {
            return UEditorAssetLibrary::MakeDirectory(FolderPath);
        }
        return true;
    }
}

bool FProjectDataAssetService::CreateDataAsset(const FString& Name, const FString& AssetClass, const FString& FolderPath, const TSharedPtr<FJsonObject>& Properties, FString& OutAssetPath, FString& OutError)
{
    // Validate inputs
    if (Name.IsEmpty())
    {
        OutError = TEXT("DataAsset name cannot be empty");
        return false;
    }
    if (AssetClass.IsEmpty())
    {
        OutError = TEXT("Asset class cannot be empty");
        return false;
    }

    // Determine the path
    FString BasePath = FolderPath.IsEmpty() ? TEXT("/Game/Data") : FolderPath;
    FString PackageName = BasePath / Name;
    FString AssetName = Name;

    // Ensure the folder exists
    ProjectDataAssetServiceHelpers::EnsureFolderExists(BasePath);

    // Try to find the DataAsset class using UE5's FindFirstObject (replaces deprecated ANY_PACKAGE)
    UClass* DataAssetClass = nullptr;

    // First, try exact match
    DataAssetClass = FindFirstObject<UClass>(*AssetClass, EFindFirstObjectOptions::ExactClass);

    // Try with U prefix
    if (!DataAssetClass)
    {
        DataAssetClass = FindFirstObject<UClass>(*(TEXT("U") + AssetClass), EFindFirstObjectOptions::ExactClass);
    }

    // Try loading by path if it looks like a path
    if (!DataAssetClass && AssetClass.Contains(TEXT("/")))
    {
        DataAssetClass = LoadClass<UDataAsset>(nullptr, *AssetClass);
    }

    // Fallback to base UDataAsset
    if (!DataAssetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP Project: Could not find class '%s', using UPrimaryDataAsset"), *AssetClass);
        DataAssetClass = UPrimaryDataAsset::StaticClass();
    }

    // Verify it's a DataAsset subclass
    if (!DataAssetClass->IsChildOf(UDataAsset::StaticClass()))
    {
        OutError = FString::Printf(TEXT("Class '%s' is not a DataAsset subclass"), *AssetClass);
        return false;
    }

    // Create the package
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package: %s"), *PackageName);
        return false;
    }

    // Create the DataAsset
    UDataAsset* NewDataAsset = NewObject<UDataAsset>(Package, DataAssetClass, *AssetName, RF_Public | RF_Standalone);
    if (!NewDataAsset)
    {
        OutError = FString::Printf(TEXT("Failed to create DataAsset: %s"), *Name);
        return false;
    }

    // Set properties if provided
    if (Properties.IsValid())
    {
        for (const auto& Pair : Properties->Values)
        {
            FString PropError;
            if (!SetDataAssetProperty(PackageName, Pair.Key, Pair.Value, PropError))
            {
                UE_LOG(LogTemp, Warning, TEXT("MCP Project: Failed to set property '%s': %s"), *Pair.Key, *PropError);
            }
        }
    }

    // Mark the asset as modified
    NewDataAsset->MarkPackageDirty();
    Package->MarkPackageDirty();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewDataAsset);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    OutAssetPath = PackageName;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully created DataAsset '%s' of type '%s' at '%s'"),
        *Name, *DataAssetClass->GetName(), *OutAssetPath);

    return true;
}

bool FProjectDataAssetService::SetDataAssetProperty(const FString& AssetPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    // Validate inputs
    if (AssetPath.IsEmpty())
    {
        OutError = TEXT("Asset path cannot be empty");
        return false;
    }
    if (PropertyName.IsEmpty())
    {
        OutError = TEXT("Property name cannot be empty");
        return false;
    }

    // Normalize the path
    FString NormalizedPath = AssetPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(AssetPath);
        NormalizedPath = AssetPath + TEXT(".") + AssetName;
    }

    // Load the asset
    UObject* LoadedAsset = StaticLoadObject(UDataAsset::StaticClass(), nullptr, *NormalizedPath);
    UDataAsset* DataAsset = Cast<UDataAsset>(LoadedAsset);
    if (!DataAsset)
    {
        OutError = FString::Printf(TEXT("Failed to load DataAsset: %s"), *AssetPath);
        return false;
    }

    // Find the property
    FProperty* Property = DataAsset->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on DataAsset"), *PropertyName);
        return false;
    }

    // Set the property value based on type
    void* PropertyPtr = Property->ContainerPtrToValuePtr<void>(DataAsset);

    if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
    {
        if (PropertyValue->Type == EJson::Number)
        {
            if (NumericProp->IsFloatingPoint())
            {
                NumericProp->SetFloatingPointPropertyValue(PropertyPtr, PropertyValue->AsNumber());
            }
            else
            {
                NumericProp->SetIntPropertyValue(PropertyPtr, static_cast<int64>(PropertyValue->AsNumber()));
            }
        }
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        if (PropertyValue->Type == EJson::Boolean)
        {
            BoolProp->SetPropertyValue(PropertyPtr, PropertyValue->AsBool());
        }
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        if (PropertyValue->Type == EJson::String)
        {
            StrProp->SetPropertyValue(PropertyPtr, PropertyValue->AsString());
        }
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        if (PropertyValue->Type == EJson::String)
        {
            NameProp->SetPropertyValue(PropertyPtr, FName(*PropertyValue->AsString()));
        }
    }
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        if (PropertyValue->Type == EJson::String)
        {
            TextProp->SetPropertyValue(PropertyPtr, FText::FromString(PropertyValue->AsString()));
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName);
        return false;
    }

    // Mark as modified and save
    DataAsset->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(AssetPath, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Set property '%s' on DataAsset '%s'"), *PropertyName, *AssetPath);
    return true;
}

TSharedPtr<FJsonObject> FProjectDataAssetService::GetDataAssetMetadata(const FString& AssetPath, FString& OutError)
{
    // Validate inputs
    if (AssetPath.IsEmpty())
    {
        OutError = TEXT("Asset path cannot be empty");
        return nullptr;
    }

    // Normalize the path
    FString NormalizedPath = AssetPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(AssetPath);
        NormalizedPath = AssetPath + TEXT(".") + AssetName;
    }

    // Load the asset
    UObject* LoadedAsset = StaticLoadObject(UDataAsset::StaticClass(), nullptr, *NormalizedPath);
    UDataAsset* DataAsset = Cast<UDataAsset>(LoadedAsset);
    if (!DataAsset)
    {
        OutError = FString::Printf(TEXT("Failed to load DataAsset: %s"), *AssetPath);
        return nullptr;
    }

    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetBoolField(TEXT("success"), true);
    Metadata->SetStringField(TEXT("path"), AssetPath);
    Metadata->SetStringField(TEXT("name"), DataAsset->GetName());
    Metadata->SetStringField(TEXT("class"), DataAsset->GetClass()->GetName());
    Metadata->SetStringField(TEXT("class_path"), DataAsset->GetClass()->GetPathName());

    // Get all properties
    TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();

    for (TFieldIterator<FProperty> It(DataAsset->GetClass()); It; ++It)
    {
        FProperty* Property = *It;
        if (Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        {
            TSharedPtr<FJsonObject> PropInfo = MakeShared<FJsonObject>();
            PropInfo->SetStringField(TEXT("name"), Property->GetName());
            PropInfo->SetStringField(TEXT("type"), Property->GetCPPType());

            // Get value as string representation
            void* PropertyPtr = Property->ContainerPtrToValuePtr<void>(DataAsset);
            FString ValueStr;
            Property->ExportText_Direct(ValueStr, PropertyPtr, PropertyPtr, DataAsset, PPF_None);
            PropInfo->SetStringField(TEXT("value"), ValueStr);

            PropertiesObj->SetObjectField(Property->GetName(), PropInfo);
        }
    }

    Metadata->SetObjectField(TEXT("properties"), PropertiesObj);

    // Get referenced assets
    TArray<TSharedPtr<FJsonValue>> ReferencesArray;
    TArray<UObject*> References;
    FReferenceFinder ReferenceFinder(References, DataAsset, false, true, true, false);
    ReferenceFinder.FindReferences(DataAsset);

    for (UObject* Reference : References)
    {
        if (Reference && Reference != DataAsset)
        {
            TSharedPtr<FJsonObject> RefObj = MakeShared<FJsonObject>();
            RefObj->SetStringField(TEXT("name"), Reference->GetName());
            RefObj->SetStringField(TEXT("class"), Reference->GetClass()->GetName());
            RefObj->SetStringField(TEXT("path"), Reference->GetPathName());
            ReferencesArray.Add(MakeShared<FJsonValueObject>(RefObj));
        }
    }
    Metadata->SetArrayField(TEXT("references"), ReferencesArray);

    return Metadata;
}
