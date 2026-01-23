#include "Commands/StateTree/GetAvailableConditionsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetAvailableConditionsCommand::FGetAvailableConditionsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetAvailableConditionsCommand::Execute(const FString& Parameters)
{
    TArray<TPair<FString, FString>> Conditions;
    if (!Service.GetAvailableConditionTypes(Conditions))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve available condition types"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);

    TArray<TSharedPtr<FJsonValue>> ConditionsArray;
    for (const auto& Condition : Conditions)
    {
        TSharedPtr<FJsonObject> ConditionObj = MakeShared<FJsonObject>();
        ConditionObj->SetStringField(TEXT("path"), Condition.Key);
        ConditionObj->SetStringField(TEXT("name"), Condition.Value);
        ConditionsArray.Add(MakeShared<FJsonValueObject>(ConditionObj));
    }
    ResponseObj->SetArrayField(TEXT("conditions"), ConditionsArray);
    ResponseObj->SetNumberField(TEXT("count"), Conditions.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetAvailableConditionsCommand::GetCommandName() const
{
    return TEXT("get_available_conditions");
}

bool FGetAvailableConditionsCommand::ValidateParams(const FString& Parameters) const
{
    // No required parameters for this command
    return true;
}

FString FGetAvailableConditionsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
