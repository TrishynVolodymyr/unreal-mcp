#include "Commands/Sound/AddMetaSoundInputCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FAddMetaSoundInputCommand::FAddMetaSoundInputCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FAddMetaSoundInputCommand::GetCommandName() const
{
    return TEXT("add_metasound_input");
}

bool FAddMetaSoundInputCommand::ValidateParams(const FString& Parameters) const
{
    FString MetaSoundPath, InputName, DataType, DefaultValue, Error;
    return ParseParameters(Parameters, MetaSoundPath, InputName, DataType, DefaultValue, Error);
}

FString FAddMetaSoundInputCommand::Execute(const FString& Parameters)
{
    FString MetaSoundPath, InputName, DataType, DefaultValue, Error;
    if (!ParseParameters(Parameters, MetaSoundPath, InputName, DataType, DefaultValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    FMetaSoundInputParams Params;
    Params.MetaSoundPath = MetaSoundPath;
    Params.InputName = InputName;
    Params.DataType = DataType;
    Params.DefaultValue = DefaultValue;

    FString InputNodeId;
    if (!SoundService.AddMetaSoundInput(Params, InputNodeId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(InputNodeId, InputName, DataType);
}

bool FAddMetaSoundInputCommand::ParseParameters(const FString& JsonString, FString& OutMetaSoundPath,
    FString& OutInputName, FString& OutDataType, FString& OutDefaultValue, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("metasound_path"), OutMetaSoundPath) || OutMetaSoundPath.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: metasound_path");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("input_name"), OutInputName) || OutInputName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: input_name");
        return false;
    }

    // Optional parameters
    if (!JsonObject->TryGetStringField(TEXT("data_type"), OutDataType))
    {
        OutDataType = TEXT("Float");
    }

    JsonObject->TryGetStringField(TEXT("default_value"), OutDefaultValue);

    return true;
}

FString FAddMetaSoundInputCommand::CreateSuccessResponse(const FString& InputNodeId, const FString& InputName, const FString& DataType) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("input_node_id"), InputNodeId);
    Response->SetStringField(TEXT("input_name"), InputName);
    Response->SetStringField(TEXT("data_type"), DataType);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Added input '%s' (type: %s, ID: %s)"),
        *InputName, *DataType, *InputNodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddMetaSoundInputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
