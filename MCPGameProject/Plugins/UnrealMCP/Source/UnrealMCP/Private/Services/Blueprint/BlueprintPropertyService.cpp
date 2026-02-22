#include "Services/Blueprint/BlueprintPropertyService.h"
#include "Services/Blueprint/BlueprintCacheService.h"
#include "Services/PropertyService.h"
#include "Services/ComponentService.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "GameFramework/Pawn.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "StructUtils/UserDefinedStruct.h"

bool FBlueprintPropertyService::AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, const FString& VariableType, bool bIsExposed, FBlueprintCache& Cache)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddVariableToBlueprint: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddVariableToBlueprint: Adding variable '%s' of type '%s' to blueprint '%s'"),
        *VariableName, *VariableType, *Blueprint->GetName());

    // Check for container types: Array<X>, Set<X>, Map<K,V>
    bool bIsArray = false;
    bool bIsSet = false;
    FString InnerTypeName = VariableType;
    
    if (VariableType.StartsWith(TEXT("Array<")) || VariableType.StartsWith(TEXT("TArray<")))
    {
        bIsArray = true;
        int32 OpenBracket = VariableType.Find(TEXT("<"));
        int32 CloseBracket = VariableType.Find(TEXT(">"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (OpenBracket != INDEX_NONE && CloseBracket != INDEX_NONE)
        {
            InnerTypeName = VariableType.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1).TrimStartAndEnd();
        }
        UE_LOG(LogTemp, Log, TEXT("AddVariableToBlueprint: Detected Array container, inner type: '%s'"), *InnerTypeName);
    }
    else if (VariableType.StartsWith(TEXT("Set<")) || VariableType.StartsWith(TEXT("TSet<")))
    {
        bIsSet = true;
        int32 OpenBracket = VariableType.Find(TEXT("<"));
        int32 CloseBracket = VariableType.Find(TEXT(">"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (OpenBracket != INDEX_NONE && CloseBracket != INDEX_NONE)
        {
            InnerTypeName = VariableType.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1).TrimStartAndEnd();
        }
        UE_LOG(LogTemp, Log, TEXT("AddVariableToBlueprint: Detected Set container, inner type: '%s'"), *InnerTypeName);
    }

    // Resolve variable type (using inner type for containers)
    UObject* TypeObject = ResolveVariableType(InnerTypeName);
    if (!TypeObject)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddVariableToBlueprint: Unknown variable type '%s'"), *InnerTypeName);
        return false;
    }

    // Create variable description
    FBPVariableDescription NewVar;
    NewVar.VarName = *VariableName;
    NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Object; // Default, will be adjusted based on type
    
    // Set container type
    if (bIsArray)
    {
        NewVar.VarType.ContainerType = EPinContainerType::Array;
    }
    else if (bIsSet)
    {
        NewVar.VarType.ContainerType = EPinContainerType::Set;
    }

    // Set type based on resolved type object
    // Check if this is a Class reference type (TSubclassOf)
    if (InnerTypeName.StartsWith(TEXT("Class")) || InnerTypeName.StartsWith(TEXT("TSubclassOf")))
    {
        NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Class;

        // Extract the inner class type if specified: "Class<UserWidget>" -> "UserWidget"
        FString InnerClassName;
        int32 OpenBracket = InnerTypeName.Find(TEXT("<"));
        int32 CloseBracket = InnerTypeName.Find(TEXT(">"));
        if (OpenBracket != INDEX_NONE && CloseBracket != INDEX_NONE && CloseBracket > OpenBracket)
        {
            InnerClassName = InnerTypeName.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);
        }

        // Default to UObject if no inner class specified
        UClass* MetaClass = UObject::StaticClass();
        if (!InnerClassName.IsEmpty())
        {
            // Try to find the inner class
            UObject* InnerClassObj = ResolveVariableType(InnerClassName);
            if (UClass* FoundInnerClass = Cast<UClass>(InnerClassObj))
            {
                MetaClass = FoundInnerClass;
            }
        }
        NewVar.VarType.PinSubCategoryObject = MetaClass;
        UE_LOG(LogTemp, Log, TEXT("AddVariableToBlueprint: Creating Class reference variable with meta class '%s'"), *MetaClass->GetName());
    }
    else if (UClass* ClassType = Cast<UClass>(TypeObject))
    {
        NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Object;
        NewVar.VarType.PinSubCategoryObject = ClassType;
    }
    else if (UScriptStruct* StructType = Cast<UScriptStruct>(TypeObject))
    {
        NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        NewVar.VarType.PinSubCategoryObject = StructType;
    }
    else
    {
        // Handle basic types (use InnerTypeName for container support)
        if (InnerTypeName == TEXT("Boolean") || InnerTypeName == TEXT("bool"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else if (InnerTypeName == TEXT("Integer") || InnerTypeName == TEXT("int") || InnerTypeName == TEXT("int32"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (InnerTypeName == TEXT("Float") || InnerTypeName == TEXT("float"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Real;
            NewVar.VarType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (InnerTypeName == TEXT("String") || InnerTypeName == TEXT("FString"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (InnerTypeName == TEXT("Vector") || InnerTypeName == TEXT("FVector"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            NewVar.VarType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        }
        else if (InnerTypeName == TEXT("Rotator") || InnerTypeName == TEXT("FRotator"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            NewVar.VarType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        }
        else if (InnerTypeName == TEXT("Name") || InnerTypeName == TEXT("FName"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Name;
        }
        else if (InnerTypeName == TEXT("Text") || InnerTypeName == TEXT("FText"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Text;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddVariableToBlueprint: Unsupported basic type '%s'"), *InnerTypeName);
            return false;
        }
    }

    // Set exposure
    if (bIsExposed)
    {
        NewVar.PropertyFlags |= CPF_BlueprintVisible;
        NewVar.PropertyFlags |= CPF_Edit;
    }

    // Add variable to blueprint
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, NewVar.VarName, NewVar.VarType);

    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Compile blueprint so the variable is immediately available on the CDO
    // This is necessary for set_blueprint_variable_value to work right after adding a variable
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Invalidate cache since blueprint was modified
    Cache.InvalidateBlueprint(Blueprint->GetName());

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddVariableToBlueprint: Successfully added variable '%s'"), *VariableName);
    return true;
}

bool FBlueprintPropertyService::SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutErrorMessage, FBlueprintCache& Cache)
{
    if (!Blueprint)
    {
        OutErrorMessage = TEXT("Invalid blueprint");
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetBlueprintProperty: %s"), *OutErrorMessage);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetBlueprintProperty: Setting property '%s' on blueprint '%s'"),
        *PropertyName, *Blueprint->GetName());

    // Get the blueprint's default object
    UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    if (!DefaultObject)
    {
        OutErrorMessage = FString::Printf(TEXT("No default object available for blueprint '%s'. Try compiling the blueprint first."), *Blueprint->GetName());
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetBlueprintProperty: %s"), *OutErrorMessage);
        return false;
    }

    // Set the property using PropertyService
    if (!FPropertyService::Get().SetObjectProperty(DefaultObject, PropertyName, PropertyValue, OutErrorMessage))
    {
        // OutErrorMessage is already set by PropertyService
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetBlueprintProperty: Failed to set property - %s"), *OutErrorMessage);
        return false;
    }

    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Invalidate cache since blueprint was modified
    Cache.InvalidateBlueprint(Blueprint->GetName());

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetBlueprintProperty: Successfully set property '%s'"), *PropertyName);
    return true;
}

bool FBlueprintPropertyService::SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TMap<FString, float>& PhysicsParams, FBlueprintCache& Cache)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetPhysicsProperties: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetPhysicsProperties: Setting physics properties on component '%s' in blueprint '%s'"),
        *ComponentName, *Blueprint->GetName());

    // Convert TMap to JSON object for ComponentService
    TSharedPtr<FJsonObject> PhysicsJsonParams = MakeShared<FJsonObject>();
    for (const auto& Param : PhysicsParams)
    {
        PhysicsJsonParams->SetNumberField(Param.Key, Param.Value);
    }

    // Delegate to ComponentService for physics operations
    bool bResult = FComponentService::Get().SetPhysicsProperties(Blueprint, ComponentName, PhysicsJsonParams);

    if (bResult)
    {
        // Invalidate cache since blueprint was modified
        Cache.InvalidateBlueprint(Blueprint->GetName());
    }

    return bResult;
}

bool FBlueprintPropertyService::GetBlueprintComponents(UBlueprint* Blueprint, TArray<TPair<FString, FString>>& OutComponents)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::GetBlueprintComponents: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::GetBlueprintComponents: Getting components for blueprint '%s'"), *Blueprint->GetName());

    OutComponents.Empty();

    // Get components from Simple Construction Script
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->ComponentTemplate)
            {
                FString ComponentName = Node->GetVariableName().ToString();
                FString ComponentType = Node->ComponentTemplate->GetClass()->GetName();
                OutComponents.Add(TPair<FString, FString>(ComponentName, ComponentType));
            }
        }
    }

    // Get inherited components from CDO
    if (Blueprint->GeneratedClass)
    {
        UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
        AActor* DefaultActor = Cast<AActor>(DefaultObject);
        if (DefaultActor)
        {
            TArray<UActorComponent*> AllComponents;
            DefaultActor->GetComponents(AllComponents);
            for (UActorComponent* Component : AllComponents)
            {
                if (Component)
                {
                    FString ComponentName = Component->GetName();
                    FString ComponentType = Component->GetClass()->GetName();

                    // Check if already added from SCS
                    bool bAlreadyAdded = false;
                    for (const auto& ExistingComponent : OutComponents)
                    {
                        if (ExistingComponent.Key == ComponentName)
                        {
                            bAlreadyAdded = true;
                            break;
                        }
                    }

                    if (!bAlreadyAdded)
                    {
                        OutComponents.Add(TPair<FString, FString>(ComponentName, ComponentType));
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::GetBlueprintComponents: Found %d components"), OutComponents.Num());
    return true;
}

bool FBlueprintPropertyService::SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath, FBlueprintCache& Cache)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetStaticMeshProperties: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetStaticMeshProperties: Setting static mesh '%s' on component '%s' in blueprint '%s'"),
        *StaticMeshPath, *ComponentName, *Blueprint->GetName());

    // Delegate to ComponentService for static mesh operations
    bool bResult = FComponentService::Get().SetStaticMeshProperties(Blueprint, ComponentName, StaticMeshPath);

    if (bResult)
    {
        // Invalidate cache since blueprint was modified
        Cache.InvalidateBlueprint(Blueprint->GetName());
    }

    return bResult;
}

bool FBlueprintPropertyService::SetPawnProperties(UBlueprint* Blueprint, const TMap<FString, FString>& PawnParams, FBlueprintCache& Cache)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetPawnProperties: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetPawnProperties: Setting pawn properties on blueprint '%s'"), *Blueprint->GetName());

    // Get the blueprint's default object
    UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    APawn* DefaultPawn = Cast<APawn>(DefaultObject);
    if (!DefaultPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetPawnProperties: Blueprint is not a Pawn or Character"));
        return false;
    }

    // Set pawn properties
    for (const auto& Param : PawnParams)
    {
        const FString& PropertyName = Param.Key;
        const FString& PropertyValue = Param.Value;

        if (PropertyName == TEXT("auto_possess_player"))
        {
            // Handle auto possess player setting
            EAutoReceiveInput::Type AutoPossessType = EAutoReceiveInput::Disabled;
            if (PropertyValue == TEXT("Player0"))
            {
                AutoPossessType = EAutoReceiveInput::Player0;
            }
            else if (PropertyValue == TEXT("Player1"))
            {
                AutoPossessType = EAutoReceiveInput::Player1;
            }
            DefaultPawn->AutoPossessPlayer = AutoPossessType;
        }
        else if (PropertyName == TEXT("use_controller_rotation_yaw"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->bUseControllerRotationYaw = bValue;
        }
        else if (PropertyName == TEXT("use_controller_rotation_pitch"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->bUseControllerRotationPitch = bValue;
        }
        else if (PropertyName == TEXT("use_controller_rotation_roll"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->bUseControllerRotationRoll = bValue;
        }
        else if (PropertyName == TEXT("can_be_damaged"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->SetCanBeDamaged(bValue);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::SetPawnProperties: Unknown pawn property '%s'"), *PropertyName);
        }
    }

    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Invalidate cache since blueprint was modified
    Cache.InvalidateBlueprint(Blueprint->GetName());

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetPawnProperties: Successfully set pawn properties"));
    return true;
}

// Private helper methods
UObject* FBlueprintPropertyService::ResolveVariableType(const FString& TypeString) const
{
    // Handle basic types (these don't need UObject resolution)
    if (TypeString == TEXT("Boolean") || TypeString == TEXT("bool") ||
        TypeString == TEXT("Integer") || TypeString == TEXT("int") || TypeString == TEXT("int32") ||
        TypeString == TEXT("Float") || TypeString == TEXT("float") ||
        TypeString == TEXT("String") || TypeString == TEXT("FString") ||
        TypeString == TEXT("Vector") || TypeString == TEXT("FVector") ||
        TypeString == TEXT("Rotator") || TypeString == TEXT("FRotator") ||
        TypeString == TEXT("Name") || TypeString == TEXT("FName") ||
        TypeString == TEXT("Text") || TypeString == TEXT("FText"))
    {
        return reinterpret_cast<UObject*>(1); // Non-null placeholder for basic types
    }

    // Handle DataTable type explicitly
    if (TypeString == TEXT("DataTable") || TypeString == TEXT("UDataTable"))
    {
        UClass* DataTableClass = UDataTable::StaticClass();
        UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Resolved DataTable type"));
        return DataTableClass;
    }

    // Handle Class reference types (TSubclassOf)
    // Format: "Class" or "Class<ClassName>" or "TSubclassOf<ClassName>"
    if (TypeString.StartsWith(TEXT("Class")) || TypeString.StartsWith(TEXT("TSubclassOf")))
    {
        // Return a special marker - we'll handle this in AddVariableToBlueprint
        // Use UClass::StaticClass() as a marker that this is a class reference
        UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Detected Class reference type '%s'"), *TypeString);
        return UClass::StaticClass();
    }

    // Try to find as a native C++ class first
    if (UClass* FoundClass = FindFirstObject<UClass>(*TypeString, EFindFirstObjectOptions::None, ELogVerbosity::Warning, TEXT("ResolveVariableType")))
    {
        return FoundClass;
    }

    // Try to find as a struct
    if (UScriptStruct* FoundStruct = FindFirstObject<UScriptStruct>(*TypeString, EFindFirstObjectOptions::None, ELogVerbosity::Warning, TEXT("ResolveVariableType")))
    {
        return FoundStruct;
    }

    // Try loading from common paths for native types
    TArray<FString> SearchPaths = {
        FString::Printf(TEXT("/Script/Engine.%s"), *TypeString),
        FString::Printf(TEXT("/Script/CoreUObject.%s"), *TypeString)
    };

    for (const FString& SearchPath : SearchPaths)
    {
        if (UClass* LoadedClass = LoadClass<UObject>(nullptr, *SearchPath))
        {
            return LoadedClass;
        }

        if (UScriptStruct* LoadedStruct = LoadObject<UScriptStruct>(nullptr, *SearchPath))
        {
            return LoadedStruct;
        }
    }

    // Check if it's a Blueprint class name (look for BP_ prefix or try to find as Blueprint)
    // Search all loaded Blueprints for a matching name
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Try multiple search strategies for Blueprint classes
    TArray<FString> BlueprintSearchNames;
    BlueprintSearchNames.Add(TypeString);  // Exact name

    // If TypeString is a full path like /Game/Dialogue/Blueprints/BP_DialogueNPC, use it directly
    if (TypeString.StartsWith(TEXT("/")))
    {
        FString AssetPath = TypeString;

        // First, try to load as a UserDefinedStruct (for structs created via create_struct)
        // This handles paths like /Game/Inventory/Data/S_ItemDefinition
        if (UUserDefinedStruct* UserStruct = LoadObject<UUserDefinedStruct>(nullptr, *AssetPath))
        {
            UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Found UserDefinedStruct '%s' from full path"), *TypeString);
            return UserStruct;
        }

        // Try to load as a generic ScriptStruct (for other struct types)
        if (UScriptStruct* LoadedStruct = LoadObject<UScriptStruct>(nullptr, *AssetPath))
        {
            UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Found ScriptStruct '%s' from full path"), *TypeString);
            return LoadedStruct;
        }

        // Ensure it ends with the Blueprint asset name suffix if needed
        if (!AssetPath.EndsWith(TEXT("_C")))
        {
            // Try to load as Blueprint asset first
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
            if (Blueprint && Blueprint->GeneratedClass)
            {
                UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Found Blueprint '%s' from full path, using GeneratedClass"), *TypeString);
                return Blueprint->GeneratedClass;
            }
            // Try with _C suffix for the generated class
            FString GeneratedClassPath = AssetPath + TEXT("_C");
            if (UClass* LoadedClass = LoadClass<UObject>(nullptr, *GeneratedClassPath))
            {
                UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Found Blueprint GeneratedClass from path '%s'"), *GeneratedClassPath);
                return LoadedClass;
            }
        }
    }

    // Search in Asset Registry for UserDefinedStruct assets matching this name
    // This handles user-created structs like S_DialogueRow, S_InventoryItem, etc.
    {
        FARFilter StructFilter;
        StructFilter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
        StructFilter.bRecursiveClasses = true;
        StructFilter.bRecursivePaths = true;

        TArray<FAssetData> StructAssetDataList;
        AssetRegistry.GetAssets(StructFilter, StructAssetDataList);

        for (const FAssetData& AssetData : StructAssetDataList)
        {
            // Check if asset name matches our type string
            if (AssetData.AssetName.ToString() == TypeString)
            {
                // Load the UserDefinedStruct
                UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(AssetData.GetAsset());
                if (UserStruct)
                {
                    UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Found UserDefinedStruct '%s' via Asset Registry"), *TypeString);
                    return UserStruct;
                }
            }
        }
    }

    // Search in Asset Registry for Blueprint assets matching this name
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        // Check if asset name matches our type string
        if (AssetData.AssetName.ToString() == TypeString)
        {
            // Load the Blueprint and return its GeneratedClass
            UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
            if (Blueprint && Blueprint->GeneratedClass)
            {
                UE_LOG(LogTemp, Log, TEXT("ResolveVariableType: Found Blueprint '%s' via Asset Registry, using GeneratedClass"), *TypeString);
                return Blueprint->GeneratedClass;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("ResolveVariableType: Could not resolve type '%s'"), *TypeString);
    return nullptr;
}
