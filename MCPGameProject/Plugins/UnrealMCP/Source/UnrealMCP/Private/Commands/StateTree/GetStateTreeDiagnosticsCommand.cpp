#include "Commands/StateTree/GetStateTreeDiagnosticsCommand.h"
#include "StateTree.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetStateTreeDiagnosticsCommand::FGetStateTreeDiagnosticsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetStateTreeDiagnosticsCommand::Execute(const FString& Parameters)
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

    TSharedPtr<FJsonObject> DiagnosticsObj;
    if (!Service.GetStateTreeDiagnostics(StateTree, DiagnosticsObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve diagnostics"));
    }

    // Wrap in success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("diagnostics"), DiagnosticsObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetStateTreeDiagnosticsCommand::GetCommandName() const
{
    return TEXT("get_state_tree_diagnostics");
}

bool FGetStateTreeDiagnosticsCommand::ValidateParams(const FString& Parameters) const
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

FString FGetStateTreeDiagnosticsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
