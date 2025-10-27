#include "Commands/BlueprintNode/UnrealMCPNodeCreationHelpers.h"
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
            // Get detailed error information from the reader
            FString ErrorMessage = Reader->GetErrorMessage();
            int32 LineNumber = Reader->GetLineNumber();
            int32 CharacterNumber = Reader->GetCharacterNumber();
            
            FString DetailedError = FString::Printf(TEXT("JSON parsing failed at line %d, character %d: %s"), 
                                                  LineNumber, CharacterNumber, *ErrorMessage);
            
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: %s"), *DetailedError);
            
            OutResultObj->SetBoolField(TEXT("success"), false);
            OutResultObj->SetStringField(TEXT("message"), DetailedError);
            return false;
        }
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully parsed JSON parameters"));
    }
    return true;
} 
