#include "Commands/BlueprintNode/GetBlueprintGraphsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FGetBlueprintGraphsCommand::FGetBlueprintGraphsCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FGetBlueprintGraphsCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString ParseError;
    
    // Parse parameters with proper validation
    if (!ParseParameters(Parameters, BlueprintName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the Blueprint using common utilities - Service Layer Pattern
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Delegate to service layer for business logic
    TArray<FString> GraphNames;
    if (BlueprintNodeService.GetBlueprintGraphs(Blueprint, GraphNames))
    {
        return CreateSuccessResponse(GraphNames);
    }
    else
    {
        return CreateErrorResponse(TEXT("Failed to get Blueprint graphs"));
    }
}

FString FGetBlueprintGraphsCommand::GetCommandName() const
{
    return TEXT("get_blueprint_graphs");
}

bool FGetBlueprintGraphsCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString ParseError;
    
    // Delegate to parameter parsing for validation
    return ParseParameters(Parameters, BlueprintName, ParseError);
}

bool FGetBlueprintGraphsCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Validate blueprint name is not empty
    if (OutBlueprintName.IsEmpty())
    {
        OutError = TEXT("Blueprint name cannot be empty");
        return false;
    }
    
    return true;
}

FString FGetBlueprintGraphsCommand::CreateSuccessResponse(const TArray<FString>& GraphNames) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    // Add graph names array
    TArray<TSharedPtr<FJsonValue>> GraphNamesArray;
    for (const FString& GraphName : GraphNames)
    {
        GraphNamesArray.Add(MakeShared<FJsonValueString>(GraphName));
    }
    ResponseObj->SetArrayField(TEXT("graph_names"), GraphNamesArray);
    ResponseObj->SetNumberField(TEXT("graph_count"), GraphNames.Num());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetBlueprintGraphsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}