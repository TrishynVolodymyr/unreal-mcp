#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FMCPError::ToJsonString() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    
    JsonObject->SetNumberField(TEXT("errorType"), static_cast<int32>(ErrorType));
    JsonObject->SetNumberField(TEXT("errorCode"), ErrorCode);
    JsonObject->SetStringField(TEXT("errorMessage"), ErrorMessage);
    JsonObject->SetStringField(TEXT("errorDetails"), ErrorDetails);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

FMCPError FMCPError::FromJsonString(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return FMCPError(EMCPErrorType::InternalError, 1000, TEXT("Failed to parse error JSON"));
    }
    
    FMCPError Error;
    
    int32 ErrorTypeInt = 0;
    if (JsonObject->TryGetNumberField(TEXT("errorType"), ErrorTypeInt))
    {
        Error.ErrorType = static_cast<EMCPErrorType>(ErrorTypeInt);
    }
    
    JsonObject->TryGetNumberField(TEXT("errorCode"), Error.ErrorCode);
    JsonObject->TryGetStringField(TEXT("errorMessage"), Error.ErrorMessage);
    JsonObject->TryGetStringField(TEXT("errorDetails"), Error.ErrorDetails);
    
    return Error;
}