#include "Commands/StateTree/AddTaskToStateCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddTaskToStateCommand::FAddTaskToStateCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddTaskToStateCommand::Execute(const FString& Parameters)
{
    FAddTaskParams Params;
    FString ParseError;

    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString AddError;
    if (!Service.AddTaskToState(Params, AddError))
    {
        return CreateErrorResponse(AddError.IsEmpty() ? TEXT("Failed to add task") : AddError);
    }

    return CreateSuccessResponse(Params.StateName, Params.TaskStructPath);
}

FString FAddTaskToStateCommand::GetCommandName() const
{
    return TEXT("add_task_to_state");
}

bool FAddTaskToStateCommand::ValidateParams(const FString& Parameters) const
{
    FAddTaskParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FAddTaskToStateCommand::ParseParameters(const FString& JsonString, FAddTaskParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("state_tree_path"), OutParams.StateTreePath))
    {
        OutError = TEXT("Missing required 'state_tree_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("state_name"), OutParams.StateName))
    {
        OutError = TEXT("Missing required 'state_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("task_struct_path"), OutParams.TaskStructPath))
    {
        OutError = TEXT("Missing required 'task_struct_path' parameter");
        return false;
    }

    JsonObject->TryGetStringField(TEXT("task_name"), OutParams.TaskName);

    // Parse task properties if provided
    const TSharedPtr<FJsonObject>* TaskPropertiesObj;
    if (JsonObject->TryGetObjectField(TEXT("task_properties"), TaskPropertiesObj))
    {
        OutParams.TaskProperties = *TaskPropertiesObj;
    }

    return true;
}

FString FAddTaskToStateCommand::CreateSuccessResponse(const FString& StateName, const FString& TaskType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetStringField(TEXT("task_type"), TaskType);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Task added to state '%s' successfully"), *StateName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddTaskToStateCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
