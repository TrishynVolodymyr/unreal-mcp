#include "Commands/AddBlueprintVariableCommand.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/DataTable.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"

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
            // Try struct
            UScriptStruct* FoundStruct = FUnrealMCPCommonUtils::FindStructType(InnerType);
            if (FoundStruct) {
                InnerPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                InnerPinType.PinSubCategoryObject = FoundStruct;
                bInnerResolved = true;
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

            UClass* TargetClass = nullptr;
            if (InnerType.Equals(TEXT("UserWidget"), ESearchCase::IgnoreCase)) {
                TargetClass = UUserWidget::StaticClass();
            } else if (InnerType.Equals(TEXT("Actor"), ESearchCase::IgnoreCase)) {
                TargetClass = AActor::StaticClass();
            } else if (InnerType.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase)) {
                TargetClass = APawn::StaticClass();
            } else {
                UClass* FoundClass = FUnrealMCPCommonUtils::FindWidgetClass(InnerType);
                if (FoundClass) {
                    TargetClass = FoundClass;
                }
            }

            if (TargetClass) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Class, TargetClass);
            }
        } else {
            // Try struct
            UScriptStruct* FoundStruct = FUnrealMCPCommonUtils::FindStructType(TypeStr);
            if (FoundStruct) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, FoundStruct);
            } else {
                // Try object/class
                UClass* FoundClass = nullptr;
                
                // Try direct loading
                FoundClass = LoadObject<UClass>(nullptr, *TypeStr);
                
                if (!FoundClass) {
                    // Try with /Game/ prefix
                    FString GamePath = FString::Printf(TEXT("/Game/%s"), *TypeStr);
                    FoundClass = LoadObject<UClass>(nullptr, *GamePath);
                }

                if (FoundClass) {
                    SetPinTypeForCategory(UEdGraphSchema_K2::PC_Object, FoundClass);
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