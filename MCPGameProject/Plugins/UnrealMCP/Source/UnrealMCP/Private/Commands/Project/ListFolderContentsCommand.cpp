#include "Commands/Project/ListFolderContentsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FListFolderContentsCommand::FListFolderContentsCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FListFolderContentsCommand::GetCommandName() const
{
    return TEXT("list_folder_contents");
}

bool FListFolderContentsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString FolderPath;
    if (!JsonObject->TryGetStringField(TEXT("folder_path"), FolderPath) || FolderPath.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FListFolderContentsCommand::Execute(const FString& Parameters)
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
    FString FolderPath = JsonObject->GetStringField(TEXT("folder_path"));

    // Execute the operation using ProjectService
    bool bSuccess = false;
    FString Error;
    TArray<FString> Contents = ProjectService->ListFolderContents(FolderPath, bSuccess, Error);
    
    if (!bSuccess)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Error);
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Convert contents to JSON array
    TArray<TSharedPtr<FJsonValue>> ContentsArray;
    for (const FString& Content : Contents)
    {
        ContentsArray.Add(MakeShared<FJsonValueString>(Content));
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("folder_path"), FolderPath);
    ResponseData->SetNumberField(TEXT("count"), Contents.Num());
    ResponseData->SetArrayField(TEXT("contents"), ContentsArray);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}

