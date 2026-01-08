#include "Commands/Blueprint/DeleteBlueprintVariableCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"

FDeleteBlueprintVariableCommand::FDeleteBlueprintVariableCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FDeleteBlueprintVariableCommand::Execute(const FString& Parameters)
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

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Check if variable exists
    FName VarFName(*VariableName);
    bool bVariableExists = false;
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == VarFName)
        {
            bVariableExists = true;
            break;
        }
    }

    if (!bVariableExists)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Variable '%s' not found in Blueprint '%s'"),
            *VariableName, *BlueprintName));
    }

    // Remove the variable using BlueprintEditorUtils
    FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, VarFName);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("DeleteBlueprintVariable: Successfully deleted variable '%s' from Blueprint '%s'"),
        *VariableName, *BlueprintName);

    return CreateSuccessResponse(BlueprintName, VariableName);
}

FString FDeleteBlueprintVariableCommand::GetCommandName() const
{
    return TEXT("delete_blueprint_variable");
}

bool FDeleteBlueprintVariableCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("blueprint_name")) &&
           JsonObject->HasField(TEXT("variable_name"));
}

FString FDeleteBlueprintVariableCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& VariableName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("variable_name"), VariableName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Variable '%s' deleted from Blueprint '%s'"),
        *VariableName, *BlueprintName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FDeleteBlueprintVariableCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
