#include "Commands/Niagara/SearchNiagaraModulesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSearchNiagaraModulesCommand::FSearchNiagaraModulesCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSearchNiagaraModulesCommand::Execute(const FString& Parameters)
{
    FString SearchQuery;
    FString StageFilter;
    int32 MaxResults = 50;
    FString Error;

    if (!ParseParameters(Parameters, SearchQuery, StageFilter, MaxResults, Error))
    {
        return CreateErrorResponse(Error);
    }

    TArray<TSharedPtr<FJsonObject>> Modules;
    if (!NiagaraService.SearchModules(SearchQuery, StageFilter, MaxResults, Modules))
    {
        return CreateErrorResponse(TEXT("Failed to search modules"));
    }

    return CreateSuccessResponse(Modules);
}

FString FSearchNiagaraModulesCommand::GetCommandName() const
{
    return TEXT("search_niagara_modules");
}

bool FSearchNiagaraModulesCommand::ValidateParams(const FString& Parameters) const
{
    FString SearchQuery, StageFilter, Error;
    int32 MaxResults;
    return ParseParameters(Parameters, SearchQuery, StageFilter, MaxResults, Error);
}

bool FSearchNiagaraModulesCommand::ParseParameters(const FString& JsonString, FString& OutSearchQuery, FString& OutStageFilter, int32& OutMaxResults, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // All parameters are optional
    JsonObject->TryGetStringField(TEXT("search_query"), OutSearchQuery);
    JsonObject->TryGetStringField(TEXT("stage_filter"), OutStageFilter);

    if (!JsonObject->TryGetNumberField(TEXT("max_results"), OutMaxResults))
    {
        OutMaxResults = 50;
    }

    return true;
}

FString FSearchNiagaraModulesCommand::CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Modules) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);

    TArray<TSharedPtr<FJsonValue>> ModulesArray;
    for (const TSharedPtr<FJsonObject>& Module : Modules)
    {
        ModulesArray.Add(MakeShared<FJsonValueObject>(Module));
    }
    ResponseObj->SetArrayField(TEXT("modules"), ModulesArray);
    ResponseObj->SetNumberField(TEXT("count"), Modules.Num());
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d modules"), Modules.Num()));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSearchNiagaraModulesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
