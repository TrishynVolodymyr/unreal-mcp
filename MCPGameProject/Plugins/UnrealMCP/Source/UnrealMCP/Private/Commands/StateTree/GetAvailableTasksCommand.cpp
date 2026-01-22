#include "Commands/StateTree/GetAvailableTasksCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetAvailableTasksCommand::FGetAvailableTasksCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetAvailableTasksCommand::Execute(const FString& Parameters)
{
    TArray<TPair<FString, FString>> Tasks;
    if (!Service.GetAvailableTaskTypes(Tasks))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve available task types"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);

    TArray<TSharedPtr<FJsonValue>> TasksArray;
    for (const auto& Task : Tasks)
    {
        TSharedPtr<FJsonObject> TaskObj = MakeShared<FJsonObject>();
        TaskObj->SetStringField(TEXT("path"), Task.Key);
        TaskObj->SetStringField(TEXT("name"), Task.Value);
        TasksArray.Add(MakeShared<FJsonValueObject>(TaskObj));
    }
    ResponseObj->SetArrayField(TEXT("tasks"), TasksArray);
    ResponseObj->SetNumberField(TEXT("count"), Tasks.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetAvailableTasksCommand::GetCommandName() const
{
    return TEXT("get_available_tasks");
}

bool FGetAvailableTasksCommand::ValidateParams(const FString& Parameters) const
{
    // No required parameters for this command
    return true;
}

FString FGetAvailableTasksCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
