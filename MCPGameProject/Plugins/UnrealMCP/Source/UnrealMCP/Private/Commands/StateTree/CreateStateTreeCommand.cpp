#include "Commands/StateTree/CreateStateTreeCommand.h"
#include "StateTree.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FCreateStateTreeCommand::FCreateStateTreeCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FCreateStateTreeCommand::Execute(const FString& Parameters)
{
    FStateTreeCreationParams Params;
    FString ParseError;

    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString CreateError;
    UStateTree* StateTree = Service.CreateStateTree(Params, CreateError);
    if (!StateTree)
    {
        return CreateErrorResponse(CreateError.IsEmpty() ? TEXT("Failed to create StateTree") : CreateError);
    }

    return CreateSuccessResponse(StateTree);
}

FString FCreateStateTreeCommand::GetCommandName() const
{
    return TEXT("create_state_tree");
}

bool FCreateStateTreeCommand::ValidateParams(const FString& Parameters) const
{
    FStateTreeCreationParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FCreateStateTreeCommand::ParseParameters(const FString& JsonString, FStateTreeCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing required 'name' parameter");
        return false;
    }

    JsonObject->TryGetStringField(TEXT("path"), OutParams.FolderPath);
    JsonObject->TryGetStringField(TEXT("schema_class"), OutParams.SchemaClass);
    JsonObject->TryGetBoolField(TEXT("compile_on_creation"), OutParams.bCompileOnCreation);

    return true;
}

FString FCreateStateTreeCommand::CreateSuccessResponse(UStateTree* StateTree) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), StateTree->GetName());
    ResponseObj->SetStringField(TEXT("path"), StateTree->GetPathName());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCreateStateTreeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
