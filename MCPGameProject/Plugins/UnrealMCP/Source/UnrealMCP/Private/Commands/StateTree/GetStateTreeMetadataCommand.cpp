#include "Commands/StateTree/GetStateTreeMetadataCommand.h"
#include "StateTree.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetStateTreeMetadataCommand::FGetStateTreeMetadataCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetStateTreeMetadataCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString StateTreePath;
    if (!JsonObject->TryGetStringField(TEXT("state_tree_path"), StateTreePath))
    {
        return CreateErrorResponse(TEXT("Missing required 'state_tree_path' parameter"));
    }

    UStateTree* StateTree = Service.FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return CreateErrorResponse(FString::Printf(TEXT("StateTree not found: '%s'"), *StateTreePath));
    }

    TSharedPtr<FJsonObject> MetadataObj;
    if (!Service.GetStateTreeMetadata(StateTree, MetadataObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve metadata"));
    }

    // Wrap in success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), MetadataObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetStateTreeMetadataCommand::GetCommandName() const
{
    return TEXT("get_state_tree_metadata");
}

bool FGetStateTreeMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString StateTreePath;
    return JsonObject->TryGetStringField(TEXT("state_tree_path"), StateTreePath);
}

FString FGetStateTreeMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
