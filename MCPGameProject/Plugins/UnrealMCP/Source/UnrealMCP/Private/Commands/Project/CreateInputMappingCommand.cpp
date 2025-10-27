#include "Commands/Project/CreateInputMappingCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateInputMappingCommand::FCreateInputMappingCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateInputMappingCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ActionName;
    if (!JsonObject->TryGetStringField(TEXT("action_name"), ActionName) || ActionName.IsEmpty())
    {
        return false;
    }

    FString Key;
    if (!JsonObject->TryGetStringField(TEXT("key"), Key) || Key.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FCreateInputMappingCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Validate parameters
    if (!ValidateParams(Parameters))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString ActionName = JsonObject->GetStringField(TEXT("action_name"));
    FString Key = JsonObject->GetStringField(TEXT("key"));

    // Extract modifiers if present
    TSharedPtr<FJsonObject> Modifiers = MakeShared<FJsonObject>();
    if (JsonObject->HasField(TEXT("shift")))
    {
        Modifiers->SetBoolField(TEXT("shift"), JsonObject->GetBoolField(TEXT("shift")));
    }
    if (JsonObject->HasField(TEXT("ctrl")))
    {
        Modifiers->SetBoolField(TEXT("ctrl"), JsonObject->GetBoolField(TEXT("ctrl")));
    }
    if (JsonObject->HasField(TEXT("alt")))
    {
        Modifiers->SetBoolField(TEXT("alt"), JsonObject->GetBoolField(TEXT("alt")));
    }
    if (JsonObject->HasField(TEXT("cmd")))
    {
        Modifiers->SetBoolField(TEXT("cmd"), JsonObject->GetBoolField(TEXT("cmd")));
    }

    // Execute the operation
    FString Error;
    if (!ProjectService->CreateInputMapping(ActionName, Key, Modifiers, Error))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Error);
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetStringField(TEXT("action_name"), ActionName);
    ResponseData->SetStringField(TEXT("key"), Key);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}

