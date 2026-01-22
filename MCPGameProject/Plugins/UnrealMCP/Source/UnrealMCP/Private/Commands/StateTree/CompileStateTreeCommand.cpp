#include "Commands/StateTree/CompileStateTreeCommand.h"
#include "StateTree.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FCompileStateTreeCommand::FCompileStateTreeCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FCompileStateTreeCommand::Execute(const FString& Parameters)
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

    FString CompileError;
    if (!Service.CompileStateTree(StateTree, CompileError))
    {
        return CreateErrorResponse(CompileError.IsEmpty() ? TEXT("Compilation failed") : CompileError);
    }

    return CreateSuccessResponse(StateTree->GetName());
}

FString FCompileStateTreeCommand::GetCommandName() const
{
    return TEXT("compile_state_tree");
}

bool FCompileStateTreeCommand::ValidateParams(const FString& Parameters) const
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

FString FCompileStateTreeCommand::CreateSuccessResponse(const FString& StateTreeName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_tree_name"), StateTreeName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("StateTree '%s' compiled successfully"), *StateTreeName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCompileStateTreeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
