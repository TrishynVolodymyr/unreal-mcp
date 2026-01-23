#include "Commands/StateTree/QueryStatesByTagCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FQueryStatesByTagCommand::FQueryStatesByTagCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FQueryStatesByTagCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FQueryStatesByTagParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    Params.GameplayTag = ParamsObj->GetStringField(TEXT("gameplay_tag"));
    ParamsObj->TryGetBoolField(TEXT("exact_match"), Params.bExactMatch);

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    TArray<FString> States;
    if (!Service.QueryStatesByTag(Params, States))
    {
        return CreateErrorResponse(TEXT("Failed to query states by tag"));
    }

    TArray<TSharedPtr<FJsonValue>> StatesArray;
    for (const FString& StateName : States)
    {
        StatesArray.Add(MakeShared<FJsonValueString>(StateName));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetArrayField(TEXT("states"), StatesArray);
    ResponseObj->SetNumberField(TEXT("count"), States.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FQueryStatesByTagCommand::GetCommandName() const
{
    return TEXT("query_states_by_tag");
}

bool FQueryStatesByTagCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("gameplay_tag"));
}

FString FQueryStatesByTagCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
