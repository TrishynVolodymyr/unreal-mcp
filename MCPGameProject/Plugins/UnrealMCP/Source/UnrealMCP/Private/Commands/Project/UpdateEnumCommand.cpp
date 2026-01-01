#include "Commands/Project/UpdateEnumCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FUpdateEnumCommand::FUpdateEnumCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FUpdateEnumCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString EnumName;
    if (!JsonObject->TryGetStringField(TEXT("enum_name"), EnumName) || EnumName.IsEmpty())
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* ValuesArray;
    if (!JsonObject->TryGetArrayField(TEXT("values"), ValuesArray) || ValuesArray->Num() == 0)
    {
        return false;
    }

    return true;
}

FString FUpdateEnumCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: enum_name (string), values (array of strings)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString EnumName = JsonObject->GetStringField(TEXT("enum_name"));

    FString Path = TEXT("/Game");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString Description;
    JsonObject->TryGetStringField(TEXT("description"), Description);

    // Extract values array - supports both simple strings and objects with name/description
    const TArray<TSharedPtr<FJsonValue>>* ValuesArray;
    JsonObject->TryGetArrayField(TEXT("values"), ValuesArray);

    TArray<FString> EnumValues;
    TMap<FString, FString> ValueDescriptions;
    if (ValuesArray)
    {
        for (const TSharedPtr<FJsonValue>& Value : *ValuesArray)
        {
            // Check if this is a simple string value
            FString StringValue;
            if (Value->TryGetString(StringValue))
            {
                EnumValues.Add(StringValue);
            }
            // Or an object with name and description
            else if (Value->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ValueObj = Value->AsObject();
                if (ValueObj.IsValid())
                {
                    FString ValueName;
                    if (ValueObj->TryGetStringField(TEXT("name"), ValueName) && !ValueName.IsEmpty())
                    {
                        EnumValues.Add(ValueName);

                        // Get optional description
                        FString ValueDesc;
                        if (ValueObj->TryGetStringField(TEXT("description"), ValueDesc) && !ValueDesc.IsEmpty())
                        {
                            ValueDescriptions.Add(ValueName, ValueDesc);
                        }
                    }
                }
            }
        }
    }

    // Execute the operation
    FString Error;
    if (!ProjectService->UpdateEnum(EnumName, Path, Description, EnumValues, ValueDescriptions, Error))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Error);
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("enum_name"), EnumName);
    ResponseData->SetStringField(TEXT("path"), Path);
    ResponseData->SetNumberField(TEXT("value_count"), EnumValues.Num());
    ResponseData->SetStringField(TEXT("message"), TEXT("Enum updated successfully"));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
