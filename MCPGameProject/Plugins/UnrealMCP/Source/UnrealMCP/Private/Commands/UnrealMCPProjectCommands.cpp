#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
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

// Helper to get a user-friendly type string for a property
static FString GetPropertyTypeString(const FProperty* Property)
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

FUnrealMCPProjectCommands::FUnrealMCPProjectCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_input_mapping"))
    {
        return HandleCreateInputMapping(Params);
    }
    else if (CommandType == TEXT("create_folder"))
    {
        return HandleCreateFolder(Params);
    }
    else if (CommandType == TEXT("create_struct"))
    {
        return HandleCreateStruct(Params);
    }
    else if (CommandType == TEXT("update_struct"))
    {
        return HandleUpdateStruct(Params);
    }
    else if (CommandType == TEXT("get_project_dir"))
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("project_dir"), FPaths::ProjectDir());
        return ResultObj;
    }
    else if (CommandType == TEXT("show_struct_variables"))
    {
        return HandleShowStructVariables(Params);
    }
    else if (CommandType == TEXT("list_folder_contents"))
    {
        return HandleListFolderContents(Params);
    }
    else if (CommandType == TEXT("create_enhanced_input_action"))
    {
        return HandleCreateEnhancedInputAction(Params);
    }
    else if (CommandType == TEXT("create_input_mapping_context"))
    {
        return HandleCreateInputMappingContext(Params);
    }
    else if (CommandType == TEXT("add_mapping_to_context"))
    {
        return HandleAddMappingToContext(Params);
    }
    else if (CommandType == TEXT("list_input_actions"))
    {
        return HandleListInputActions(Params);
    }
    else if (CommandType == TEXT("list_input_mapping_contexts"))
    {
        return HandleListInputMappingContexts(Params);
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown project command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Get the input settings
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get input settings"));
    }

    // Create the input action mapping
    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    // Add modifiers if provided
    if (Params->HasField(TEXT("shift")))
    {
        ActionMapping.bShift = Params->GetBoolField(TEXT("shift"));
    }
    if (Params->HasField(TEXT("ctrl")))
    {
        ActionMapping.bCtrl = Params->GetBoolField(TEXT("ctrl"));
    }
    if (Params->HasField(TEXT("alt")))
    {
        ActionMapping.bAlt = Params->GetBoolField(TEXT("alt"));
    }
    if (Params->HasField(TEXT("cmd")))
    {
        ActionMapping.bCmd = Params->GetBoolField(TEXT("cmd"));
    }

    // Add the mapping
    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("key"), Key);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateFolder(const TSharedPtr<FJsonObject>& Params)
{
    FString FolderPath;
    if (!Params->TryGetStringField(TEXT("folder_path"), FolderPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'folder_path' parameter"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
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
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("folder_path"), FolderPath);
            ResultObj->SetBoolField(TEXT("already_exists"), true);
            return ResultObj;
        }
        
        if (!UEditorAssetLibrary::MakeDirectory(AssetPath))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create content folder: %s"), *AssetPath));
        }
    }
    else
    {
        // For non-content folders, use platform file system
        FString FullPath = FPaths::Combine(ProjectPath, FolderPath);
        
        // Check if directory already exists
        if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FullPath))
        {
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("folder_path"), FolderPath);
            ResultObj->SetBoolField(TEXT("already_exists"), true);
            return ResultObj;
        }
        
        if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FullPath))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create folder: %s"), *FullPath));
        }
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("folder_path"), FolderPath);
    ResultObj->SetBoolField(TEXT("already_exists"), false);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateStruct(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString StructName;
    if (!Params->TryGetStringField(TEXT("struct_name"), StructName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'struct_name' parameter"));
    }

    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray;
    if (!Params->TryGetArrayField(TEXT("properties"), PropertiesArray))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'properties' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    FString Description = TEXT("");
    Params->TryGetStringField(TEXT("description"), Description);

    // Make sure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create directory: %s"), *Path));
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

    // Check if the struct already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct already exists: %s"), *PackageName));
    }

    // Create the struct asset
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UStructureFactory* StructFactory = NewObject<UStructureFactory>();
    UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath.LeftChop(1), UUserDefinedStruct::StaticClass(), StructFactory);
    UUserDefinedStruct* NewStruct = Cast<UUserDefinedStruct>(CreatedAsset);
    
    if (!NewStruct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create struct asset"));
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
    for (const TSharedPtr<FJsonValue>& PropertyValue : *PropertiesArray)
    {
        TSharedPtr<FJsonObject> PropertyObj = PropertyValue->AsObject();
        if (!PropertyObj.IsValid())
        {
            continue;
        }

        FString PropertyName;
        if (!PropertyObj->TryGetStringField(TEXT("name"), PropertyName))
        {
            continue;
        }

        FString PropertyType;
        if (!PropertyObj->TryGetStringField(TEXT("type"), PropertyType))
        {
            continue;
        }

        FString PropertyTooltip;
        PropertyObj->TryGetStringField(TEXT("description"), PropertyTooltip);

        // Create the pin type
        FEdGraphPinType PinType;
        bool bTypeResolved = false;
        
        // Check if this is an array type
        if (PropertyType.EndsWith(TEXT("[]")))
        {
            // Get the base type without the array suffix
            FString BaseType = PropertyType.LeftChop(2);
            
            // Create a temporary pin type for the base type
            FEdGraphPinType BasePinType;
            
            // Resolve the base type
            if (BaseType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Int;
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Float;
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_String;
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                BasePinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                BasePinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                BasePinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
                bTypeResolved = true;
            }
            else if (BaseType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
            {
                BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                BasePinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
                bTypeResolved = true;
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
                        bTypeResolved = true;
                        break;
                    }
                }
            }
            
            if (bTypeResolved)
            {
                // Set up the array type
                PinType = BasePinType;
                PinType.ContainerType = EPinContainerType::Array;
            }
            else
            {
                // Default to string array if type not resolved
                PinType.PinCategory = UEdGraphSchema_K2::PC_String;
                PinType.ContainerType = EPinContainerType::Array;
                bTypeResolved = true;
            }
        }
        else
        {
            // Handle non-array types
            if (PropertyType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
            }
            else if (PropertyType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
            }
            else if (PropertyType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
            }
            else if (PropertyType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_String;
            }
            else if (PropertyType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
            }
            else if (PropertyType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
            }
            else if (PropertyType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
            }
            else if (PropertyType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
            }
            else if (PropertyType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
            }
            else
            {
                // Try to find a custom struct
                UScriptStruct* FoundStruct = nullptr;
                TArray<FString> StructNameVariations;
                StructNameVariations.Add(PropertyType);
                StructNameVariations.Add(FString::Printf(TEXT("F%s"), *PropertyType));
                StructNameVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Blueprints/%s.%s"), *PropertyType, *PropertyType)));
                StructNameVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("DataStructures/%s.%s"), *PropertyType, *PropertyType)));
                
                for (const FString& StructVariation : StructNameVariations)
                {
                    FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructVariation);
                    if (FoundStruct)
                    {
                        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                        PinType.PinSubCategoryObject = FoundStruct;
                        break;
                    }
                }
                
                if (!FoundStruct)
                {
                    // Default to string if type not recognized
                    PinType.PinCategory = UEdGraphSchema_K2::PC_String;
                }
            }
        }

        // First, add the variable
        bool bAdded = FStructureEditorUtils::AddVariable(NewStruct, PinType);
        if (!bAdded)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to add variable %s to struct %s"), *PropertyName, *StructName);
            continue;
        }

        // Get the updated variable list and find the last added variable
        const TArray<FStructVariableDescription>& VarDescArray = FStructureEditorUtils::GetVarDesc(NewStruct);
        if (VarDescArray.Num() > 0)
        {
            const FStructVariableDescription& NewVarDesc = VarDescArray.Last();
            
            // Rename the variable
            FStructureEditorUtils::RenameVariable(NewStruct, NewVarDesc.VarGuid, *PropertyName);
            
            // Set tooltip
            if (!PropertyTooltip.IsEmpty())
            {
                FStructureEditorUtils::ChangeVariableTooltip(NewStruct, NewVarDesc.VarGuid, PropertyTooltip);
            }
            
            // Force a recompile to ensure the variable is properly set up
            FStructureEditorUtils::CompileStructure(NewStruct);
            
            UE_LOG(LogTemp, Display, TEXT("Added variable %s to struct %s"), *PropertyName, *StructName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to find newly added variable %s in struct %s"), *PropertyName, *StructName);
        }
    }

    // Clean up any remaining unrenamed variables (MemberVar_)
    TArray<FGuid> GuidsToRemove;
    for (const FStructVariableDescription& Desc : FStructureEditorUtils::GetVarDesc(NewStruct))
    {
        if (Desc.VarName.ToString().StartsWith(TEXT("MemberVar_")))
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP CreateStruct] Will remove leftover variable: %s"), *Desc.VarName.ToString());
            GuidsToRemove.Add(Desc.VarGuid);
        }
    }
    for (const FGuid& Guid : GuidsToRemove)
    {
        FStructureEditorUtils::RemoveVariable(NewStruct, Guid);
        UE_LOG(LogTemp, Warning, TEXT("[MCP CreateStruct] Removed variable with GUID: %s"), *Guid.ToString());
    }
    UE_LOG(LogTemp, Warning, TEXT("[MCP CreateStruct] Cleanup removed %d variables."), GuidsToRemove.Num());

    // Final compilation and save
    FStructureEditorUtils::CompileStructure(NewStruct);
    NewStruct->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewStruct);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("struct_name"), StructName);
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetStringField(TEXT("full_path"), PackageName);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleUpdateStruct(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString StructName;
    if (!Params->TryGetStringField(TEXT("struct_name"), StructName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'struct_name' parameter"));
    }

    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray;
    if (!Params->TryGetArrayField(TEXT("properties"), PropertiesArray))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'properties' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    FString Description = TEXT("");
    Params->TryGetStringField(TEXT("description"), Description);

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
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct does not exist: %s"), *PackageName));
    }

    UObject* AssetObj = UEditorAssetLibrary::LoadAsset(PackageName);
    UUserDefinedStruct* ExistingStruct = Cast<UUserDefinedStruct>(AssetObj);
    if (!ExistingStruct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load struct asset"));
    }

    // Set the struct description and tooltip
    if (!Description.IsEmpty())
    {
        ExistingStruct->SetMetaData(TEXT("Comments"), *Description);
        FStructureEditorUtils::ChangeTooltip(ExistingStruct, Description);
    }

    // Build a map of existing variables by name
    TMap<FString, FStructVariableDescription> ExistingVarsByName;
    auto InitialVarDescArray = FStructureEditorUtils::GetVarDesc(ExistingStruct);
    UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Initial variables in struct:"));
    for (const FStructVariableDescription& Desc : InitialVarDescArray) {
        UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Desc.VarName.ToString());
    }
    for (const FStructVariableDescription& Desc : InitialVarDescArray)
    {
        ExistingVarsByName.Add(Desc.VarName.ToString(), Desc);
    }

    // Track which variables were updated or added
    TSet<FString> UpdatedOrAddedNames;

    int32 PropertyIndex = 0;
    for (const TSharedPtr<FJsonValue>& PropertyValue : *PropertiesArray)
    {
        TSharedPtr<FJsonObject> PropertyObj = PropertyValue->AsObject();
        if (!PropertyObj.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Property at index %d is invalid, skipping."), PropertyIndex);
            ++PropertyIndex;
            continue;
        }

        FString PropertyName;
        if (!PropertyObj->TryGetStringField(TEXT("name"), PropertyName))
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Property at index %d missing 'name', skipping."), PropertyIndex);
            ++PropertyIndex;
            continue;
        }

        FString PropertyType;
        if (!PropertyObj->TryGetStringField(TEXT("type"), PropertyType))
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Property '%s' missing 'type', skipping."), *PropertyName);
            ++PropertyIndex;
            continue;
        }

        FString PropertyTooltip;
        PropertyObj->TryGetStringField(TEXT("description"), PropertyTooltip);

        if (ExistingVarsByName.Contains(PropertyName))
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Property '%s' already exists, will update tooltip if needed."), *PropertyName);
            // Update tooltip if needed (type change is not supported by UE, so log if mismatch)
            const FStructVariableDescription& ExistingDesc = ExistingVarsByName[PropertyName];
            // There is no Type member in FStructVariableDescription in UE 5.5, so we cannot compare types directly.
            // Instead, just update the tooltip if needed and log that type changes are not supported.
            if (!PropertyTooltip.IsEmpty() && PropertyTooltip != ExistingDesc.ToolTip)
            {
                FStructureEditorUtils::ChangeVariableTooltip(ExistingStruct, ExistingDesc.VarGuid, PropertyTooltip);
                UE_LOG(LogTemp, Display, TEXT("[MCP UpdateStruct] Updated tooltip for '%s' to '%s'."), *PropertyName, *PropertyTooltip);
            }
            // Log that type changes are not supported
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Type change for '%s' is not supported and will be ignored (cannot check type in UE 5.5)."), *PropertyName);
            UpdatedOrAddedNames.Add(PropertyName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Property '%s' does not exist, will add as new variable."), *PropertyName);
            // Add new variable
            FEdGraphPinType PinType;
            bool bTypeResolved = false;
            
            // Check if this is an array type
            if (PropertyType.EndsWith(TEXT("[]")))
            {
                // Get the base type without the array suffix
                FString BaseType = PropertyType.LeftChop(2);
                
                // Create a temporary pin type for the base type
                FEdGraphPinType BasePinType;
                
                // Resolve the base type
                if (BaseType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Int;
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Float;
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_String;
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    BasePinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    BasePinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    BasePinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
                    bTypeResolved = true;
                }
                else if (BaseType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
                {
                    BasePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    BasePinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
                    bTypeResolved = true;
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
                            bTypeResolved = true;
                            break;
                        }
                    }
                }
                
                if (bTypeResolved)
                {
                    // Set up the array type
                    PinType = BasePinType;
                    PinType.ContainerType = EPinContainerType::Array;
                }
                else
                {
                    // Default to string array if type not resolved
                    PinType.PinCategory = UEdGraphSchema_K2::PC_String;
                    PinType.ContainerType = EPinContainerType::Array;
                    bTypeResolved = true;
                }
            }
            else
            {
                // Handle non-array types
                if (PropertyType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
                }
                else if (PropertyType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
                }
                else if (PropertyType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
                }
                else if (PropertyType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_String;
                }
                else if (PropertyType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
                }
                else if (PropertyType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
                }
                else if (PropertyType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
                }
                else if (PropertyType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
                }
                else if (PropertyType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
                {
                    PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
                }
                else
                {
                    // Try to find a custom struct
                    UScriptStruct* FoundStruct = nullptr;
                    TArray<FString> StructNameVariations;
                    StructNameVariations.Add(PropertyType);
                    StructNameVariations.Add(FString::Printf(TEXT("F%s"), *PropertyType));
                    StructNameVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Blueprints/%s.%s"), *PropertyType, *PropertyType)));
                    StructNameVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("DataStructures/%s.%s"), *PropertyType, *PropertyType)));
                    
                    for (const FString& StructVariation : StructNameVariations)
                    {
                        FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructVariation);
                        if (FoundStruct)
                        {
                            PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                            PinType.PinSubCategoryObject = FoundStruct;
                            break;
                        }
                    }
                    
                    if (!FoundStruct)
                    {
                        // Default to string if type not recognized
                        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
                    }
                }
            }

            FStructureEditorUtils::AddVariable(ExistingStruct, PinType);
            // Find and rename the just-added variable
            auto VarDescArray = FStructureEditorUtils::GetVarDesc(ExistingStruct);
            for (const FStructVariableDescription& Desc : VarDescArray)
            {
                if (Desc.VarName.ToString().StartsWith(TEXT("MemberVar_")))
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Renaming new variable %s to %s"), *Desc.VarName.ToString(), *PropertyName);
                    FStructureEditorUtils::RenameVariable(ExistingStruct, Desc.VarGuid, PropertyName);
                    if (!PropertyTooltip.IsEmpty())
                    {
                        FStructureEditorUtils::ChangeVariableTooltip(ExistingStruct, Desc.VarGuid, PropertyTooltip);
                    }
                    UE_LOG(LogTemp, Display, TEXT("[MCP UpdateStruct] Added new variable '%s' (type: %s)."), *PropertyName, *PropertyType);
                    break;
                }
            }
            UpdatedOrAddedNames.Add(PropertyName);
        }
        ++PropertyIndex;
    }

    // Final compilation
    FStructureEditorUtils::CompileStructure(ExistingStruct);
    ExistingStruct->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(ExistingStruct);
    UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Variables in struct after update, before cleanup:"));
    auto PreCleanupVarDescArray = FStructureEditorUtils::GetVarDesc(ExistingStruct);
    for (const FStructVariableDescription& Desc : PreCleanupVarDescArray) {
        UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Desc.VarName.ToString());
    }
    // Clean up any remaining unrenamed variables (MemberVar_)
    TArray<FGuid> GuidsToRemove;
    for (const FStructVariableDescription& Desc : FStructureEditorUtils::GetVarDesc(ExistingStruct))
    {
        if (Desc.VarName.ToString().StartsWith(TEXT("MemberVar_")))
        {
            UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Will remove leftover variable: %s"), *Desc.VarName.ToString());
            GuidsToRemove.Add(Desc.VarGuid);
        }
    }
    for (const FGuid& Guid : GuidsToRemove)
    {
        FStructureEditorUtils::RemoveVariable(ExistingStruct, Guid);
        UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Removed variable with GUID: %s"), *Guid.ToString());
    }
    UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Cleanup removed %d variables."), GuidsToRemove.Num());
    // Log final state
    auto FinalVarDescArray = FStructureEditorUtils::GetVarDesc(ExistingStruct);
    UE_LOG(LogTemp, Warning, TEXT("[MCP UpdateStruct] Final variables in struct after cleanup:"));
    for (const FStructVariableDescription& Desc : FinalVarDescArray) {
        UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Desc.VarName.ToString());
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("struct_name"), StructName);
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetStringField(TEXT("full_path"), PackageName);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleShowStructVariables(const TSharedPtr<FJsonObject>& Params)
{
    FString StructName;
    if (!Params->TryGetStringField(TEXT("struct_name"), StructName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'struct_name' parameter"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    FString AssetName = StructName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;

    if (!UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct does not exist: %s"), *PackageName));
    }

    UObject* AssetObj = UEditorAssetLibrary::LoadAsset(PackageName);
    UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(AssetObj);
    if (!Struct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load struct asset"));
    }

    auto VarDescArray = FStructureEditorUtils::GetVarDesc(Struct);
    TArray<TSharedPtr<FJsonValue>> VariablesJson;
    for (const FStructVariableDescription& Desc : VarDescArray)
    {
        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Desc.VarName.ToString());
        if (FProperty* Property = Struct->FindPropertyByName(Desc.VarName))
        {
            FString TypeString = GetPropertyTypeString(Property);
            VarObj->SetStringField(TEXT("type"), TypeString);
        }
        else
        {
            VarObj->SetStringField(TEXT("type"), TEXT("Unknown"));
        }
        VarObj->SetStringField(TEXT("tooltip"), Desc.ToolTip);
        VariablesJson.Add(MakeShared<FJsonValueObject>(VarObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("struct_name"), StructName);
    ResultObj->SetArrayField(TEXT("variables"), VariablesJson);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListFolderContents(const TSharedPtr<FJsonObject>& Params)
{
    FString FolderPath;
    if (!Params->TryGetStringField(TEXT("folder_path"), FolderPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'folder_path' parameter"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> SubfoldersJson;
    TArray<TSharedPtr<FJsonValue>> FilesJson;

    // Determine if this is a content folder
    bool bIsContentFolder = FolderPath.StartsWith(TEXT("/Content/")) || FolderPath.StartsWith(TEXT("Content/")) || FolderPath.StartsWith(TEXT("/Game/")) || FolderPath.StartsWith(TEXT("Game/"));
    FString AssetPath = FolderPath;
    if (bIsContentFolder)
    {
        // Normalize all /Content, /Content/, /Game/Content, /Game/Content/ to /Game
        if (AssetPath == TEXT("/Content") || AssetPath == TEXT("/Content/") || AssetPath == TEXT("Content") || AssetPath == TEXT("Content/"))
        {
            AssetPath = TEXT("/Game");
        }
        else if (AssetPath.StartsWith(TEXT("/Content/")))
        {
            AssetPath = TEXT("/Game/") + AssetPath.RightChop(9); // "/Content/" is 9 chars
        }
        else if (AssetPath.StartsWith(TEXT("Content/")))
        {
            AssetPath = TEXT("/Game/") + AssetPath.RightChop(8); // "Content/" is 8 chars
        }
        else if (AssetPath == TEXT("/Game/Content") || AssetPath == TEXT("/Game/Content/"))
        {
            AssetPath = TEXT("/Game");
        }
        else if (AssetPath.StartsWith(TEXT("/Game/Content/")))
        {
            AssetPath = TEXT("/Game/") + AssetPath.RightChop(14); // "/Game/Content/" is 14 chars
        }
        else if (AssetPath.StartsWith(TEXT("Game/")))
        {
            AssetPath = TEXT("/Game/") + AssetPath.RightChop(5);
        }
        // Normalize AssetPath for root and remove trailing slash if present
        if (AssetPath.EndsWith(TEXT("/")))
        {
            AssetPath = AssetPath.LeftChop(1);
        }
        UE_LOG(LogTemp, Display, TEXT("About to call GetSubPaths for folder: %s"), *AssetPath);
        // Use AssetRegistry to get subfolders
        TArray<FString> Subfolders;
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FString> RootSubfolders;
        AssetRegistryModule.Get().GetSubPaths(TEXT(""), RootSubfolders, false);
        UE_LOG(LogTemp, Warning, TEXT("GetSubPaths(\"\") returned: %s"), *FString::Join(RootSubfolders, TEXT(", ")));
        for (const FString& Subfolder : RootSubfolders)
        {
            if (!Subfolders.Contains(Subfolder))
                Subfolders.Add(Subfolder);
        }
        TArray<FString> PathSubfolders;
        AssetRegistryModule.Get().GetSubPaths(AssetPath, PathSubfolders, false);
        UE_LOG(LogTemp, Warning, TEXT("GetSubPaths(%s) returned: %s"), *AssetPath, *FString::Join(PathSubfolders, TEXT(", ")));
        for (const FString& Subfolder : PathSubfolders)
        {
            if (!Subfolders.Contains(Subfolder))
                Subfolders.Add(Subfolder);
        }
        for (const FString& Subfolder : Subfolders)
        {
            SubfoldersJson.Add(MakeShared<FJsonValueString>(Subfolder));
        }
        // List assets/files
        TArray<FString> Assets = UEditorAssetLibrary::ListAssets(AssetPath, false);
        for (const FString& Asset : Assets)
        {
            FilesJson.Add(MakeShared<FJsonValueString>(Asset));
        }
    }
    else
    {
        // Use platform file system for non-content folders
        FString ProjectPath = FPaths::ProjectDir();
        FString FullPath = FPaths::Combine(ProjectPath, FolderPath);
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        // List subfolders and files
        PlatformFile.IterateDirectory(*FullPath, [&SubfoldersJson, &FilesJson, &PlatformFile](const TCHAR* Path, bool bIsDirectory) {
            FString Item(Path);
            if (bIsDirectory)
            {
                SubfoldersJson.Add(MakeShared<FJsonValueString>(Item));
            }
            else
            {
                FilesJson.Add(MakeShared<FJsonValueString>(Item));
            }
            return true;
        });
    }
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("subfolders"), SubfoldersJson);
    ResultObj->SetArrayField(TEXT("files"), FilesJson);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateEnhancedInputAction(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/Input/Actions");
    Params->TryGetStringField(TEXT("path"), Path);

    FString Description = TEXT("");
    Params->TryGetStringField(TEXT("description"), Description);

    FString ValueType = TEXT("Digital");  // Digital, Analog, Axis2D, Axis3D
    Params->TryGetStringField(TEXT("value_type"), ValueType);

    // Make sure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create directory: %s"), *Path));
        }
    }

    // Create the asset name with IA_ prefix if not already present
    FString AssetName = ActionName;
    if (!AssetName.StartsWith(TEXT("IA_")))
    {
        AssetName = TEXT("IA_") + AssetName;
    }

    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;

    // Check if the action already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("action_name"), ActionName);
        ResultObj->SetStringField(TEXT("asset_name"), AssetName);
        ResultObj->SetStringField(TEXT("path"), Path);
        ResultObj->SetStringField(TEXT("full_path"), PackageName);
        ResultObj->SetBoolField(TEXT("already_exists"), true);
        return ResultObj;
    }

    // Create the input action asset using proper UE5.5 workflow
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    
    // Create the package first
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }
    Package->FullyLoad();

    // Create the input action asset directly in the target package
    UInputAction* NewInputAction = NewObject<UInputAction>(Package, UInputAction::StaticClass(), *AssetName, RF_Public | RF_Standalone);
    if (!NewInputAction)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Input Action asset"));
    }

    // Set value type
    if (ValueType == TEXT("Digital"))
    {
        NewInputAction->ValueType = EInputActionValueType::Boolean;
    }
    else if (ValueType == TEXT("Analog"))
    {
        NewInputAction->ValueType = EInputActionValueType::Axis1D;
    }
    else if (ValueType == TEXT("Axis2D"))
    {
        NewInputAction->ValueType = EInputActionValueType::Axis2D;
    }
    else if (ValueType == TEXT("Axis3D"))
    {
        NewInputAction->ValueType = EInputActionValueType::Axis3D;
    }

    // Mark package dirty and register with asset registry
    Package->MarkPackageDirty();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.AssetCreated(NewInputAction);

    // Save the asset and package
    if (!UEditorAssetLibrary::SaveAsset(PackageName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to save Input Action asset"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("asset_name"), AssetName);
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetStringField(TEXT("full_path"), PackageName);
    ResultObj->SetStringField(TEXT("value_type"), ValueType);
    ResultObj->SetBoolField(TEXT("already_exists"), false);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ContextName;
    if (!Params->TryGetStringField(TEXT("context_name"), ContextName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'context_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/Input");
    Params->TryGetStringField(TEXT("path"), Path);

    FString Description = TEXT("");
    Params->TryGetStringField(TEXT("description"), Description);

    // Make sure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create directory: %s"), *Path));
        }
    }

    // Create the asset name with IMC_ prefix if not already present
    FString AssetName = ContextName;
    if (!AssetName.StartsWith(TEXT("IMC_")))
    {
        AssetName = TEXT("IMC_") + AssetName;
    }

    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;

    // Check if the context already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("context_name"), ContextName);
        ResultObj->SetStringField(TEXT("asset_name"), AssetName);
        ResultObj->SetStringField(TEXT("path"), Path);
        ResultObj->SetStringField(TEXT("full_path"), PackageName);
        ResultObj->SetBoolField(TEXT("already_exists"), true);
        return ResultObj;
    }

    // Create the input mapping context asset using proper UE5.5 workflow
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    
    // Create the package first
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }
    Package->FullyLoad();

    // Create the input mapping context asset directly in the target package
    UInputMappingContext* NewMappingContext = NewObject<UInputMappingContext>(Package, UInputMappingContext::StaticClass(), *AssetName, RF_Public | RF_Standalone);
    if (!NewMappingContext)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Input Mapping Context asset"));
    }

    // Mark package dirty and register with asset registry
    Package->MarkPackageDirty();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.AssetCreated(NewMappingContext);

    // Save the asset and package
    if (!UEditorAssetLibrary::SaveAsset(PackageName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to save Input Mapping Context asset"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context_name"), ContextName);
    ResultObj->SetStringField(TEXT("asset_name"), AssetName);
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetStringField(TEXT("full_path"), PackageName);
    ResultObj->SetBoolField(TEXT("already_exists"), false);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleAddMappingToContext(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ContextPath;
    if (!Params->TryGetStringField(TEXT("context_path"), ContextPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'context_path' parameter"));
    }

    FString ActionPath;
    if (!Params->TryGetStringField(TEXT("action_path"), ActionPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_path' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Load the mapping context
    UInputMappingContext* MappingContext = Cast<UInputMappingContext>(UEditorAssetLibrary::LoadAsset(ContextPath));
    if (!MappingContext)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load Input Mapping Context: %s"), *ContextPath));
    }

    // Load the input action
    UInputAction* InputAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(ActionPath));
    if (!InputAction)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load Input Action: %s"), *ActionPath));
    }

    // Create the mapping
    FEnhancedActionKeyMapping& NewMapping = MappingContext->MapKey(InputAction, FKey(*Key));

    // Set optional modifiers if provided
    if (Params->HasField(TEXT("shift")))
    {
        bool bShift = Params->GetBoolField(TEXT("shift"));
        // Note: Enhanced Input uses different modifiers system than legacy
        // This would need to be implemented with proper modifiers/triggers
    }

    // Mark the context as dirty and save
    MappingContext->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(ContextPath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context_path"), ContextPath);
    ResultObj->SetStringField(TEXT("action_path"), ActionPath);
    ResultObj->SetStringField(TEXT("key"), Key);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListInputActions(const TSharedPtr<FJsonObject>& Params)
{
    // Get optional path parameter
    FString Path = TEXT("/Game");
    Params->TryGetStringField(TEXT("path"), Path);

    // Get all Input Action assets using ListAssets instead of FindAssetData for UE 5.5
    TArray<FString> AllAssets = UEditorAssetLibrary::ListAssets(Path, true, false);
    TArray<FString> InputActionPaths;
    
    // Filter for Input Action assets
    for (const FString& AssetPath : AllAssets)
    {
        if (UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath))
        {
            if (Asset->IsA<UInputAction>())
            {
                InputActionPaths.Add(AssetPath);
            }
        }
    }

    TArray<TSharedPtr<FJsonValue>> ActionsJson;
    for (const FString& ActionPath : InputActionPaths)
    {
        TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
        ActionObj->SetStringField(TEXT("path"), ActionPath);
        
        // Extract asset name from path
        FString AssetName = FPaths::GetBaseFilename(ActionPath);
        ActionObj->SetStringField(TEXT("name"), AssetName);
        
        // Try to load the asset to get more details
        if (UInputAction* InputAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(ActionPath)))
        {
            FString ValueTypeString;
            switch (InputAction->ValueType)
            {
                case EInputActionValueType::Boolean:
                    ValueTypeString = TEXT("Digital");
                    break;
                case EInputActionValueType::Axis1D:
                    ValueTypeString = TEXT("Analog");
                    break;
                case EInputActionValueType::Axis2D:
                    ValueTypeString = TEXT("Axis2D");
                    break;
                case EInputActionValueType::Axis3D:
                    ValueTypeString = TEXT("Axis3D");
                    break;
                default:
                    ValueTypeString = TEXT("Unknown");
                    break;
            }
            ActionObj->SetStringField(TEXT("value_type"), ValueTypeString);
        }
        
        ActionsJson.Add(MakeShared<FJsonValueObject>(ActionObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetArrayField(TEXT("input_actions"), ActionsJson);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListInputMappingContexts(const TSharedPtr<FJsonObject>& Params)
{
    // Get optional path parameter
    FString Path = TEXT("/Game");
    Params->TryGetStringField(TEXT("path"), Path);

    // Get all Input Mapping Context assets using ListAssets instead of FindAssetData for UE 5.5
    TArray<FString> AllAssets = UEditorAssetLibrary::ListAssets(Path, true, false);
    TArray<FString> ContextPaths;
    
    // Filter for Input Mapping Context assets
    for (const FString& AssetPath : AllAssets)
    {
        if (UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath))
        {
            if (Asset->IsA<UInputMappingContext>())
            {
                ContextPaths.Add(AssetPath);
            }
        }
    }

    TArray<TSharedPtr<FJsonValue>> ContextsJson;
    for (const FString& ContextPath : ContextPaths)
    {
        TSharedPtr<FJsonObject> ContextObj = MakeShared<FJsonObject>();
        ContextObj->SetStringField(TEXT("path"), ContextPath);
        
        // Extract asset name from path
        FString AssetName = FPaths::GetBaseFilename(ContextPath);
        ContextObj->SetStringField(TEXT("name"), AssetName);
        
        // Try to load the asset to get more details
        if (UInputMappingContext* MappingContext = Cast<UInputMappingContext>(UEditorAssetLibrary::LoadAsset(ContextPath)))
        {
            TArray<TSharedPtr<FJsonValue>> MappingsJson;
            for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
            {
                TSharedPtr<FJsonObject> MappingObj = MakeShared<FJsonObject>();
                if (Mapping.Action)
                {
                    MappingObj->SetStringField(TEXT("action"), Mapping.Action->GetName());
                }
                MappingObj->SetStringField(TEXT("key"), Mapping.Key.ToString());
                MappingsJson.Add(MakeShared<FJsonValueObject>(MappingObj));
            }
            ContextObj->SetArrayField(TEXT("mappings"), MappingsJson);
        }
        
        ContextsJson.Add(MakeShared<FJsonValueObject>(ContextObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetArrayField(TEXT("input_mapping_contexts"), ContextsJson);
    return ResultObj;
}
