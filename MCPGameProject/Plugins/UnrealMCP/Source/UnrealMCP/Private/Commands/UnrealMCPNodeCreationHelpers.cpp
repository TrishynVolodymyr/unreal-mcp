#include "Commands/UnrealMCPNodeCreationHelpers.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool UnrealMCPNodeCreationHelpers::ParseJsonParameters(const FString& JsonParams, TSharedPtr<FJsonObject>& OutParamsObject, TSharedPtr<FJsonObject>& OutResultObj)
{
    if (!JsonParams.IsEmpty())
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonParams);
        if (!FJsonSerializer::Deserialize(Reader, OutParamsObject) || !OutParamsObject.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to parse JSON parameters"));
            OutResultObj->SetBoolField(TEXT("success"), false);
            OutResultObj->SetStringField(TEXT("message"), TEXT("Invalid JSON parameters"));
            return false;
        }
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully parsed JSON parameters"));
    }
    return true;
} 