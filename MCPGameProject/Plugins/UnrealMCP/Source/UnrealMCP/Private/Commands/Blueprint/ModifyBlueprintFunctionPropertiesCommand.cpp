#include "Commands/Blueprint/ModifyBlueprintFunctionPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"

FModifyBlueprintFunctionPropertiesCommand::FModifyBlueprintFunctionPropertiesCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FModifyBlueprintFunctionPropertiesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!JsonObject->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }

    // Find the function graph
    UEdGraph* FuncGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            FuncGraph = Graph;
            break;
        }
    }

    if (!FuncGraph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Function '%s' not found in Blueprint '%s'"), *FunctionName, *BlueprintName));
    }

    // Find the function entry node
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        EntryNode = Cast<UK2Node_FunctionEntry>(Node);
        if (EntryNode)
        {
            break;
        }
    }

    if (!EntryNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Function entry node not found for '%s'"), *FunctionName));
    }

    // Track what was modified
    TArray<FString> ModifiedProperties;

    // Get current flags
    uint32 FunctionFlags = EntryNode->GetFunctionFlags();

    // Modify is_pure
    bool bIsPure;
    if (JsonObject->TryGetBoolField(TEXT("is_pure"), bIsPure))
    {
        if (bIsPure)
        {
            FunctionFlags |= FUNC_BlueprintPure;
        }
        else
        {
            FunctionFlags &= ~FUNC_BlueprintPure;
        }
        ModifiedProperties.Add(TEXT("is_pure"));
    }

    // Modify is_const
    bool bIsConst;
    if (JsonObject->TryGetBoolField(TEXT("is_const"), bIsConst))
    {
        if (bIsConst)
        {
            FunctionFlags |= FUNC_Const;
        }
        else
        {
            FunctionFlags &= ~FUNC_Const;
        }
        ModifiedProperties.Add(TEXT("is_const"));
    }

    // Modify access_specifier
    FString AccessSpecifier;
    if (JsonObject->TryGetStringField(TEXT("access_specifier"), AccessSpecifier))
    {
        // Clear existing access flags
        FunctionFlags &= ~(FUNC_Public | FUNC_Protected | FUNC_Private);

        if (AccessSpecifier.Equals(TEXT("Public"), ESearchCase::IgnoreCase))
        {
            FunctionFlags |= FUNC_Public;
        }
        else if (AccessSpecifier.Equals(TEXT("Protected"), ESearchCase::IgnoreCase))
        {
            FunctionFlags |= FUNC_Protected;
        }
        else if (AccessSpecifier.Equals(TEXT("Private"), ESearchCase::IgnoreCase))
        {
            FunctionFlags |= FUNC_Private;
        }
        else
        {
            return CreateErrorResponse(FString::Printf(TEXT("Invalid access_specifier '%s'. Must be 'Public', 'Protected', or 'Private'"), *AccessSpecifier));
        }
        ModifiedProperties.Add(TEXT("access_specifier"));
    }

    // Apply the modified flags
    // We need to clear all controllable flags first, then set the new ones
    EntryNode->ClearExtraFlags(FUNC_BlueprintPure | FUNC_Const | FUNC_Public | FUNC_Protected | FUNC_Private);
    EntryNode->SetExtraFlags(FunctionFlags);

    // Modify category
    FString Category;
    if (JsonObject->TryGetStringField(TEXT("category"), Category))
    {
        if (!Category.IsEmpty())
        {
            EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_FunctionCategory, Category);
        }
        ModifiedProperties.Add(TEXT("category"));
    }

    if (ModifiedProperties.Num() == 0)
    {
        return CreateErrorResponse(TEXT("No properties specified to modify. Provide at least one of: is_pure, is_const, access_specifier, category"));
    }

    // Reconstruct node to apply changes
    EntryNode->ReconstructNode();

    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    // Build response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("function_name"), FunctionName);

    TArray<TSharedPtr<FJsonValue>> ModifiedArray;
    for (const FString& Prop : ModifiedProperties)
    {
        ModifiedArray.Add(MakeShared<FJsonValueString>(Prop));
    }
    ResponseObj->SetArrayField(TEXT("modified_properties"), ModifiedArray);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully modified %d properties on function '%s'"), ModifiedProperties.Num(), *FunctionName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FModifyBlueprintFunctionPropertiesCommand::GetCommandName() const
{
    return TEXT("modify_blueprint_function_properties");
}

bool FModifyBlueprintFunctionPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString BlueprintName, FunctionName;
    return JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName) &&
           JsonObject->TryGetStringField(TEXT("function_name"), FunctionName);
}

FString FModifyBlueprintFunctionPropertiesCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& FunctionName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("function_name"), FunctionName);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FModifyBlueprintFunctionPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
