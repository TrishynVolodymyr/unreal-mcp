#include "Services/ProjectService.h"
#include "Services/PropertyTypeResolverService.h"
#include "Services/AssetDiscoveryService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "GameFramework/InputSettings.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "EditorAssetLibrary.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/StructureEditorUtils.h"
#include "Kismet2/EnumEditorUtils.h"
#include "UnrealEd.h"
#include "AssetToolsModule.h"
#include "Factories/StructureFactory.h"
#include "Factories/EnumFactory.h"
// FontFace assets are created directly using NewObject
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "Factories/Factory.h"
#include "Misc/PackageName.h"

FProjectService::FProjectService()
{
}

bool FProjectService::CreateInputMapping(const FString& ActionName, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError)
{
    // Get the input settings
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        OutError = TEXT("Failed to get input settings");
        return false;
    }

    // Create the input action mapping
    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    // Add modifiers if provided
    if (Modifiers.IsValid())
    {
        if (Modifiers->HasField(TEXT("shift")))
        {
            ActionMapping.bShift = Modifiers->GetBoolField(TEXT("shift"));
        }
        if (Modifiers->HasField(TEXT("ctrl")))
        {
            ActionMapping.bCtrl = Modifiers->GetBoolField(TEXT("ctrl"));
        }
        if (Modifiers->HasField(TEXT("alt")))
        {
            ActionMapping.bAlt = Modifiers->GetBoolField(TEXT("alt"));
        }
        if (Modifiers->HasField(TEXT("cmd")))
        {
            ActionMapping.bCmd = Modifiers->GetBoolField(TEXT("cmd"));
        }
    }

    // Add the mapping
    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    return true;
}

bool FProjectService::CreateFolder(const FString& FolderPath, bool& bOutAlreadyExists, FString& OutError)
{
    bOutAlreadyExists = false;
    
    // Get the base project directory
    FString ProjectPath = FPaths::ProjectDir();
    
    // Check if this is a content folder request
    bool bIsContentFolder = FolderPath.StartsWith(TEXT("/Content/")) || FolderPath.StartsWith(TEXT("Content/"));
    if (bIsContentFolder)
    {
        // Use UE's asset system for content folders
        FString AssetPath = FolderPath;
        if (!AssetPath.StartsWith(TEXT("/Game/")))
        {
            // Convert Content/ to /Game/ for asset paths
            AssetPath = AssetPath.Replace(TEXT("/Content/"), TEXT("/Game/"));
            AssetPath = AssetPath.Replace(TEXT("Content/"), TEXT("/Game/"));
        }
        
        // Check if the directory already exists
        if (UEditorAssetLibrary::DoesDirectoryExist(AssetPath))
        {
            bOutAlreadyExists = true;
            return true;
        }
        
        if (!UEditorAssetLibrary::MakeDirectory(AssetPath))
        {
            OutError = FString::Printf(TEXT("Failed to create content folder: %s"), *AssetPath);
            return false;
        }
    }
    else
    {
        // For non-content folders, use platform file system
        // Clean up the folder path to avoid double slashes
        FString CleanFolderPath = FolderPath;
        if (CleanFolderPath.StartsWith(TEXT("/")))
        {
            CleanFolderPath = CleanFolderPath.RightChop(1);
        }
        
        FString FullPath = FPaths::Combine(ProjectPath, CleanFolderPath);
        
        // Check if directory already exists
        if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FullPath))
        {
            bOutAlreadyExists = true;
            return true;
        }
        
        if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FullPath))
        {
            OutError = FString::Printf(TEXT("Failed to create folder: %s"), *FullPath);
            return false;
        }
    }

    return true;
}

TArray<FString> FProjectService::ListFolderContents(const FString& FolderPath, bool& bOutSuccess, FString& OutError)
{
    TArray<FString> Contents;
    bOutSuccess = false;

    // Check if this is a content folder request
    bool bIsContentFolder = FolderPath.StartsWith(TEXT("/Game")) || FolderPath.StartsWith(TEXT("/Content/")) || FolderPath.StartsWith(TEXT("Content/"));

    if (bIsContentFolder)
    {
        // Use UE's asset system for content folders
        FString AssetPath = FolderPath;
        if (AssetPath.StartsWith(TEXT("/Content/")))
        {
            AssetPath = AssetPath.Replace(TEXT("/Content/"), TEXT("/Game/"));
        }
        else if (AssetPath.StartsWith(TEXT("Content/")))
        {
            AssetPath = AssetPath.Replace(TEXT("Content/"), TEXT("/Game/"));
        }

        // Get subdirectories and assets using AssetRegistry
        // NOTE: We don't use UEditorAssetLibrary::DoesDirectoryExist as it's unreliable for virtual content paths.
        // Instead, we try to get contents and check if anything was found.
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FString> SubPaths;
        AssetRegistry.GetSubPaths(AssetPath, SubPaths, false);

        for (const FString& SubPath : SubPaths)
        {
            Contents.Add(FString::Printf(TEXT("FOLDER: %s"), *SubPath));
        }

        // Get assets (UE 5.7 compatible)
        TArray<FString> Assets = UEditorAssetLibrary::ListAssets(AssetPath, false, false);
        for (const FString& Asset : Assets)
        {
            Contents.Add(FString::Printf(TEXT("ASSET: %s"), *Asset));
        }

        // If no content was found at all, the path likely doesn't exist
        if (Contents.Num() == 0)
        {
            // Double-check with recursive search - maybe there are only nested assets
            TArray<FString> RecursiveAssets = UEditorAssetLibrary::ListAssets(AssetPath, true, false);
            if (RecursiveAssets.Num() == 0)
            {
                OutError = FString::Printf(TEXT("Content directory does not exist or is empty: %s"), *AssetPath);
                return Contents;
            }
            // If we found recursive assets, the folder exists but only has nested content
            // Get sub-paths again with that knowledge - the recursive assets tell us subfolders exist
            SubPaths.Empty();
            AssetRegistry.GetSubPaths(AssetPath, SubPaths, false);
            for (const FString& SubPath : SubPaths)
            {
                Contents.Add(FString::Printf(TEXT("FOLDER: %s"), *SubPath));
            }
        }
    }
    else
    {
        // For non-content folders, use platform file system
        FString ProjectPath = FPaths::ProjectDir();
        
        // Clean up the folder path to avoid double slashes
        FString CleanFolderPath = FolderPath;
        if (CleanFolderPath.StartsWith(TEXT("/")))
        {
            CleanFolderPath = CleanFolderPath.RightChop(1);
        }
        
        FString FullPath = FPaths::Combine(ProjectPath, CleanFolderPath);
        
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FullPath))
        {
            OutError = FString::Printf(TEXT("Directory does not exist: %s"), *FullPath);
            return Contents;
        }
        
        // List directory contents
        TArray<FString> FoundFiles;
        
        FPlatformFileManager::Get().GetPlatformFile().FindFiles(FoundFiles, *FullPath, TEXT("*"));
        
        // Use IterateDirectory to find subdirectories (UE 5.7 compatible)
        FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*FullPath, [&Contents](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
        {
            if (bIsDirectory)
            {
                Contents.Add(FString::Printf(TEXT("DIR: %s"), *FPaths::GetCleanFilename(FilenameOrDirectory)));
            }
            return true; // Continue iteration
        });
        
        for (const FString& File : FoundFiles)
        {
            Contents.Add(FString::Printf(TEXT("FILE: %s"), *FPaths::GetCleanFilename(File)));
        }
    }
    
    bOutSuccess = true;
    return Contents;
}

FString FProjectService::GetProjectDirectory() const
{
    return FPaths::ProjectDir();
}



bool FProjectService::CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError)
{
    // Make sure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Create the struct asset path
    FString AssetName = StructName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;
    OutFullPath = PackageName;

    // Check if the struct already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Struct already exists: %s"), *PackageName);
        return false;
    }

    // Create the struct asset
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UStructureFactory* StructFactory = NewObject<UStructureFactory>();
    UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath.LeftChop(1), UUserDefinedStruct::StaticClass(), StructFactory);
    UUserDefinedStruct* NewStruct = Cast<UUserDefinedStruct>(CreatedAsset);
    
    if (!NewStruct)
    {
        OutError = TEXT("Failed to create struct asset");
        return false;
    }

    // Set the struct description and tooltip
    if (!Description.IsEmpty())
    {
        NewStruct->SetMetaData(TEXT("Comments"), *Description);
        FStructureEditorUtils::ChangeTooltip(NewStruct, Description);
    }

    // First, collect all existing variables to remove
    TArray<FGuid> ExistingGuids;
    {
        const TArray<FStructVariableDescription>& VarDescArray = FStructureEditorUtils::GetVarDesc(NewStruct);
        for (int32 i = 0; i < VarDescArray.Num(); ++i)
        {
            ExistingGuids.Add(VarDescArray[i].VarGuid);
        }
    }

    // Remove all existing variables
    for (const FGuid& Guid : ExistingGuids)
    {
        FStructureEditorUtils::RemoveVariable(NewStruct, Guid);
    }

    // Add new variables
    for (const TSharedPtr<FJsonObject>& PropertyObj : Properties)
    {
        if (!CreateStructProperty(NewStruct, PropertyObj))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create property for struct %s"), *StructName);
        }
    }

    // Clean up any remaining unrenamed variables (MemberVar_)
    TArray<FGuid> GuidsToRemove;
    for (const FStructVariableDescription& Desc : FStructureEditorUtils::GetVarDesc(NewStruct))
    {
        if (Desc.VarName.ToString().StartsWith(TEXT("MemberVar_")))
        {
            GuidsToRemove.Add(Desc.VarGuid);
        }
    }
    for (const FGuid& Guid : GuidsToRemove)
    {
        FStructureEditorUtils::RemoveVariable(NewStruct, Guid);
    }

    // Final compilation and save
    FStructureEditorUtils::CompileStructure(NewStruct);
    
    // Force save the asset
    NewStruct->MarkPackageDirty();
    UPackage* Package = NewStruct->GetPackage();
    if (Package)
    {
        Package->MarkPackageDirty();
        Package->SetDirtyFlag(true);
    }
    
    FAssetRegistryModule::AssetCreated(NewStruct);
    
    // Additional save attempt
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    return true;
}

bool FProjectService::UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError)
{
    // Create the struct asset path
    // If StructName already contains a full path (starts with /), use it directly
    FString PackageName;
    if (StructName.StartsWith(TEXT("/")))
    {
        // StructName is a full path - use it directly
        // Remove any trailing asset name duplication (e.g., "/Game/Structs/MyStruct.MyStruct" -> "/Game/Structs/MyStruct")
        PackageName = StructName;
        int32 DotIndex;
        if (PackageName.FindLastChar('.', DotIndex))
        {
            PackageName = PackageName.Left(DotIndex);
        }
    }
    else
    {
        // StructName is just a name - combine with Path
        FString PackagePath = Path;
        if (!PackagePath.EndsWith(TEXT("/")))
        {
            PackagePath += TEXT("/");
        }
        PackageName = PackagePath + StructName;
    }

    // Check if the struct exists
    if (!UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Struct does not exist: %s"), *PackageName);
        return false;
    }

    UObject* AssetObj = UEditorAssetLibrary::LoadAsset(PackageName);
    UUserDefinedStruct* ExistingStruct = Cast<UUserDefinedStruct>(AssetObj);
    if (!ExistingStruct)
    {
        OutError = TEXT("Failed to load struct asset");
        return false;
    }

    // Set the struct description and tooltip
    if (!Description.IsEmpty())
    {
        ExistingStruct->SetMetaData(TEXT("Comments"), *Description);
        FStructureEditorUtils::ChangeTooltip(ExistingStruct, Description);
    }

    // Build a map of existing variables by name (extract base name without GUID)
    TMap<FString, FStructVariableDescription> ExistingVarsByName;
    auto InitialVarDescArray = FStructureEditorUtils::GetVarDesc(ExistingStruct);
    for (const FStructVariableDescription& Desc : InitialVarDescArray)
    {
        FString VarName = Desc.VarName.ToString();
        // Extract base name (everything before the first underscore and number)
        FString BaseName = VarName;
        int32 UnderscoreIndex;
        if (VarName.FindChar('_', UnderscoreIndex))
        {
            BaseName = VarName.Left(UnderscoreIndex);
        }
        ExistingVarsByName.Add(BaseName, Desc);
    }

    // Track which variables were updated or added
    TSet<FString> UpdatedOrAddedNames;

    for (const TSharedPtr<FJsonObject>& PropertyObj : Properties)
    {
        if (!PropertyObj.IsValid())
        {
            continue;
        }

        FString PropertyName;
        if (!PropertyObj->TryGetStringField(TEXT("name"), PropertyName))
        {
            continue;
        }

        FString PropertyTooltip;
        PropertyObj->TryGetStringField(TEXT("description"), PropertyTooltip);

        if (ExistingVarsByName.Contains(PropertyName))
        {
            const FStructVariableDescription& ExistingDesc = ExistingVarsByName[PropertyName];
            
            // Check if type needs to be updated using proper Unreal Engine approach
            FString NewPropertyType;
            PropertyObj->TryGetStringField(TEXT("type"), NewPropertyType);
            
            FEdGraphPinType NewPinType;
            bool bNewTypeValid = FPropertyTypeResolverService::Get().ResolvePropertyType(NewPropertyType, NewPinType);
            
            if (bNewTypeValid)
            {
                // Use FStructureEditorUtils::ChangeVariableType for proper type updates
                // This is the official Unreal Engine way to change variable types
                if (FStructureEditorUtils::ChangeVariableType(ExistingStruct, ExistingDesc.VarGuid, NewPinType))
                {
                    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully changed type for property '%s' in struct '%s'"), *PropertyName, *StructName);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("MCP Project: Failed to change type for property '%s' in struct '%s' - type may be the same"), *PropertyName, *StructName);
                }
            }
            
            // Update tooltip if needed
            if (!PropertyTooltip.IsEmpty())
            {
                FStructureEditorUtils::ChangeVariableTooltip(ExistingStruct, ExistingDesc.VarGuid, PropertyTooltip);
            }
            
            UpdatedOrAddedNames.Add(PropertyName);
        }
        else
        {
            // Add new variable
            if (!CreateStructProperty(ExistingStruct, PropertyObj))
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to add new property %s to struct %s"), *PropertyName, *StructName);
            }
            else
            {
                UpdatedOrAddedNames.Add(PropertyName);
            }
        }
    }

    // Remove variables that are no longer in the properties list
    TArray<FGuid> GuidsToRemove;
    for (const FStructVariableDescription& Desc : FStructureEditorUtils::GetVarDesc(ExistingStruct))
    {
        FString VarName = Desc.VarName.ToString();
        FString BaseName = VarName;
        int32 UnderscoreIndex;
        if (VarName.FindChar('_', UnderscoreIndex))
        {
            BaseName = VarName.Left(UnderscoreIndex);
        }
        
        if (!UpdatedOrAddedNames.Contains(BaseName) && !VarName.StartsWith(TEXT("MemberVar_")))
        {
            GuidsToRemove.Add(Desc.VarGuid);
        }
    }
    for (const FGuid& Guid : GuidsToRemove)
    {
        FStructureEditorUtils::RemoveVariable(ExistingStruct, Guid);
    }

    // Final compilation and save
    FStructureEditorUtils::CompileStructure(ExistingStruct);
    ExistingStruct->MarkPackageDirty();

    return true;
}

bool FProjectService::CreateStructProperty(UUserDefinedStruct* Struct, const TSharedPtr<FJsonObject>& PropertyObj) const
{
    if (!Struct || !PropertyObj.IsValid())
    {
        return false;
    }

    FString PropertyName;
    if (!PropertyObj->TryGetStringField(TEXT("name"), PropertyName))
    {
        return false;
    }

    FString PropertyType;
    if (!PropertyObj->TryGetStringField(TEXT("type"), PropertyType))
    {
        return false;
    }

    FString PropertyTooltip;
    PropertyObj->TryGetStringField(TEXT("description"), PropertyTooltip);

    // Create the pin type
    FEdGraphPinType PinType;
    if (!FPropertyTypeResolverService::Get().ResolvePropertyType(PropertyType, PinType))
    {
        return false;
    }

    // First, add the variable
    bool bAdded = FStructureEditorUtils::AddVariable(Struct, PinType);
    if (!bAdded)
    {
        return false;
    }

    // Get the updated variable list and find the last added variable
    const TArray<FStructVariableDescription>& VarDescArray = FStructureEditorUtils::GetVarDesc(Struct);
    if (VarDescArray.Num() > 0)
    {
        const FStructVariableDescription& NewVarDesc = VarDescArray.Last();
        
        // Rename the variable - try multiple times if needed
        bool bRenameSuccess = false;
        for (int32 Attempt = 0; Attempt < 3; ++Attempt)
        {
            if (FStructureEditorUtils::RenameVariable(Struct, NewVarDesc.VarGuid, *PropertyName))
            {
                bRenameSuccess = true;
                break;
            }
            
            // Wait a bit and try again
            FPlatformProcess::Sleep(0.01f);
        }
        
        if (!bRenameSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to rename variable to %s"), *PropertyName);
        }
        
        // Set tooltip
        if (!PropertyTooltip.IsEmpty())
        {
            FStructureEditorUtils::ChangeVariableTooltip(Struct, NewVarDesc.VarGuid, PropertyTooltip);
        }
        
        // Mark the struct as modified
        Struct->MarkPackageDirty();
        
        return true;
    }

    return false;
}

TArray<TSharedPtr<FJsonObject>> FProjectService::ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError)
{
    TArray<TSharedPtr<FJsonObject>> Variables;
    bOutSuccess = false;

    UUserDefinedStruct* Struct = nullptr;

    // Strategy 1: Try exact path if provided
    if (!Path.IsEmpty())
    {
        FString PackagePath = Path;
        if (!PackagePath.EndsWith(TEXT("/")))
        {
            PackagePath += TEXT("/");
        }
        FString PackageName = PackagePath + StructName;

        if (UEditorAssetLibrary::DoesAssetExist(PackageName))
        {
            UObject* AssetObj = UEditorAssetLibrary::LoadAsset(PackageName);
            Struct = Cast<UUserDefinedStruct>(AssetObj);
        }
    }

    // Strategy 2: Use smart asset discovery service (searches by name)
    if (!Struct)
    {
        UScriptStruct* FoundStruct = FAssetDiscoveryService::Get().FindStructType(StructName);
        if (FoundStruct)
        {
            // Check if it's a user-defined struct
            Struct = Cast<UUserDefinedStruct>(FoundStruct);
            if (!Struct)
            {
                // It's a native/C++ struct - still valid, we can read its properties
                // For native structs, we iterate properties differently
                for (TFieldIterator<FProperty> PropIt(FoundStruct); PropIt; ++PropIt)
                {
                    FProperty* Property = *PropIt;
                    if (!Property) continue;

                    TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
                    VarObj->SetStringField(TEXT("name"), Property->GetName());
                    VarObj->SetStringField(TEXT("type"), FPropertyTypeResolverService::Get().GetPropertyTypeString(Property));

                    FString Tooltip = Property->GetToolTipText().ToString();
                    if (!Tooltip.IsEmpty())
                    {
                        VarObj->SetStringField(TEXT("description"), Tooltip);
                    }
                    Variables.Add(VarObj);
                }
                bOutSuccess = true;
                return Variables;
            }
        }
    }

    // Strategy 3: Try common paths as fallback
    if (!Struct)
    {
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/%s"), *StructName),
            FString::Printf(TEXT("/Game/Blueprints/%s"), *StructName),
            FString::Printf(TEXT("/Game/Data/%s"), *StructName),
            FString::Printf(TEXT("/Game/Structs/%s"), *StructName),
            FString::Printf(TEXT("/Game/Inventory/Data/%s"), *StructName),
            FString::Printf(TEXT("/Game/DataStructures/%s"), *StructName)
        };

        for (const FString& SearchPath : SearchPaths)
        {
            if (UEditorAssetLibrary::DoesAssetExist(SearchPath))
            {
                UObject* AssetObj = UEditorAssetLibrary::LoadAsset(SearchPath);
                Struct = Cast<UUserDefinedStruct>(AssetObj);
                if (Struct) break;
            }
        }
    }

    if (!Struct)
    {
        OutError = FString::Printf(TEXT("Struct '%s' not found. Searched in common paths and asset registry. Try providing full path like '/Game/Inventory/Data/%s'"), *StructName, *StructName);
        return Variables;
    }

    // Get all properties from the struct
    for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!Property)
        {
            continue;
        }

        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Property->GetName());
        VarObj->SetStringField(TEXT("type"), FPropertyTypeResolverService::Get().GetPropertyTypeString(Property));
        
        // Get tooltip/description if available
        FString Tooltip = Property->GetToolTipText().ToString();
        if (!Tooltip.IsEmpty())
        {
            VarObj->SetStringField(TEXT("description"), Tooltip);
        }

        Variables.Add(VarObj);
    }

    bOutSuccess = true;
    return Variables;
}

bool FProjectService::CreateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutFullPath, FString& OutError)
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

    // Add the user-specified enumerators
    for (int32 i = 0; i < Values.Num(); ++i)
    {
        // Add a new enumerator
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
            FText DisplayName = NewEnum->GetDisplayNameTextByIndex(i);
            FString DisplayNameStr = DisplayName.ToString();

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

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully created enum '%s' with %d values at '%s'"), *EnumName, Values.Num(), *OutFullPath);

    return true;
}

// Enhanced Input methods - placeholder implementations
bool FProjectService::CreateEnhancedInputAction(const FString& ActionName, const FString& Path, const FString& Description, const FString& ValueType, FString& OutAssetPath, FString& OutError)
{
    // Enhanced Input Action creation is handled by the legacy command system
    // This service method is a placeholder for future refactoring
    OutError = TEXT("Enhanced Input Action creation is handled by legacy commands - use create_enhanced_input_action command");
    return false;
}

bool FProjectService::CreateInputMappingContext(const FString& ContextName, const FString& Path, const FString& Description, FString& OutAssetPath, FString& OutError)
{
    // Input Mapping Context creation is handled by the legacy command system
    // This service method is a placeholder for future refactoring
    OutError = TEXT("Input Mapping Context creation is handled by legacy commands - use create_input_mapping_context command");
    return false;
}

bool FProjectService::AddMappingToContext(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError)
{
    // Mapping addition to context is handled by the legacy command system
    // This service method is a placeholder for future refactoring
    OutError = TEXT("Add mapping to context is handled by legacy commands - use add_mapping_to_context command");
    return false;
}

TArray<TSharedPtr<FJsonObject>> FProjectService::ListInputActions(const FString& Path, bool& bOutSuccess, FString& OutError)
{
    TArray<TSharedPtr<FJsonObject>> Actions;
    bOutSuccess = false;
    OutError = TEXT("List input actions not yet implemented in service layer");
    return Actions;
}

TArray<TSharedPtr<FJsonObject>> FProjectService::ListInputMappingContexts(const FString& Path, bool& bOutSuccess, FString& OutError)
{
    TArray<TSharedPtr<FJsonObject>> Contexts;
    bOutSuccess = false;
    OutError = TEXT("List input mapping contexts not yet implemented in service layer");
    return Contexts;
}

bool FProjectService::DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError)
{
    // Validate source asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
    {
        OutError = FString::Printf(TEXT("Source asset does not exist: %s"), *SourcePath);
        return false;
    }

    // Ensure destination path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationPath))
    {
        if (!UEditorAssetLibrary::MakeDirectory(DestinationPath))
        {
            OutError = FString::Printf(TEXT("Failed to create destination directory: %s"), *DestinationPath);
            return false;
        }
    }

    // Build the full destination path
    FString CleanDestPath = DestinationPath;
    if (!CleanDestPath.EndsWith(TEXT("/")))
    {
        CleanDestPath += TEXT("/");
    }
    FString FullDestinationPath = CleanDestPath + NewName;

    // Check if destination already exists
    if (UEditorAssetLibrary::DoesAssetExist(FullDestinationPath))
    {
        OutError = FString::Printf(TEXT("Destination asset already exists: %s"), *FullDestinationPath);
        return false;
    }

    // Use UEditorAssetLibrary::DuplicateAsset - this is the simplest and most reliable method
    // It handles all asset types automatically (Blueprints, Widgets, DataTables, Materials, etc.)
    if (!UEditorAssetLibrary::DuplicateAsset(SourcePath, FullDestinationPath))
    {
        OutError = FString::Printf(TEXT("Failed to duplicate asset from '%s' to '%s'"), *SourcePath, *FullDestinationPath);
        return false;
    }

    OutNewAssetPath = FullDestinationPath;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully duplicated asset from '%s' to '%s'"), *SourcePath, *FullDestinationPath);

    return true;
}

bool FProjectService::CreateFontFace(const FString& FontName, const FString& Path, const FString& SourceTexturePath, bool bUseSDF, int32 DistanceFieldSpread, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError)
{
    // Ensure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Build the asset path
    FString AssetName = FontName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;
    OutAssetPath = PackageName;

    // Check if the font face already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Font face already exists: %s"), *PackageName);
        return false;
    }

    // Load the source texture if provided
    UTexture2D* SourceTexture = nullptr;
    if (!SourceTexturePath.IsEmpty())
    {
        FString NormalizedTexturePath = SourceTexturePath;
        if (!NormalizedTexturePath.Contains(TEXT(".")))
        {
            FString TextureAssetName = FPaths::GetBaseFilename(SourceTexturePath);
            NormalizedTexturePath = SourceTexturePath + TEXT(".") + TextureAssetName;
        }

        UObject* LoadedAsset = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *NormalizedTexturePath);
        SourceTexture = Cast<UTexture2D>(LoadedAsset);
        if (!SourceTexture)
        {
            OutError = FString::Printf(TEXT("Failed to load source texture: %s"), *SourceTexturePath);
            return false;
        }
    }

    // Create the font face asset
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

    // Create a package for the new asset
    FString PackagePathForCreate = PackagePath.LeftChop(1); // Remove trailing slash
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font face: %s"), *PackageName);
        return false;
    }

    // Create the FontFace object
    UFontFace* NewFontFace = NewObject<UFontFace>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    if (!NewFontFace)
    {
        OutError = TEXT("Failed to create font face object");
        return false;
    }

    // Configure SDF settings
    if (bUseSDF)
    {
        // For SDF fonts, we need to set up the font face to use the texture
        // UFontFace properties for SDF:
        NewFontFace->Hinting = EFontHinting::None; // SDF fonts don't use hinting
        NewFontFace->LoadingPolicy = EFontLoadingPolicy::LazyLoad;

        // Set the source texture as the font source - this requires setting up the font data
        // For SDF fonts from textures, we typically need to store a reference
        // Note: UFontFace in UE5 uses FontFaceData for the actual font data
        // For SDF textures, we'll store metadata indicating SDF usage
    }

    // Apply font metrics if provided
    if (FontMetrics.IsValid())
    {
        double Ascender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("ascender"), Ascender))
        {
            NewFontFace->bIsAscendOverridden = true;
            NewFontFace->AscendOverriddenValue = static_cast<int32>(Ascender);
        }

        double Descender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("descender"), Descender))
        {
            NewFontFace->bIsDescendOverridden = true;
            NewFontFace->DescendOverriddenValue = static_cast<int32>(Descender);
        }
    }

    // Mark the asset as modified
    NewFontFace->MarkPackageDirty();
    Package->MarkPackageDirty();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewFontFace);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully created font face '%s' at '%s'"), *FontName, *OutAssetPath);

    return true;
}

bool FProjectService::ImportTTFFont(const FString& FontName, const FString& Path, const FString& TTFFilePath, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError)
{
    // Validate the TTF file exists
    if (!FPaths::FileExists(TTFFilePath))
    {
        OutError = FString::Printf(TEXT("TTF file not found: %s"), *TTFFilePath);
        return false;
    }

    // Ensure the output path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Build the asset paths
    FString AssetName = FontName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString FontPackageName = PackagePath + AssetName;
    FString FontFacePackageName = PackagePath + AssetName + TEXT("_Face");
    OutAssetPath = FontPackageName;

    // Check if the font already exists
    if (UEditorAssetLibrary::DoesAssetExist(FontPackageName))
    {
        OutError = FString::Printf(TEXT("Font already exists: %s"), *FontPackageName);
        return false;
    }

    // Read the TTF file into memory
    TArray<uint8> FontData;
    if (!FFileHelper::LoadFileToArray(FontData, *TTFFilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read TTF file: %s"), *TTFFilePath);
        return false;
    }

    // Step 1: Create the FontFace asset (holds the raw TTF data)
    UPackage* FontFacePackage = CreatePackage(*FontFacePackageName);
    if (!FontFacePackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font face: %s"), *FontFacePackageName);
        return false;
    }

    FString FontFaceAssetName = AssetName + TEXT("_Face");
    UFontFace* NewFontFace = NewObject<UFontFace>(FontFacePackage, FName(*FontFaceAssetName), RF_Public | RF_Standalone);
    if (!NewFontFace)
    {
        OutError = TEXT("Failed to create font face object");
        return false;
    }

    // Set the source file path
    NewFontFace->SourceFilename = TTFFilePath;

    // Load the font data into the FontFace
    NewFontFace->FontFaceData = MakeShared<FFontFaceData>();
    NewFontFace->FontFaceData->SetData(MoveTemp(FontData));

    // Set properties for TTF fonts - use Inline loading since we embedded the data
    NewFontFace->Hinting = EFontHinting::Default;
    NewFontFace->LoadingPolicy = EFontLoadingPolicy::Inline;

    // Apply font metrics if provided
    if (FontMetrics.IsValid())
    {
        double Ascender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("ascender"), Ascender))
        {
            NewFontFace->bIsAscendOverridden = true;
            NewFontFace->AscendOverriddenValue = static_cast<int32>(Ascender);
        }

        double Descender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("descender"), Descender))
        {
            NewFontFace->bIsDescendOverridden = true;
            NewFontFace->DescendOverriddenValue = static_cast<int32>(Descender);
        }
    }

    // Save the FontFace
    NewFontFace->MarkPackageDirty();
    FontFacePackage->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewFontFace);
    UEditorAssetLibrary::SaveAsset(FontFacePackageName, false);

    // Step 2: Create the UFont (composite font) that UMG can use
    UPackage* FontPackage = CreatePackage(*FontPackageName);
    if (!FontPackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font: %s"), *FontPackageName);
        return false;
    }

    UFont* NewFont = NewObject<UFont>(FontPackage, FName(*AssetName), RF_Public | RF_Standalone);
    if (!NewFont)
    {
        OutError = TEXT("Failed to create font object");
        return false;
    }

    // Configure as a Runtime font (renders TTF on-demand)
    NewFont->FontCacheType = EFontCacheType::Runtime;

    // Set up the CompositeFont to reference our FontFace
    // The default typeface will use our imported font
    FCompositeFont& CompositeFont = NewFont->GetMutableInternalCompositeFont();

    // Clear and set up the default typeface
    CompositeFont.DefaultTypeface.Fonts.Empty();

    // Add our font as the "Regular" style in the default typeface
    FTypefaceEntry& TypefaceEntry = CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
    TypefaceEntry.Name = TEXT("Regular");
    TypefaceEntry.Font = FFontData(NewFontFace);

    // Save the Font
    NewFont->MarkPackageDirty();
    FontPackage->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewFont);
    UEditorAssetLibrary::SaveAsset(FontPackageName, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully imported TTF font '%s' from '%s' (FontFace: %s, Font: %s)"),
        *FontName, *TTFFilePath, *FontFacePackageName, *OutAssetPath);

    return true;
}

bool FProjectService::SetFontFaceProperties(const FString& FontPath, const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, TArray<FString>& OutFailedProperties, FString& OutError)
{
    OutSuccessProperties.Empty();
    OutFailedProperties.Empty();

    // Normalize the path
    FString NormalizedPath = FontPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(FontPath);
        NormalizedPath = FontPath + TEXT(".") + AssetName;
    }

    // Load the font face
    UObject* LoadedAsset = StaticLoadObject(UFontFace::StaticClass(), nullptr, *NormalizedPath);
    UFontFace* FontFace = Cast<UFontFace>(LoadedAsset);
    if (!FontFace)
    {
        OutError = FString::Printf(TEXT("Failed to load font face: %s"), *FontPath);
        return false;
    }

    // Apply properties
    if (!Properties.IsValid())
    {
        OutError = TEXT("No properties provided");
        return false;
    }

    // Handle Hinting property
    FString HintingStr;
    if (Properties->TryGetStringField(TEXT("Hinting"), HintingStr))
    {
        if (HintingStr == TEXT("None"))
        {
            FontFace->Hinting = EFontHinting::None;
            OutSuccessProperties.Add(TEXT("Hinting"));
        }
        else if (HintingStr == TEXT("Auto"))
        {
            FontFace->Hinting = EFontHinting::Auto;
            OutSuccessProperties.Add(TEXT("Hinting"));
        }
        else if (HintingStr == TEXT("AutoLight"))
        {
            FontFace->Hinting = EFontHinting::AutoLight;
            OutSuccessProperties.Add(TEXT("Hinting"));
        }
        else
        {
            OutFailedProperties.Add(FString::Printf(TEXT("Hinting_InvalidValue_%s"), *HintingStr));
        }
    }

    // Handle LoadingPolicy property
    FString LoadingPolicyStr;
    if (Properties->TryGetStringField(TEXT("LoadingPolicy"), LoadingPolicyStr))
    {
        if (LoadingPolicyStr == TEXT("LazyLoad"))
        {
            FontFace->LoadingPolicy = EFontLoadingPolicy::LazyLoad;
            OutSuccessProperties.Add(TEXT("LoadingPolicy"));
        }
        else if (LoadingPolicyStr == TEXT("Stream"))
        {
            FontFace->LoadingPolicy = EFontLoadingPolicy::Stream;
            OutSuccessProperties.Add(TEXT("LoadingPolicy"));
        }
        else
        {
            OutFailedProperties.Add(FString::Printf(TEXT("LoadingPolicy_InvalidValue_%s"), *LoadingPolicyStr));
        }
    }

    // Handle Ascender override
    double Ascender = 0;
    if (Properties->TryGetNumberField(TEXT("Ascender"), Ascender))
    {
        FontFace->bIsAscendOverridden = true;
        FontFace->AscendOverriddenValue = static_cast<int32>(Ascender);
        OutSuccessProperties.Add(TEXT("Ascender"));
    }

    // Handle Descender override
    double Descender = 0;
    if (Properties->TryGetNumberField(TEXT("Descender"), Descender))
    {
        FontFace->bIsDescendOverridden = true;
        FontFace->DescendOverriddenValue = static_cast<int32>(Descender);
        OutSuccessProperties.Add(TEXT("Descender"));
    }

    // Handle StrikeBrushHeightPercentage (SubFaceIndex is not a direct property)
    double StrikeHeight = 0;
    if (Properties->TryGetNumberField(TEXT("StrikeBrushHeightPercentage"), StrikeHeight))
    {
        FontFace->StrikeBrushHeightPercentage = static_cast<int32>(StrikeHeight);
        OutSuccessProperties.Add(TEXT("StrikeBrushHeightPercentage"));
    }

    // Mark as dirty and save
    FontFace->Modify();
    FontFace->MarkPackageDirty();

    return OutSuccessProperties.Num() > 0 || OutFailedProperties.Num() == 0;
}

TSharedPtr<FJsonObject> FProjectService::GetFontFaceMetadata(const FString& FontPath, FString& OutError)
{
    // Normalize the path
    FString NormalizedPath = FontPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(FontPath);
        NormalizedPath = FontPath + TEXT(".") + AssetName;
    }

    // Load the font face
    UObject* LoadedAsset = StaticLoadObject(UFontFace::StaticClass(), nullptr, *NormalizedPath);
    UFontFace* FontFace = Cast<UFontFace>(LoadedAsset);
    if (!FontFace)
    {
        OutError = FString::Printf(TEXT("Failed to load font face: %s"), *FontPath);
        return nullptr;
    }

    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetBoolField(TEXT("success"), true);
    Metadata->SetStringField(TEXT("font_path"), FontPath);
    Metadata->SetStringField(TEXT("font_name"), FontFace->GetName());

    // Hinting
    FString HintingStr;
    switch (FontFace->Hinting)
    {
        case EFontHinting::None: HintingStr = TEXT("None"); break;
        case EFontHinting::Auto: HintingStr = TEXT("Auto"); break;
        case EFontHinting::AutoLight: HintingStr = TEXT("AutoLight"); break;
        default: HintingStr = TEXT("Default"); break;
    }
    Metadata->SetStringField(TEXT("hinting"), HintingStr);

    // Loading Policy
    FString LoadingPolicyStr;
    switch (FontFace->LoadingPolicy)
    {
        case EFontLoadingPolicy::LazyLoad: LoadingPolicyStr = TEXT("LazyLoad"); break;
        case EFontLoadingPolicy::Stream: LoadingPolicyStr = TEXT("Stream"); break;
        default: LoadingPolicyStr = TEXT("LazyLoad"); break;
    }
    Metadata->SetStringField(TEXT("loading_policy"), LoadingPolicyStr);

    // Source filename
    Metadata->SetStringField(TEXT("source_filename"), FontFace->SourceFilename);

    // Metrics
    Metadata->SetBoolField(TEXT("ascender_override_set"), FontFace->bIsAscendOverridden);
    Metadata->SetNumberField(TEXT("ascender"), FontFace->AscendOverriddenValue);
    Metadata->SetBoolField(TEXT("descender_override_set"), FontFace->bIsDescendOverridden);
    Metadata->SetNumberField(TEXT("descender"), FontFace->DescendOverriddenValue);
    Metadata->SetNumberField(TEXT("strike_brush_height_percentage"), FontFace->StrikeBrushHeightPercentage);

    return Metadata;
}

bool FProjectService::CreateOfflineFont(const FString& FontName, const FString& Path, const FString& TexturePath, const FString& MetricsFilePath, FString& OutAssetPath, FString& OutError)
{
    // Load metrics JSON from file
    if (MetricsFilePath.IsEmpty())
    {
        OutError = TEXT("Metrics file path is required for offline font creation");
        return false;
    }

    // Check if file exists
    if (!FPaths::FileExists(MetricsFilePath))
    {
        OutError = FString::Printf(TEXT("Metrics file not found: %s"), *MetricsFilePath);
        return false;
    }

    // Read the file content
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *MetricsFilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read metrics file: %s"), *MetricsFilePath);
        return false;
    }

    // Parse the JSON
    TSharedPtr<FJsonObject> MetricsJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, MetricsJson) || !MetricsJson.IsValid())
    {
        OutError = FString::Printf(TEXT("Failed to parse metrics JSON from file: %s"), *MetricsFilePath);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Loaded metrics from file: %s"), *MetricsFilePath);

    // Ensure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Build the asset path
    FString AssetName = FontName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;
    OutAssetPath = PackageName;

    // Check if the font already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Font already exists: %s"), *PackageName);
        return false;
    }

    // Load the texture
    FString NormalizedTexturePath = TexturePath;
    if (!NormalizedTexturePath.Contains(TEXT(".")))
    {
        FString TextureAssetName = FPaths::GetBaseFilename(TexturePath);
        NormalizedTexturePath = TexturePath + TEXT(".") + TextureAssetName;
    }

    UObject* LoadedTexture = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *NormalizedTexturePath);
    UTexture2D* FontTexture = Cast<UTexture2D>(LoadedTexture);
    if (!FontTexture)
    {
        OutError = FString::Printf(TEXT("Failed to load texture: %s"), *TexturePath);
        return false;
    }

    // Create a package for the new asset
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font: %s"), *PackageName);
        return false;
    }

    // Create the UFont object
    UFont* NewFont = NewObject<UFont>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    if (!NewFont)
    {
        OutError = TEXT("Failed to create font object");
        return false;
    }

    // Set to offline caching mode
    NewFont->FontCacheType = EFontCacheType::Offline;

    // Add the texture to the font
    NewFont->Textures.Add(FontTexture);

    // Extract metrics from JSON
    int32 AtlasWidth = MetricsJson->GetIntegerField(TEXT("atlasWidth"));
    int32 AtlasHeight = MetricsJson->GetIntegerField(TEXT("atlasHeight"));
    int32 LineHeight = MetricsJson->GetIntegerField(TEXT("lineHeight"));
    int32 Baseline = MetricsJson->GetIntegerField(TEXT("baseline"));

    // Set font metrics
    NewFont->EmScale = 1.0f;
    NewFont->Ascent = static_cast<float>(Baseline);
    NewFont->Descent = static_cast<float>(LineHeight - Baseline);
    NewFont->Leading = 0.0f;
    NewFont->Kerning = 0;
    NewFont->ScalingFactor = 1.0f;
    NewFont->LegacyFontSize = LineHeight;

    // Process characters from JSON
    const TSharedPtr<FJsonObject>* CharactersObj;
    if (MetricsJson->TryGetObjectField(TEXT("characters"), CharactersObj))
    {
        TMap<FString, TSharedPtr<FJsonValue>> CharMap = (*CharactersObj)->Values;

        // Initialize character remap
        NewFont->IsRemapped = 1;

        for (const auto& CharPair : CharMap)
        {
            int32 CharCode = FCString::Atoi(*CharPair.Key);
            const TSharedPtr<FJsonObject>& CharData = CharPair.Value->AsObject();

            if (!CharData.IsValid())
            {
                continue;
            }

            // Get UV coordinates (normalized 0-1)
            double U = CharData->GetNumberField(TEXT("u"));
            double V = CharData->GetNumberField(TEXT("v"));
            int32 Width = CharData->GetIntegerField(TEXT("width"));
            int32 Height = CharData->GetIntegerField(TEXT("height"));
            int32 YOffset = CharData->GetIntegerField(TEXT("yOffset"));

            // Convert normalized UVs to pixel coordinates
            int32 StartU = FMath::RoundToInt(U * AtlasWidth);
            int32 StartV = FMath::RoundToInt(V * AtlasHeight);

            // Create the font character
            FFontCharacter FontChar;
            FontChar.StartU = StartU;
            FontChar.StartV = StartV;
            FontChar.USize = Width;
            FontChar.VSize = Height;
            FontChar.TextureIndex = 0; // First (and only) texture
            FontChar.VerticalOffset = YOffset;

            // Add to characters array
            int32 CharIndex = NewFont->Characters.Add(FontChar);

            // Add to remap table
            NewFont->CharRemap.Add(static_cast<uint16>(CharCode), static_cast<uint16>(CharIndex));
        }
    }

    // Cache character count and max height
    NewFont->CacheCharacterCountAndMaxCharHeight();

    // Mark the asset as modified
    NewFont->MarkPackageDirty();
    Package->MarkPackageDirty();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewFont);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully created offline font '%s' at '%s' with %d characters"),
        *FontName, *OutAssetPath, NewFont->Characters.Num());

    return true;
}

TSharedPtr<FJsonObject> FProjectService::GetFontMetadata(const FString& FontPath, FString& OutError)
{
    // Normalize the path
    FString NormalizedPath = FontPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(FontPath);
        NormalizedPath = FontPath + TEXT(".") + AssetName;
    }

    // Load the font
    UObject* LoadedAsset = StaticLoadObject(UFont::StaticClass(), nullptr, *NormalizedPath);
    UFont* Font = Cast<UFont>(LoadedAsset);
    if (!Font)
    {
        OutError = FString::Printf(TEXT("Failed to load font: %s"), *FontPath);
        return nullptr;
    }

    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetBoolField(TEXT("success"), true);
    Metadata->SetStringField(TEXT("font_path"), FontPath);
    Metadata->SetStringField(TEXT("font_name"), Font->GetName());

    // Font cache type
    FString CacheTypeStr = (Font->FontCacheType == EFontCacheType::Offline) ? TEXT("Offline") : TEXT("Runtime");
    Metadata->SetStringField(TEXT("cache_type"), CacheTypeStr);

    // Metrics
    Metadata->SetNumberField(TEXT("em_scale"), Font->EmScale);
    Metadata->SetNumberField(TEXT("ascent"), Font->Ascent);
    Metadata->SetNumberField(TEXT("descent"), Font->Descent);
    Metadata->SetNumberField(TEXT("leading"), Font->Leading);
    Metadata->SetNumberField(TEXT("kerning"), Font->Kerning);
    Metadata->SetNumberField(TEXT("scaling_factor"), Font->ScalingFactor);
    Metadata->SetNumberField(TEXT("legacy_font_size"), Font->LegacyFontSize);

    // Character count
    Metadata->SetNumberField(TEXT("character_count"), Font->Characters.Num());
    Metadata->SetNumberField(TEXT("texture_count"), Font->Textures.Num());
    Metadata->SetBoolField(TEXT("is_remapped"), Font->IsRemapped != 0);

    return Metadata;
}

