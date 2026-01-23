#include "Commands/StateTree/AddGlobalTaskCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddGlobalTaskCommand::FAddGlobalTaskCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddGlobalTaskCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FAddGlobalTaskParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    Params.TaskStructPath = ParamsObj->GetStringField(TEXT("task_struct_path"));
    ParamsObj->TryGetStringField(TEXT("task_name"), Params.TaskName);

    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (ParamsObj->TryGetObjectField(TEXT("task_properties"), PropertiesPtr))
    {
        Params.TaskProperties = *PropertiesPtr;
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.AddGlobalTask(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Added global task '%s'"), *Params.TaskStructPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddGlobalTaskCommand::GetCommandName() const
{
    return TEXT("add_global_task");
}

bool FAddGlobalTaskCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("task_struct_path"));
}

FString FAddGlobalTaskCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
