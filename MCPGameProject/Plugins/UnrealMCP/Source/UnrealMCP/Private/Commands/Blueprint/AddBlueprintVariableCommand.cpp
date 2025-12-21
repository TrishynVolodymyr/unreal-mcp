#include "Commands/Blueprint/AddBlueprintVariableCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Services/AssetDiscoveryService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/DataTable.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"

FAddBlueprintVariableCommand::FAddBlueprintVariableCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FAddBlueprintVariableCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Get required parameters
    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!JsonObject->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!JsonObject->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    // Get optional parameters
    bool IsExposed = false;
    if (JsonObject->HasField(TEXT("is_exposed")))
    {
        IsExposed = JsonObject->GetBoolField(TEXT("is_exposed"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create variable based on type
    FEdGraphPinType PinType;
    bool bTypeResolved = false;

    auto SetPinTypeForCategory = [&](auto Category, UObject* SubCategoryObject = nullptr) {
        if constexpr (std::is_same_v<std::decay_t<decltype(Category)>, FName>) {
            PinType.PinCategory = Category;
        } else {
            PinType.PinCategory = FName(Category);
        }
        PinType.PinSubCategoryObject = SubCategoryObject;
        bTypeResolved = true;
    };

    FString TypeStr = VariableType;
    if (TypeStr.StartsWith(TEXT("/")))
    {
        TypeStr.RemoveFromStart(TEXT("/"));
    }
    TypeStr.TrimStartAndEndInline();

    // Handle array containers
    if (TypeStr.EndsWith(TEXT("[]"))) 
    {
        // Array type
        FString InnerType = TypeStr.LeftChop(2);
        InnerType.TrimStartAndEndInline();

        // Recursively resolve inner type
        FEdGraphPinType InnerPinType;
        bool bInnerResolved = false;

        // Built-in types
        if (InnerType.Equals(TEXT("Float"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
            InnerPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) || InnerType.Equals(TEXT("Int"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("String"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_String;
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Name"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Text"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            InnerPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            InnerPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            InnerPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
            bInnerResolved = true;
        } else if (InnerType.Equals(TEXT("Color"), ESearchCase::IgnoreCase)) {
            InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            InnerPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
            bInnerResolved = true;
        } else {
            // Try enum using AssetDiscoveryService (supports UUserDefinedEnum)
            UEnum* FoundEnum = FAssetDiscoveryService::Get().FindEnumType(InnerType);
            if (FoundEnum) {
                // User-defined enums use PC_Byte with the enum as subcategory object
                InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
                InnerPinType.PinSubCategoryObject = FoundEnum;
                bInnerResolved = true;
                UE_LOG(LogTemp, Display, TEXT("Successfully resolved array enum type: %s -> %s"), *InnerType, *FoundEnum->GetName());
            } else {
                // Try struct using AssetDiscoveryService (supports UUserDefinedStruct)
                UScriptStruct* FoundStruct = FAssetDiscoveryService::Get().FindStructType(InnerType);
                if (FoundStruct) {
                    InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    InnerPinType.PinSubCategoryObject = FoundStruct;
                    bInnerResolved = true;
                    UE_LOG(LogTemp, Display, TEXT("Successfully resolved array struct type: %s -> %s"), *InnerType, *FoundStruct->GetName());
                } else {
                    // Try object/class resolution
                    UClass* FoundClass = FAssetDiscoveryService::Get().ResolveObjectClass(InnerType);

                    if (!FoundClass) {
                        // Try widget class finding for widget blueprints
                        FoundClass = FAssetDiscoveryService::Get().FindWidgetClass(InnerType);
                    }

                    if (FoundClass) {
                        InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                        InnerPinType.PinSubCategoryObject = FoundClass;
                        bInnerResolved = true;
                        UE_LOG(LogTemp, Display, TEXT("Successfully resolved array object type: %s -> %s"), *InnerType, *FoundClass->GetName());
                    } else {
                        UE_LOG(LogTemp, Warning, TEXT("Could not resolve array inner type: %s"), *InnerType);
                    }
                }
            }
        }

        if (bInnerResolved) {
            PinType = InnerPinType;
            PinType.ContainerType = EPinContainerType::Array;
            bTypeResolved = true;
        }
    } 
    else 
    {
        // Built-in types
        if (TypeStr.Equals(TEXT("Float"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Real);
            PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        } else if (TypeStr.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Boolean);
        } else if (TypeStr.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) || TypeStr.Equals(TEXT("Int"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Int);
        } else if (TypeStr.Equals(TEXT("String"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_String);
        } else if (TypeStr.Equals(TEXT("Name"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Name);
        } else if (TypeStr.Equals(TEXT("Text"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Text);
        } else if (TypeStr.Equals(TEXT("Vector"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
        } else if (TypeStr.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FRotator>::Get());
        } else if (TypeStr.Equals(TEXT("Transform"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FTransform>::Get());
        } else if (TypeStr.Equals(TEXT("Color"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FLinearColor>::Get());
        } else if (TypeStr.StartsWith(TEXT("Class<")) && TypeStr.EndsWith(TEXT(">"))) {
            // Handle class reference types like "Class<UserWidget>"
            FString InnerType = TypeStr.Mid(6, TypeStr.Len() - 7); // Remove "Class<" and ">"
            InnerType.TrimStartAndEndInline();

            UClass* TargetClass = FAssetDiscoveryService::Get().ResolveObjectClass(InnerType);
            
            if (!TargetClass) {
                // Try widget class finding for widget blueprints
                TargetClass = FAssetDiscoveryService::Get().FindWidgetClass(InnerType);
            }

            if (TargetClass) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Class, TargetClass);
            }
        } else {
            // Try enum first using the asset discovery service
            UEnum* FoundEnum = FAssetDiscoveryService::Get().FindEnumType(TypeStr);
            if (FoundEnum) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Byte, FoundEnum);
                UE_LOG(LogTemp, Display, TEXT("Successfully resolved enum type: %s -> %s"), *TypeStr, *FoundEnum->GetName());
            } else {
                // Try struct using the asset discovery service
                UScriptStruct* FoundStruct = FAssetDiscoveryService::Get().FindStructType(TypeStr);
                if (FoundStruct) {
                    SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, FoundStruct);
                    UE_LOG(LogTemp, Display, TEXT("Successfully resolved struct type: %s -> %s"), *TypeStr, *FoundStruct->GetName());
                } else {
                    // Try object/class resolution using the asset discovery service
                    UClass* FoundClass = FAssetDiscoveryService::Get().ResolveObjectClass(TypeStr);

                    if (!FoundClass) {
                        // Try widget class finding for widget blueprints
                        FoundClass = FAssetDiscoveryService::Get().FindWidgetClass(TypeStr);
                    }

                    if (FoundClass) {
                        SetPinTypeForCategory(UEdGraphSchema_K2::PC_Object, FoundClass);
                        UE_LOG(LogTemp, Display, TEXT("Successfully resolved object type: %s -> %s"), *TypeStr, *FoundClass->GetName());
                    } else {
                        UE_LOG(LogTemp, Warning, TEXT("Could not resolve object type: %s"), *TypeStr);
                    }
                }
            }
        }
    }

    if (!bTypeResolved) {
        return CreateErrorResponse(FString::Printf(TEXT("Could not resolve variable type: %s"), *VariableType));
    }

    // Create the variable
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Set variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // Set exposure in editor
        if (IsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit;
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    return CreateSuccessResponse(BlueprintName, VariableName, VariableType, IsExposed);
}

FString FAddBlueprintVariableCommand::GetCommandName() const
{
    return TEXT("add_blueprint_variable");
}

bool FAddBlueprintVariableCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    return JsonObject->HasField(TEXT("blueprint_name")) && 
           JsonObject->HasField(TEXT("variable_name")) && 
           JsonObject->HasField(TEXT("variable_type"));
}

FString FAddBlueprintVariableCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& VariableName, 
                                                           const FString& VariableType, bool bIsExposed) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("variable_name"), VariableName);
    ResponseObj->SetStringField(TEXT("variable_type"), VariableType);
    ResponseObj->SetBoolField(TEXT("is_exposed"), bIsExposed);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddBlueprintVariableCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



