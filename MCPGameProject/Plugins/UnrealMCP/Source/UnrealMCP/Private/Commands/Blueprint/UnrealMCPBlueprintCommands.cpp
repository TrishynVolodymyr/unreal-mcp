#include "Commands/Blueprint/UnrealMCPBlueprintCommands.h"
#include "Commands/UnrealMCPMainDispatcher.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/TopLevelAssetPath.h"
#include "Factories/BlueprintInterfaceFactory.h"
#include "Math/Vector2D.h"
#include "Math/Color.h"
#include "Math/UnrealMathUtility.h"
#include "KismetCompiler.h"

// Using LogTemp instead of a custom log category for UE5.5 compatibility
// DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCP, Log, All);

FUnrealMCPBlueprintCommands::FUnrealMCPBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // All Blueprint commands now route through the new main dispatcher and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_blueprint"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_component_to_blueprint"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("modify_blueprint_component_properties"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_physics_properties"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("compile_blueprint"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("spawn_blueprint_actor"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleSetBlueprintProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_blueprint_property"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_static_mesh_properties"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleSetPawnProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_pawn_properties"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleCallFunctionByName(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("call_blueprint_function"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleAddInterfaceToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_interface_to_blueprint"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleCreateBlueprintInterface(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_blueprint_interface"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleListBlueprintComponents(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("list_blueprint_components"), Params);
}

// Helper function to convert string type to FEdGraphPinType
FEdGraphPinType GetTypeFromString(const FString& TypeString)
{
    FEdGraphPinType PinType;
    
    if (TypeString == TEXT("Boolean") || TypeString == TEXT("bool"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (TypeString == TEXT("Integer") || TypeString == TEXT("int"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (TypeString == TEXT("Float") || TypeString == TEXT("float"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
    }
    else if (TypeString == TEXT("String"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (TypeString == TEXT("Text"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
    }
    else if (TypeString == TEXT("Name"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
    }
    else if (TypeString == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else if (TypeString == TEXT("Vector2D"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
    }
    else if (TypeString == TEXT("Rotator"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
    }
    else if (TypeString == TEXT("Transform"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
    }
    else if (TypeString == TEXT("Color"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FColor>::Get();
    }
    else if (TypeString == TEXT("LinearColor"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
    }
    else if (TypeString == TEXT("Object") || TypeString == TEXT("UObject"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        PinType.PinSubCategoryObject = UObject::StaticClass();
    }
    else if (TypeString == TEXT("Actor") || TypeString == TEXT("AActor"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        PinType.PinSubCategoryObject = AActor::StaticClass();
    }
    else if (TypeString.EndsWith(TEXT("[]"))) // Array type
    {
        FString BaseType = TypeString.LeftChop(2);
        PinType = GetTypeFromString(BaseType);
        PinType.ContainerType = EPinContainerType::Array;
    }
    else if (TypeString.StartsWith(TEXT("Class<")) && TypeString.EndsWith(TEXT(">")))
    {
        // Handle Class<Type> references
        FString ClassType = TypeString.Mid(6, TypeString.Len() - 7); // Remove "Class<" and ">"
        PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
        
        // Try to find the class
        if (ClassType == TEXT("Actor"))
        {
            PinType.PinSubCategoryObject = AActor::StaticClass();
        }
        else if (ClassType == TEXT("Object"))
        {
            PinType.PinSubCategoryObject = UObject::StaticClass();
        }
        else
        {
            // For unknown class types, default to UObject
            PinType.PinSubCategoryObject = UObject::StaticClass();
        }
    }
    else
    {
        // Try to find it as a struct or custom type
        // First check if it's a known struct name
        UScriptStruct* FoundStruct = nullptr;
        
        // Try common struct prefixes
        if (!TypeString.StartsWith(TEXT("F")))
        {
            FString StructName = TEXT("F") + TypeString;
            FoundStruct = FindObject<UScriptStruct>(nullptr, *StructName);
        }
        
        if (!FoundStruct)
        {
            FoundStruct = FindObject<UScriptStruct>(nullptr, *TypeString);
        }
        
        if (FoundStruct)
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            PinType.PinSubCategoryObject = FoundStruct;
        }
        else
        {
            // Try to find it as a class
            UClass* FoundClass = FindObject<UClass>(nullptr, *TypeString);
            if (!FoundClass && !TypeString.StartsWith(TEXT("U")) && !TypeString.StartsWith(TEXT("A")))
            {
                // Try with U prefix for UObject classes
                FString UClassName = TEXT("U") + TypeString;
                FoundClass = FindObject<UClass>(nullptr, *UClassName);
                
                // Try with A prefix for Actor classes
                if (!FoundClass)
                {
                    FString AClassName = TEXT("A") + TypeString;
                    FoundClass = FindObject<UClass>(nullptr, *AClassName);
                }
            }
            
            if (FoundClass)
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                PinType.PinSubCategoryObject = FoundClass;
            }
            else
            {
                // Default to string for unknown types with a warning
                UE_LOG(LogTemp, Warning, TEXT("Unknown type '%s', defaulting to String"), *TypeString);
                PinType.PinCategory = UEdGraphSchema_K2::PC_String;
            }
        }
    }
    
    return PinType;
}

FProperty* CreatePropertyFromPinType(UObject* Outer, FName PropertyName, const FEdGraphPinType& PinType)
{
    FProperty* NewProperty = nullptr;
    
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
    {
        NewProperty = new FBoolProperty(Outer, PropertyName, RF_Public);
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
    {
        NewProperty = new FIntProperty(Outer, PropertyName, RF_Public);
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        if (PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
        {
            NewProperty = new FFloatProperty(Outer, PropertyName, RF_Public);
        }
        else
        {
            NewProperty = new FDoubleProperty(Outer, PropertyName, RF_Public);
        }
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        NewProperty = new FStrProperty(Outer, PropertyName, RF_Public);
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
    {
        NewProperty = new FNameProperty(Outer, PropertyName, RF_Public);
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
    {
        NewProperty = new FTextProperty(Outer, PropertyName, RF_Public);
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
    {
        UClass* ObjectClass = Cast<UClass>(PinType.PinSubCategoryObject.Get());
        if (ObjectClass)
        {
            FObjectProperty* ObjectProp = new FObjectProperty(Outer, PropertyName, RF_Public);
            ObjectProp->SetPropertyClass(ObjectClass);
            NewProperty = ObjectProp;
        }
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
    {
        UScriptStruct* StructType = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get());
        if (StructType)
        {
            FStructProperty* StructProp = new FStructProperty(Outer, PropertyName, RF_Public);
            StructProp->Struct = StructType;
            NewProperty = StructProp;
        }
    }
    
    return NewProperty;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintCommands::HandleCreateCustomBlueprintFunction(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all Blueprint commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_custom_blueprint_function"), Params);
}




