#include "Commands/StateTree/GetCurrentActiveStatesCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetCurrentActiveStatesCommand::FGetCurrentActiveStatesCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetCurrentActiveStatesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FString StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    FString ActorPath;
    ParamsObj->TryGetStringField(TEXT("actor_path"), ActorPath);

    if (StateTreePath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("state_tree_path is required"));
    }

    TArray<FString> ActiveStates;
    if (!Service.GetCurrentActiveStates(StateTreePath, ActorPath, ActiveStates))
    {
        return CreateErrorResponse(TEXT("Failed to get current active states"));
    }

    TArray<TSharedPtr<FJsonValue>> StatesArray;
    for (const FString& StateName : ActiveStates)
    {
        StatesArray.Add(MakeShared<FJsonValueString>(StateName));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetArrayField(TEXT("active_states"), StatesArray);
    ResponseObj->SetNumberField(TEXT("count"), ActiveStates.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetCurrentActiveStatesCommand::GetCommandName() const
{
    return TEXT("get_current_active_states");
}

bool FGetCurrentActiveStatesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path"));
}

FString FGetCurrentActiveStatesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
