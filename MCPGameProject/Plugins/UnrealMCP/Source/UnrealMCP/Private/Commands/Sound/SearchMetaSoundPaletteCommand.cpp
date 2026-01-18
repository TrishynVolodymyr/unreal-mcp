#include "Commands/Sound/SearchMetaSoundPaletteCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FSearchMetaSoundPaletteCommand::FSearchMetaSoundPaletteCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FSearchMetaSoundPaletteCommand::GetCommandName() const
{
    return TEXT("search_metasound_palette");
}

bool FSearchMetaSoundPaletteCommand::ValidateParams(const FString& Parameters) const
{
    FString SearchQuery, Error;
    int32 MaxResults;
    return ParseParameters(Parameters, SearchQuery, MaxResults, Error);
}

FString FSearchMetaSoundPaletteCommand::Execute(const FString& Parameters)
{
    FString SearchQuery, Error;
    int32 MaxResults;
    if (!ParseParameters(Parameters, SearchQuery, MaxResults, Error))
    {
        return CreateErrorResponse(Error);
    }

    TArray<TSharedPtr<FJsonObject>> Results;
    if (!SoundService.SearchMetaSoundPalette(SearchQuery, MaxResults, Results, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Results);
}

bool FSearchMetaSoundPaletteCommand::ParseParameters(const FString& JsonString, FString& OutSearchQuery, int32& OutMaxResults, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    // Search query is optional (empty = list all)
    if (!JsonObject->TryGetStringField(TEXT("search_query"), OutSearchQuery))
    {
        OutSearchQuery = TEXT("");
    }

    // Max results is optional (default 50)
    double TempMaxResults;
    OutMaxResults = JsonObject->TryGetNumberField(TEXT("max_results"), TempMaxResults) ? static_cast<int32>(TempMaxResults) : 50;

    return true;
}

FString FSearchMetaSoundPaletteCommand::CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Results) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetNumberField(TEXT("count"), Results.Num());

    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (const TSharedPtr<FJsonObject>& Result : Results)
    {
        ResultsArray.Add(MakeShared<FJsonValueObject>(Result));
    }
    Response->SetArrayField(TEXT("results"), ResultsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FSearchMetaSoundPaletteCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
