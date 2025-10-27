#include "Commands/Project/ShowStructVariablesCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FShowStructVariablesCommand::FShowStructVariablesCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FShowStructVariablesCommand::GetCommandName() const
{
    return TEXT("show_struct_variables");
}

bool FShowStructVariablesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString StructName;
    if (!JsonObject->TryGetStringField(TEXT("struct_name"), StructName) || StructName.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FShowStructVariablesCommand::Execute(const FString& Parameters)
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
    FString StructName = JsonObject->GetStringField(TEXT("struct_name"));
    
    FString Path = TEXT("/Game/Blueprints");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    // Execute the operation using ProjectService
    bool bSuccess = false;
    FString Error;
    TArray<TSharedPtr<FJsonObject>> Variables = ProjectService->ShowStructVariables(StructName, Path, bSuccess, Error);
    
    if (!bSuccess)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Error);
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Convert variables to JSON array
    TArray<TSharedPtr<FJsonValue>> VariableArray;
    for (const TSharedPtr<FJsonObject>& Variable : Variables)
    {
        VariableArray.Add(MakeShared<FJsonValueObject>(Variable));
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("struct_name"), StructName);
    ResponseData->SetStringField(TEXT("path"), Path);
    ResponseData->SetArrayField(TEXT("variables"), VariableArray);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}

