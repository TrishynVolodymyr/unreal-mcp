#include "MCPResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FMCPResponse::ToJsonString() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    
    JsonObject->SetBoolField(TEXT("success"), bSuccess);
    JsonObject->SetStringField(TEXT("data"), Data);
    JsonObject->SetStringField(TEXT("metadata"), Metadata);
    
    // Add error information if present
    if (Error.HasError())
    {
        TSharedPtr<FJsonObject> ErrorObject = MakeShareable(new FJsonObject);
        ErrorObject->SetNumberField(TEXT("errorType"), static_cast<int32>(Error.ErrorType));
        ErrorObject->SetNumberField(TEXT("errorCode"), Error.ErrorCode);
        ErrorObject->SetStringField(TEXT("errorMessage"), Error.ErrorMessage);
        ErrorObject->SetStringField(TEXT("errorDetails"), Error.ErrorDetails);
        JsonObject->SetObjectField(TEXT("error"), ErrorObject);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

FMCPResponse FMCPResponse::FromJsonString(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return FMCPResponse::CreateFailure(
            FMCPError(EMCPErrorType::InternalError, 1001, TEXT("Failed to parse response JSON"))
        );
    }
    
    FMCPResponse Response;
    
    JsonObject->TryGetBoolField(TEXT("success"), Response.bSuccess);
    JsonObject->TryGetStringField(TEXT("data"), Response.Data);
    JsonObject->TryGetStringField(TEXT("metadata"), Response.Metadata);
    
    // Parse error information if present
    const TSharedPtr<FJsonObject>* ErrorObject;
    if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject->IsValid())
    {
        int32 ErrorTypeInt = 0;
        (*ErrorObject)->TryGetNumberField(TEXT("errorType"), ErrorTypeInt);
        Response.Error.ErrorType = static_cast<EMCPErrorType>(ErrorTypeInt);
        
        (*ErrorObject)->TryGetNumberField(TEXT("errorCode"), Response.Error.ErrorCode);
        (*ErrorObject)->TryGetStringField(TEXT("errorMessage"), Response.Error.ErrorMessage);
        (*ErrorObject)->TryGetStringField(TEXT("errorDetails"), Response.Error.ErrorDetails);
    }
    
    return Response;
}