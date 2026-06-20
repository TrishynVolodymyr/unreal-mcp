#include "Services/Project/ProjectDataAssetService.h"
#include "EditorAssetLibrary.h"
#include "Engine/DataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "Serialization/ObjectReader.h"
#include "Misc/StringOutputDevice.h"
#include "Editor.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

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

    // Apply a single JSON-typed value to a property directly on an ALREADY-RESOLVED object —
    // no disk load, no save (the caller persists). Shared by CreateDataAsset, which must operate
    // on the in-hand brand-new object (it isn't on disk yet, so a StaticLoadObject-by-path would
    // fail and silently drop every property), and SetDataAssetProperty, which operates on a
    // loaded asset. Mutates in memory only.
    static bool ApplyJsonProperty(UObject* Target, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
    {
        if (!Target)
        {
            OutError = TEXT("Target object is null");
            return false;
        }
        if (PropertyName.IsEmpty())
        {
            OutError = TEXT("Property name cannot be empty");
            return false;
        }

        FProperty* Property = Target->GetClass()->FindPropertyByName(FName(*PropertyName));
        if (!Property)
        {
            OutError = FString::Printf(TEXT("Property '%s' not found on '%s'"), *PropertyName, *Target->GetClass()->GetName());
            return false;
        }

        void* PropertyPtr = Property->ContainerPtrToValuePtr<void>(Target);

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

    // Set properties (if provided) directly on the in-hand new object. We must NOT route through
    // SetDataAssetProperty here: it loads the asset by path (StaticLoadObject), but the asset does
    // not exist on disk yet at this point, so the load fails and every property is silently dropped.
    if (Properties.IsValid())
    {
        for (const auto& Pair : Properties->Values)
        {
            const FString Key = FString(Pair.Key.ToView());
            FString PropError;
            if (!ProjectDataAssetServiceHelpers::ApplyJsonProperty(NewDataAsset, Key, Pair.Value, PropError))
            {
                UE_LOG(LogTemp, Warning, TEXT("MCP Project: Failed to set property '%s': %s"), *Key, *PropError);
            }
        }
    }

    // Mark the asset as modified
    NewDataAsset->MarkPackageDirty();
    Package->MarkPackageDirty();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewDataAsset);

    // Persist to disk via UPackage::SavePackage on the in-hand package — NOT
    // UEditorAssetLibrary::SaveAsset(PackageName). The latter resolves the path through the
    // asset registry, which keys on the OBJECT path ("/Game/X.X"); a brand-new in-memory asset
    // does NOT resolve by its bare PACKAGE path ("/Game/X"), so that save silently no-ops and
    // the .uasset never reaches disk (and the old code didn't even check the return → false
    // success). SavePackage operates directly on the UPackage* with no registry dependency.
    // Same fix MaterialMCP needed — see Docs/known-issues.md "Created Assets Don't Persist".
    const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    if (!UPackage::SavePackage(Package, NewDataAsset, *PackageFileName, SaveArgs))
    {
        // Roll back the half-created asset: unregister it and clear the standalone root so it
        // doesn't linger as a phantom Content Browser entry (and so a retry with the same name
        // isn't blocked) after a disk-write failure.
        FAssetRegistryModule::AssetDeleted(NewDataAsset);
        NewDataAsset->ClearFlags(RF_Public | RF_Standalone);
        OutError = FString::Printf(TEXT("DataAsset '%s' created in memory but SavePackage failed (file '%s')."), *Name, *PackageFileName);
        return false;
    }

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

    // Apply the value to the property (in memory) via the shared helper.
    if (!ProjectDataAssetServiceHelpers::ApplyJsonProperty(DataAsset, PropertyName, PropertyValue, OutError))
    {
        return false;
    }

    // Mark as modified and save. Use the normalized OBJECT path — the bare package path does not
    // resolve through the asset registry (see FProjectAssetOperations::SaveAsset), so saving by
    // AssetPath could no-op for an asset addressed by package path.
    DataAsset->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(NormalizedPath, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Set property '%s' on DataAsset '%s'"), *PropertyName, *AssetPath);
    return true;
}

bool FProjectDataAssetService::CreateAsset(const FString& Name, const FString& AssetClass, const FString& FolderPath, FString& OutAssetPath, FString& OutError)
{
    if (Name.IsEmpty())
    {
        OutError = TEXT("Asset name cannot be empty");
        return false;
    }
    if (AssetClass.IsEmpty())
    {
        OutError = TEXT("Asset class cannot be empty");
        return false;
    }

    const FString BasePath = FolderPath.IsEmpty() ? TEXT("/Game") : FolderPath;
    const FString PackageName = BasePath / Name;

    ProjectDataAssetServiceHelpers::EnsureFolderExists(BasePath);

    // Resolve the class generically (any UObject), trying: exact, U-prefixed, by path.
    UClass* AssetUClass = FindFirstObject<UClass>(*AssetClass, EFindFirstObjectOptions::ExactClass);
    if (!AssetUClass)
    {
        AssetUClass = FindFirstObject<UClass>(*(TEXT("U") + AssetClass), EFindFirstObjectOptions::ExactClass);
    }
    if (!AssetUClass && AssetClass.Contains(TEXT("/")))
    {
        AssetUClass = LoadObject<UClass>(nullptr, *AssetClass);
    }
    if (!AssetUClass)
    {
        OutError = FString::Printf(TEXT("Class '%s' not found. Pass a full class path like '/Script/Voxel.VoxelSurfaceTypeAsset' or a registered class name."), *AssetClass);
        return false;
    }

    // Guard against classes that can't be instantiated as standalone assets.
    if (AssetUClass->HasAnyClassFlags(CLASS_Abstract))
    {
        OutError = FString::Printf(TEXT("Class '%s' is abstract — cannot create an instance."), *AssetUClass->GetName());
        return false;
    }
    if (!AssetUClass->IsChildOf(UObject::StaticClass()))
    {
        OutError = FString::Printf(TEXT("Class '%s' is not a UObject subclass."), *AssetUClass->GetName());
        return false;
    }

    // Refuse to clobber an existing asset.
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Asset already exists at '%s' — choose a different name/folder."), *PackageName);
        return false;
    }

    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package: %s"), *PackageName);
        return false;
    }

    UObject* NewAsset = NewObject<UObject>(Package, AssetUClass, *Name, RF_Public | RF_Standalone);
    if (!NewAsset)
    {
        OutError = FString::Printf(TEXT("NewObject failed for class '%s' (name '%s')."), *AssetUClass->GetName(), *Name);
        return false;
    }

    NewAsset->MarkPackageDirty();
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewAsset);

    // Persist via UPackage::SavePackage on the in-hand package (see CreateDataAsset above for
    // why path-based UEditorAssetLibrary::SaveAsset is unreliable for a brand-new asset).
    const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    if (!UPackage::SavePackage(Package, NewAsset, *PackageFileName, SaveArgs))
    {
        // Roll back the half-created asset (see CreateDataAsset above) so a failed save doesn't
        // leave a phantom registry entry that blocks a retry with the same name.
        FAssetRegistryModule::AssetDeleted(NewAsset);
        NewAsset->ClearFlags(RF_Public | RF_Standalone);
        OutError = FString::Printf(TEXT("Asset '%s' created in memory but SavePackage failed (file '%s')."), *Name, *PackageFileName);
        return false;
    }

    OutAssetPath = PackageName;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Created asset '%s' of class '%s' at '%s'"),
        *Name, *AssetUClass->GetName(), *OutAssetPath);
    return true;
}

bool FProjectDataAssetService::SetObjectProperty(const FString& AssetPath, const FString& PropertyName, const FString& ValueString, FString& OutError, FString* OutAppliedValue)
{
    if (AssetPath.IsEmpty()) { OutError = TEXT("Asset path cannot be empty"); return false; }
    if (PropertyName.IsEmpty()) { OutError = TEXT("Property name cannot be empty"); return false; }

    // Normalize Package → Object path (append .AssetName if missing).
    FString NormalizedPath = AssetPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        NormalizedPath = AssetPath + TEXT(".") + FPaths::GetBaseFilename(AssetPath);
    }

    UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *NormalizedPath);
    if (!Asset)
    {
        OutError = FString::Printf(TEXT("Failed to load asset at '%s'."), *AssetPath);
        return false;
    }

    FProperty* Property = Asset->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on class '%s'."),
            *PropertyName, *Asset->GetClass()->GetName());
        return false;
    }

    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Asset);

    // Generic import — ImportText parses object refs, arrays, structs, enums, etc.
    // Capture the UE parser's diagnostics for a precise error message.
    // PreviousValue backs the unresolved-ref rollback below (the import mutates the
    // asset in memory BEFORE we can detect silently-Nulled object refs).
    FString PreviousValue;
    Property->ExportTextItem_Direct(PreviousValue, ValuePtr, nullptr, Asset, PPF_None);
    FStringOutputDevice ImportErrors;
    Asset->Modify();
    const TCHAR* Result = Property->ImportText_Direct(*ValueString, ValuePtr, Asset, PPF_None, &ImportErrors);
    if (Result == nullptr || !ImportErrors.IsEmpty())
    {
        OutError = FString::Printf(TEXT("Failed to set '%s' (type %s) to '%s': %s"),
            *PropertyName, *Property->GetCPPType(), *ValueString,
            ImportErrors.IsEmpty() ? TEXT("value could not be parsed for this property type") : *ImportErrors);
        return false;
    }

    FPropertyChangedEvent ChangedEvent(Property);
    Asset->PostEditChangeProperty(ChangedEvent);
    Asset->MarkPackageDirty();

    // Anti-lie evidence (known-issues "set_object_property silently fails"): re-export
    // the property AFTER PostEditChangeProperty so the caller sees the value that will
    // actually persist — a PostEditChange that sanitizes/reverts the import is visible
    // here without a second read call.
    FString AppliedValue;
    Property->ExportTextItem_Direct(AppliedValue, ValuePtr, nullptr, Asset, PPF_None);
    if (OutAppliedValue)
    {
        *OutAppliedValue = AppliedValue;
    }

    // Honest failure for unresolved object references: UE's ImportText imports an
    // unloadable object path as None WITHOUT a parser error (verified live 2026-06-11:
    // a bogus "/Game/.../ST_DoesNotExist" array element produced applied "(None)" and
    // would otherwise report success). For object-ref-rooted properties, a None token
    // in the re-exported value that the caller did not explicitly write means at least
    // one reference failed to resolve — report failure instead of claiming success.
    // Explicit "None" in the input keeps working (deliberate null slots).
    {
        const FProperty* RefRoot = Property;
        if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Property))
        {
            RefRoot = AsArray->Inner;
        }
        else if (const FSetProperty* AsSet = CastField<FSetProperty>(Property))
        {
            RefRoot = AsSet->ElementProp;
        }
        if (CastField<FObjectPropertyBase>(RefRoot))
        {
            // A null object ref exports as the bare token None: the whole value, or
            // an array/set element right after '(' or ','. Quoted object paths can't
            // produce these sequences.
            auto HasNoneToken = [](const FString& S)
            {
                return S == TEXT("None")
                    || S.Contains(TEXT("(None"))
                    || S.Contains(TEXT(",None"));
            };
            if (HasNoneToken(AppliedValue) && !HasNoneToken(ValueString))
            {
                // Roll the in-memory value back so a later unrelated save can't
                // persist the half-Nulled import.
                FStringOutputDevice RollbackErrors;
                Property->ImportText_Direct(*PreviousValue, ValuePtr, Asset, PPF_None, &RollbackErrors);
                FPropertyChangedEvent RollbackEvent(Property);
                Asset->PostEditChangeProperty(RollbackEvent);
                OutError = FString::Printf(
                    TEXT("One or more object references in '%s' failed to resolve (imported as None). ")
                    TEXT("Check the asset paths. Rejected import: %s — asset rolled back to: %s"),
                    *PropertyName, *AppliedValue, *PreviousValue);
                return false;
            }
        }
    }

    // Force a render-state refresh for level objects so the change shows live in
    // the editor viewport. PostEditChangeProperty alone marks render state dirty
    // on the component, but with viewport realtime off the swap (e.g. an
    // OverrideMaterials change) won't appear until something redraws — hence the
    // explicit MarkRenderStateDirty + RedrawLevelEditingViewports below.
    if (UPrimitiveComponent* AsPrimitive = Cast<UPrimitiveComponent>(Asset))
    {
        AsPrimitive->MarkRenderStateDirty();
    }
    if (UActorComponent* AsComponent = Cast<UActorComponent>(Asset))
    {
        if (AActor* OwningActor = AsComponent->GetOwner())
        {
            OwningActor->MarkComponentsRenderStateDirty();
        }
    }
    else if (AActor* AsActor = Cast<AActor>(Asset))
    {
        AsActor->MarkComponentsRenderStateDirty();
    }

    // Objects embedded in a level (actors/components) live in a map package, not a
    // standalone saveable asset. UEditorAssetLibrary::SaveAsset can't save a map
    // this way, so treating that as an error is wrong — the property IS set. Mark
    // the map dirty (user saves the level) and redraw, then report success.
    UPackage* OwningPackage = Asset->GetOutermost();
    const bool bIsMapObject = OwningPackage && OwningPackage->ContainsMap();
    if (bIsMapObject)
    {
        if (OwningPackage)
        {
            OwningPackage->MarkPackageDirty();
        }
        if (GEditor)
        {
            GEditor->RedrawLevelEditingViewports(true);
        }
        UE_LOG(LogTemp, Display, TEXT("MCP Project: Set '%s' = '%s' on level object '%s' (render refreshed; map left dirty for manual save)"),
            *PropertyName, *ValueString, *AssetPath);
        return true;
    }

    if (!UEditorAssetLibrary::SaveAsset(NormalizedPath, false))
    {
        OutError = FString::Printf(TEXT("Property set but SaveAsset failed for '%s'."), *AssetPath);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Set '%s' = '%s' on '%s'"), *PropertyName, *ValueString, *AssetPath);
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
