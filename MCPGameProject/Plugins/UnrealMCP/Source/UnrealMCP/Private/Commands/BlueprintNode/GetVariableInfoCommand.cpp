#include "Commands/BlueprintNode/GetVariableInfoCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FGetVariableInfoCommand::FGetVariableInfoCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FGetVariableInfoCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, VariableName;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, VariableName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    FString VariableType;
    TSharedPtr<FJsonObject> AdditionalInfo;
    if (BlueprintNodeService.GetVariableInfo(Blueprint, VariableName, VariableType, AdditionalInfo))
    {
        return CreateSuccessResponse(VariableType, AdditionalInfo);
    }
    else
    {
        return CreateErrorResponse(FString::Printf(TEXT("Variable '%s' not found in Blueprint"), *VariableName));
    }
}

FString FGetVariableInfoCommand::GetCommandName() const
{
    return TEXT("get_variable_info");
}

bool FGetVariableInfoCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, VariableName;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, VariableName, ParseError);
}

bool FGetVariableInfoCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutVariableName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("variable_name"), OutVariableName))
    {
        OutError = TEXT("Missing required 'variable_name' parameter");
        return false;
    }
    
    return true;
}

FString FGetVariableInfoCommand::CreateSuccessResponse(const FString& VariableType, const TSharedPtr<FJsonObject>& AdditionalInfo) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("variable_type"), VariableType);
    
    if (AdditionalInfo.IsValid())
    {
        ResponseObj->SetObjectField(TEXT("additional_info"), AdditionalInfo);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetVariableInfoCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
