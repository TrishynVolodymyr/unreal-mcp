#include "Commands/Sound/AddMetaSoundOutputCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FAddMetaSoundOutputCommand::FAddMetaSoundOutputCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FAddMetaSoundOutputCommand::GetCommandName() const
{
    return TEXT("add_metasound_output");
}

bool FAddMetaSoundOutputCommand::ValidateParams(const FString& Parameters) const
{
    FString MetaSoundPath, OutputName, DataType, Error;
    return ParseParameters(Parameters, MetaSoundPath, OutputName, DataType, Error);
}

FString FAddMetaSoundOutputCommand::Execute(const FString& Parameters)
{
    FString MetaSoundPath, OutputName, DataType, Error;
    if (!ParseParameters(Parameters, MetaSoundPath, OutputName, DataType, Error))
    {
        return CreateErrorResponse(Error);
    }

    FMetaSoundOutputParams Params;
    Params.MetaSoundPath = MetaSoundPath;
    Params.OutputName = OutputName;
    Params.DataType = DataType;

    FString OutputNodeId;
    if (!SoundService.AddMetaSoundOutput(Params, OutputNodeId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(OutputNodeId, OutputName, DataType);
}

bool FAddMetaSoundOutputCommand::ParseParameters(const FString& JsonString, FString& OutMetaSoundPath,
    FString& OutOutputName, FString& OutDataType, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("output_name"), OutOutputName) || OutOutputName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: output_name");
        return false;
    }

    // Optional parameters
    if (!JsonObject->TryGetStringField(TEXT("data_type"), OutDataType))
    {
        OutDataType = TEXT("Audio");
    }

    return true;
}

FString FAddMetaSoundOutputCommand::CreateSuccessResponse(const FString& OutputNodeId, const FString& OutputName, const FString& DataType) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("output_node_id"), OutputNodeId);
    Response->SetStringField(TEXT("output_name"), OutputName);
    Response->SetStringField(TEXT("data_type"), DataType);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Added output '%s' (type: %s, ID: %s)"),
        *OutputName, *DataType, *OutputNodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddMetaSoundOutputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
