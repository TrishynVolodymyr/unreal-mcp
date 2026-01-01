#include "Services/Project/ProjectEnumService.h"
#include "EditorAssetLibrary.h"
#include "Engine/UserDefinedEnum.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/EnumEditorUtils.h"
#include "AssetToolsModule.h"
#include "Factories/EnumFactory.h"

FProjectEnumService& FProjectEnumService::Get()
{
    static FProjectEnumService Instance;
    return Instance;
}

bool FProjectEnumService::CreateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutFullPath, FString& OutError)
{
    // Validate that we have at least one value
    if (Values.Num() == 0)
    {
        OutError = TEXT("At least one enum value is required");
        return false;
    }

    // Make sure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Create the enum asset path
    FString AssetName = EnumName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;
    OutFullPath = PackageName;

    // Check if the enum already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Enum already exists: %s"), *PackageName);
        return false;
    }

    // Create the enum asset using AssetTools and EnumFactory
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UEnumFactory* EnumFactory = NewObject<UEnumFactory>();
    UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath.LeftChop(1), UUserDefinedEnum::StaticClass(), EnumFactory);
    UUserDefinedEnum* NewEnum = Cast<UUserDefinedEnum>(CreatedAsset);

    if (!NewEnum)
    {
        OutError = TEXT("Failed to create enum asset");
        return false;
    }

    // Set the enum description (the "Enum Description" property visible in the editor)
    if (!Description.IsEmpty())
    {
#if WITH_EDITORONLY_DATA
        NewEnum->EnumDescription = FText::FromString(Description);
#endif
        // Also set as tooltip metadata for additional compatibility
        NewEnum->SetMetaData(TEXT("ToolTip"), *Description);
    }

    // The enum is created with one default enumerator, we need to set up the values properly
    // First, get the count of existing enumerators to remove the default one later
    int32 InitialEnumCount = NewEnum->NumEnums();

    // Add the user-specified enumerators using the proper method that sets internal names
    for (int32 i = 0; i < Values.Num(); ++i)
    {
        // Use SetEnumerators which properly sets BOTH internal name AND display name
        // We'll add each value by directly manipulating the enum's internal data

        // Add a new enumerator first (creates with default name)
        FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(NewEnum);

        // Get the index of the newly added enumerator (it's added at the end, before MAX)
        int32 NewIndex = NewEnum->NumEnums() - 2; // -1 for MAX, -1 for 0-based

        // Set the display name for this enumerator
        FText DisplayName = FText::FromString(Values[i]);
        FEnumEditorUtils::SetEnumeratorDisplayName(NewEnum, NewIndex, DisplayName);
    }

    // Remove the initial default enumerators that were created with the enum
    // We need to remove them from the end to avoid index shifting issues
    // The default enum creates entries like "NewEnumerator0"
    for (int32 i = InitialEnumCount - 1; i >= 0; --i)
    {
        // Only remove if this is a default-named enumerator
        FName EnumEntryName = NewEnum->GetNameByIndex(i);
        FString EntryNameStr = EnumEntryName.ToString();
        if (EntryNameStr.Contains(TEXT("NewEnumerator")) || EntryNameStr.Contains(TEXT("::NewEnumerator")))
        {
            FEnumEditorUtils::RemoveEnumeratorFromUserDefinedEnum(NewEnum, i);
        }
    }

    // Set per-value descriptions (tooltips) if provided
    // Iterate through the enum values and match by display name
    if (ValueDescriptions.Num() > 0)
    {
        for (int32 i = 0; i < NewEnum->NumEnums() - 1; ++i) // -1 to skip _MAX
        {
            FText DisplayNameText = NewEnum->GetDisplayNameTextByIndex(i);
            FString DisplayNameStr = DisplayNameText.ToString();

            if (const FString* ValueDesc = ValueDescriptions.Find(DisplayNameStr))
            {
                if (!ValueDesc->IsEmpty())
                {
                    NewEnum->SetMetaData(TEXT("ToolTip"), **ValueDesc, i);
                    UE_LOG(LogTemp, Display, TEXT("MCP Project: Set description for enum value '%s': '%s'"), *DisplayNameStr, **ValueDesc);
                }
            }
        }
    }

    // Mark the enum as modified and save
    NewEnum->MarkPackageDirty();
    UPackage* Package = NewEnum->GetPackage();
    if (Package)
    {
        Package->MarkPackageDirty();
        Package->SetDirtyFlag(true);
    }

    FAssetRegistryModule::AssetCreated(NewEnum);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    // Log the created values for debugging
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Created enum '%s' with %d values:"), *EnumName, Values.Num());
    for (int32 i = 0; i < NewEnum->NumEnums() - 1; ++i)
    {
        FName InternalName = NewEnum->GetNameByIndex(i);
        FText DisplayName = NewEnum->GetDisplayNameTextByIndex(i);
        UE_LOG(LogTemp, Display, TEXT("  [%d] Internal: '%s' Display: '%s'"), i, *InternalName.ToString(), *DisplayName.ToString());
    }

    return true;
}

bool FProjectEnumService::UpdateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutError)
{
    // Validate that we have at least one value
    if (Values.Num() == 0)
    {
        OutError = TEXT("At least one enum value is required");
        return false;
    }

    // Build the asset path
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + EnumName;

    // Check if the enum exists
    if (!UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Enum does not exist: %s"), *PackageName);
        return false;
    }

    // Load the existing enum
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(PackageName);
    UUserDefinedEnum* ExistingEnum = Cast<UUserDefinedEnum>(LoadedAsset);
    if (!ExistingEnum)
    {
        OutError = FString::Printf(TEXT("Failed to load enum: %s"), *PackageName);
        return false;
    }

    // Update the enum description if provided
    if (!Description.IsEmpty())
    {
#if WITH_EDITORONLY_DATA
        ExistingEnum->EnumDescription = FText::FromString(Description);
#endif
        ExistingEnum->SetMetaData(TEXT("ToolTip"), *Description);
    }

    // Remove all existing enumerators (except MAX)
    // Work backwards to avoid index shifting
    int32 CurrentCount = ExistingEnum->NumEnums() - 1; // -1 to skip MAX
    for (int32 i = CurrentCount - 1; i >= 0; --i)
    {
        FEnumEditorUtils::RemoveEnumeratorFromUserDefinedEnum(ExistingEnum, i);
    }

    // Add the new enumerators
    for (int32 i = 0; i < Values.Num(); ++i)
    {
        // Add a new enumerator (creates with default name like NewEnumerator0)
        FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(ExistingEnum);

        // Get the index of the newly added enumerator (it's added at the end, before MAX)
        int32 NewIndex = ExistingEnum->NumEnums() - 2; // -1 for MAX, -1 for 0-based

        // Set the display name for this enumerator
        FText DisplayName = FText::FromString(Values[i]);
        FEnumEditorUtils::SetEnumeratorDisplayName(ExistingEnum, NewIndex, DisplayName);
    }

    // Set per-value descriptions (tooltips) if provided
    if (ValueDescriptions.Num() > 0)
    {
        for (int32 i = 0; i < ExistingEnum->NumEnums() - 1; ++i) // -1 to skip _MAX
        {
            FText DisplayNameText = ExistingEnum->GetDisplayNameTextByIndex(i);
            FString DisplayNameStr = DisplayNameText.ToString();

            if (const FString* ValueDesc = ValueDescriptions.Find(DisplayNameStr))
            {
                if (!ValueDesc->IsEmpty())
                {
                    ExistingEnum->SetMetaData(TEXT("ToolTip"), **ValueDesc, i);
                }
            }
        }
    }

    // Mark the enum as modified and save
    ExistingEnum->Modify();
    ExistingEnum->MarkPackageDirty();
    UPackage* Package = ExistingEnum->GetPackage();
    if (Package)
    {
        Package->MarkPackageDirty();
    }

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    // Log the updated values for debugging
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Updated enum '%s' with %d values:"), *EnumName, Values.Num());
    for (int32 i = 0; i < ExistingEnum->NumEnums() - 1; ++i)
    {
        FName InternalName = ExistingEnum->GetNameByIndex(i);
        FText DisplayName = ExistingEnum->GetDisplayNameTextByIndex(i);
        UE_LOG(LogTemp, Display, TEXT("  [%d] Internal: '%s' Display: '%s'"), i, *InternalName.ToString(), *DisplayName.ToString());
    }

    return true;
}
