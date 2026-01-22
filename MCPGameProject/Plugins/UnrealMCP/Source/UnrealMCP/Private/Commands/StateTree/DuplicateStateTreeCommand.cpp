#include "Commands/StateTree/DuplicateStateTreeCommand.h"
#include "StateTree.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FDuplicateStateTreeCommand::FDuplicateStateTreeCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FDuplicateStateTreeCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SourcePath;
    if (!JsonObject->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return CreateErrorResponse(TEXT("Missing required 'source_path' parameter"));
    }

    FString DestPath;
    if (!JsonObject->TryGetStringField(TEXT("dest_path"), DestPath))
    {
        return CreateErrorResponse(TEXT("Missing required 'dest_path' parameter"));
    }

    FString NewName;
    if (!JsonObject->TryGetStringField(TEXT("new_name"), NewName))
    {
        return CreateErrorResponse(TEXT("Missing required 'new_name' parameter"));
    }

    FString DuplicateError;
    UStateTree* DuplicatedTree = Service.DuplicateStateTree(SourcePath, DestPath, NewName, DuplicateError);
    if (!DuplicatedTree)
    {
        return CreateErrorResponse(DuplicateError.IsEmpty() ? TEXT("Failed to duplicate StateTree") : DuplicateError);
    }

    return CreateSuccessResponse(DuplicatedTree);
}

FString FDuplicateStateTreeCommand::GetCommandName() const
{
    return TEXT("duplicate_state_tree");
}

bool FDuplicateStateTreeCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SourcePath, DestPath, NewName;
    return JsonObject->TryGetStringField(TEXT("source_path"), SourcePath) &&
           JsonObject->TryGetStringField(TEXT("dest_path"), DestPath) &&
           JsonObject->TryGetStringField(TEXT("new_name"), NewName);
}

FString FDuplicateStateTreeCommand::CreateSuccessResponse(UStateTree* StateTree) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), StateTree->GetName());
    ResponseObj->SetStringField(TEXT("path"), StateTree->GetPathName());
    ResponseObj->SetStringField(TEXT("message"), TEXT("StateTree duplicated successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FDuplicateStateTreeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
