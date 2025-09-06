#include "Services/ProjectService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "GameFramework/InputSettings.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "EditorAssetLibrary.h"
#include "Engine/UserDefinedStruct.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/StructureEditorUtils.h"
#include "UnrealEd.h"
#include "AssetToolsModule.h"
#include "Factories/StructureFactory.h"
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
        
        if (!UEditorAssetLibrary::DoesDirectoryExist(AssetPath))
        {
            OutError = FString::Printf(TEXT("Content directory does not exist: %s"), *AssetPath);
            return Contents;
        }
        
        // Get subdirectories using AssetRegistry
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
        
        TArray<FString> SubPaths;
        AssetRegistry.GetSubPaths(AssetPath, SubPaths, false);
        
        for (const FString& SubPath : SubPaths)
        {
            Contents.Add(FString::Printf(TEXT("FOLDER: %s"), *SubPath));
        }
        
        // Get assets (UE 5.6 compatible)
        TArray<FString> Assets = UEditorAssetLibrary::ListAssets(AssetPath, false, false);
        for (const FString& Asset : Assets)
        {
            Contents.Add(FString::Printf(TEXT("ASSET: %s"), *Asset));
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
        
        // Use IterateDirectory to find subdirectories (UE 5.6 compatible)
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

FString FProjectService::GetPropertyTypeString(const FProperty* Property) const
{
    if (!Property) return TEXT("Unknown");
    
    // Handle array properties first
    if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        // Get the type of array elements
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

bool FProjectService::ResolvePropertyType(const FString& PropertyType, FEdGraphPinType& OutPinType) const
{
    // Check if this is an array type (either "Array", "Array<Type>", or ends with "[]")
    if (PropertyType.Equals(TEXT("Array"), ESearchCase::IgnoreCase))
    {
        // Default to string array if no specific type is provided
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        OutPinType.ContainerType = EPinContainerType::Array;
        return true;
    }
    else if (PropertyType.StartsWith(TEXT("Array<")) && PropertyType.EndsWith(TEXT(">")))
    {
        // Handle Array<Type> syntax
        FString BaseType = PropertyType.Mid(6, PropertyType.Len() - 7); // Remove "Array<" and ">"
        
        // Create a temporary pin type for the base type
        FEdGraphPinType BasePinType;
        
        // Resolve the base type
        if (BaseType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else if (BaseType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (BaseType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (BaseType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (BaseType.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        }
        else if (BaseType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
        }
        else if (BaseType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        }
        else if (BaseType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        }
        else if (BaseType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
        }
        else if (BaseType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
        }
        else
        {
            // Try to find a custom struct using dynamic search
            UScriptStruct* FoundStruct = FindCustomStruct(BaseType);
            if (FoundStruct)
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                BasePinType.PinSubCategoryObject = FoundStruct;
            }
            else
            {
                // Default to string array if type not resolved
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_String;
            }
        }
        
        // Set up the array type
        OutPinType = BasePinType;
        OutPinType.ContainerType = EPinContainerType::Array;
        return true;
    }
    else if (PropertyType.EndsWith(TEXT("[]")))
    {
        // Get the base type without the array suffix
        FString BaseType = PropertyType.LeftChop(2);
        
        // Create a temporary pin type for the base type
        FEdGraphPinType BasePinType;
        
        // Resolve the base type
        if (BaseType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else if (BaseType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (BaseType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (BaseType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (BaseType.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        }
        else if (BaseType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
        }
        else if (BaseType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        }
        else if (BaseType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        }
        else if (BaseType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
        }
        else if (BaseType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
        {
            BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            BasePinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
        }
        else
        {
            // Try to find a custom struct
            UScriptStruct* FoundStruct = nullptr;
            TArray<FString> StructNameVariations;
            StructNameVariations.Add(BaseType);
            StructNameVariations.Add(FString::Printf(TEXT("F%s"), *BaseType));
            StructNameVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Blueprints/%s.%s"), *BaseType, *BaseType)));
            StructNameVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("DataStructures/%s.%s"), *BaseType, *BaseType)));
            
            for (const FString& StructVariation : StructNameVariations)
            {
                FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructVariation);
                if (FoundStruct)
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    BasePinType.PinSubCategoryObject = FoundStruct;
                    break;
                }
            }
            
            if (!FoundStruct)
            {
                // Default to string array if type not resolved
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_String;
            }
        }
        
        // Set up the array type
        OutPinType = BasePinType;
        OutPinType.ContainerType = EPinContainerType::Array;
        return true;
    }
    else
    {
        // Handle non-array types
        if (PropertyType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else if (PropertyType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (PropertyType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (PropertyType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (PropertyType.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        }
        else if (PropertyType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
        }
        else if (PropertyType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        }
        else if (PropertyType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        }
        else if (PropertyType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
        }
        else if (PropertyType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
        }
        else
        {
            // Try to find a custom struct using dynamic search
            UScriptStruct* FoundStruct = FindCustomStruct(PropertyType);
            if (FoundStruct)
            {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                OutPinType.PinSubCategoryObject = FoundStruct;
            }
            else
            {
                // Default to string if type not recognized
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
            }
        }
        return true;
    }
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
    FString AssetName = StructName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;

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
            // Update tooltip if needed
            const FStructVariableDescription& ExistingDesc = ExistingVarsByName[PropertyName];
            if (!PropertyTooltip.IsEmpty() && PropertyTooltip != ExistingDesc.ToolTip)
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
    if (!ResolvePropertyType(PropertyType, PinType))
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

    // Create the struct asset path
    FString AssetName = StructName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;

    // Check if the struct exists
    if (!UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Struct does not exist: %s"), *PackageName);
        return Variables;
    }

    UObject* AssetObj = UEditorAssetLibrary::LoadAsset(PackageName);
    UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(AssetObj);
    if (!Struct)
    {
        OutError = TEXT("Failed to load struct asset");
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
        VarObj->SetStringField(TEXT("type"), GetPropertyTypeString(Property));
        
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

UScriptStruct* FProjectService::FindCustomStruct(const FString& StructName) const
{
    // First try direct name variations
    TArray<FString> DirectVariations;
    DirectVariations.Add(StructName);
    DirectVariations.Add(FString::Printf(TEXT("F%s"), *StructName));
    
    for (const FString& Variation : DirectVariations)
    {
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *Variation);
        if (FoundStruct)
        {
            return FoundStruct;
        }
    }
    
    // Use Asset Registry to search for UserDefinedStruct assets
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Search for UserDefinedStruct assets
    FARFilter Filter;
    Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    
    TArray<FAssetData> StructAssets;
    AssetRegistry.GetAssets(Filter, StructAssets);
    
    // Look for matching struct names
    for (const FAssetData& AssetData : StructAssets)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Equals(StructName, ESearchCase::IgnoreCase))
        {
            UObject* LoadedAsset = AssetData.GetAsset();
            if (UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(LoadedAsset))
            {
                return UserStruct;
            }
        }
    }
    
    return nullptr;
}
