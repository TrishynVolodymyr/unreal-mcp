#include "Commands/BlueprintAction/SearchBlueprintActionsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FSearchBlueprintActionsCommand::FSearchBlueprintActionsCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService)
    : BlueprintActionService(InBlueprintActionService)
{
}

FString FSearchBlueprintActionsCommand::Execute(const FString& Parameters)
{
    if (!BlueprintActionService.IsValid())
    {
        return CreateErrorResponse(TEXT("Blueprint action service not available"));
    }
    
    FString SearchQuery, Category, BlueprintName;
    int32 MaxResults;
    FString ParseError;
    
    if (!ParseParameters(Parameters, SearchQuery, Category, MaxResults, BlueprintName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    return BlueprintActionService->SearchBlueprintActions(SearchQuery, Category, MaxResults, BlueprintName);
}

FString FSearchBlueprintActionsCommand::GetCommandName() const
{
    return TEXT("search_blueprint_actions");
}

bool FSearchBlueprintActionsCommand::ValidateParams(const FString& Parameters) const
{
    FString SearchQuery, Category, BlueprintName;
    int32 MaxResults;
    FString ParseError;
    return ParseParameters(Parameters, SearchQuery, Category, MaxResults, BlueprintName, ParseError);
}

bool FSearchBlueprintActionsCommand::ParseParameters(
    const FString& Parameters,
    FString& OutSearchQuery,
    FString& OutCategory,
    int32& OutMaxResults,
    FString& OutBlueprintName,
    FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required parameter
    OutSearchQuery = JsonObject->GetStringField(TEXT("search_query"));
    if (OutSearchQuery.IsEmpty())
    {
        OutError = TEXT("search_query is required and cannot be empty");
        return false;
    }
    
    // Parse optional parameters
    OutCategory = JsonObject->GetStringField(TEXT("category"));
    OutBlueprintName = JsonObject->GetStringField(TEXT("blueprint_name"));
    
    // Parse max_results with default value
    if (JsonObject->HasField(TEXT("max_results")))
    {
        OutMaxResults = JsonObject->GetIntegerField(TEXT("max_results"));
    }
    else
    {
        OutMaxResults = 50; // Default value
    }
    
    // Validate max_results
    if (OutMaxResults <= 0 || OutMaxResults > 1000)
    {
        OutError = TEXT("max_results must be between 1 and 1000");
        return false;
    }
    
    return true;
}

FString FSearchBlueprintActionsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}


