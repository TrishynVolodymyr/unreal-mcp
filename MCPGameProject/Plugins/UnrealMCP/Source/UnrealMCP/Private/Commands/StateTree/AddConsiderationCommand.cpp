#include "Commands/StateTree/AddConsiderationCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddConsiderationCommand::FAddConsiderationCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddConsiderationCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FAddConsiderationParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    Params.StateName = ParamsObj->GetStringField(TEXT("state_name"));
    Params.ConsiderationStructPath = ParamsObj->GetStringField(TEXT("consideration_struct_path"));
    ParamsObj->TryGetNumberField(TEXT("weight"), Params.Weight);

    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (ParamsObj->TryGetObjectField(TEXT("consideration_properties"), PropertiesPtr))
    {
        Params.ConsiderationProperties = *PropertiesPtr;
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.AddConsideration(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Added consideration '%s' to state '%s' with weight %.2f"),
        *Params.ConsiderationStructPath, *Params.StateName, Params.Weight));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddConsiderationCommand::GetCommandName() const
{
    return TEXT("add_consideration");
}

bool FAddConsiderationCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("state_name")) &&
           ParamsObj->HasField(TEXT("consideration_struct_path"));
}

FString FAddConsiderationCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
