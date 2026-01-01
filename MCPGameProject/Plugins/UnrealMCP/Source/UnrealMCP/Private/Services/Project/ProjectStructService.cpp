#include "Services/Project/ProjectStructService.h"
#include "Services/PropertyTypeResolverService.h"
#include "Services/AssetDiscoveryService.h"
#include "EditorAssetLibrary.h"
#include "Engine/UserDefinedStruct.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/StructureEditorUtils.h"
#include "AssetToolsModule.h"
#include "Factories/StructureFactory.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

FProjectStructService& FProjectStructService::Get()
{
    static FProjectStructService Instance;
    return Instance;
}

bool FProjectStructService::CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError)
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

bool FProjectStructService::UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError)
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

    // Try to find and load the struct using multiple methods
    UUserDefinedStruct* ExistingStruct = nullptr;

    // Method 1: Try direct load with the constructed package name
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        UObject* AssetObj = UEditorAssetLibrary::LoadAsset(PackageName);
        ExistingStruct = Cast<UUserDefinedStruct>(AssetObj);
    }

    // Method 2: Try LoadObject which handles more path formats
    if (!ExistingStruct)
    {
        ExistingStruct = LoadObject<UUserDefinedStruct>(nullptr, *PackageName);
    }

    // Method 3: Search common paths if just a name was provided
    if (!ExistingStruct && !StructName.StartsWith(TEXT("/")))
    {
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/%s"), *StructName),
            FString::Printf(TEXT("/Game/Blueprints/%s"), *StructName),
            FString::Printf(TEXT("/Game/Data/%s"), *StructName),
            FString::Printf(TEXT("/Game/Structs/%s"), *StructName),
            FString::Printf(TEXT("/Game/Quests/Data/Structs/%s"), *StructName),
            FString::Printf(TEXT("/Game/Inventory/Data/%s"), *StructName),
            Path + TEXT("/") + StructName
        };

        for (const FString& SearchPath : SearchPaths)
        {
            ExistingStruct = LoadObject<UUserDefinedStruct>(nullptr, *SearchPath);
            if (ExistingStruct)
            {
                break;
            }
        }
    }

    // Method 4: Use Asset Registry search as fallback
    if (!ExistingStruct)
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FAssetData> AssetDataList;
        AssetRegistry.GetAssetsByClass(UUserDefinedStruct::StaticClass()->GetClassPathName(), AssetDataList);

        // Extract just the struct name for comparison
        FString JustName = StructName;
        if (JustName.Contains(TEXT("/")))
        {
            int32 LastSlash;
            if (JustName.FindLastChar('/', LastSlash))
            {
                JustName = JustName.Mid(LastSlash + 1);
            }
        }

        for (const FAssetData& AssetData : AssetDataList)
        {
            FString AssetName = AssetData.AssetName.ToString();
            if (AssetName.Equals(JustName, ESearchCase::IgnoreCase))
            {
                ExistingStruct = Cast<UUserDefinedStruct>(AssetData.GetAsset());
                if (ExistingStruct)
                {
                    break;
                }
            }
        }
    }

    if (!ExistingStruct)
    {
        OutError = FString::Printf(TEXT("Struct does not exist: %s (searched multiple paths and asset registry)"), *StructName);
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

bool FProjectStructService::CreateStructProperty(UUserDefinedStruct* Struct, const TSharedPtr<FJsonObject>& PropertyObj) const
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

TArray<TSharedPtr<FJsonObject>> FProjectStructService::ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError)
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
