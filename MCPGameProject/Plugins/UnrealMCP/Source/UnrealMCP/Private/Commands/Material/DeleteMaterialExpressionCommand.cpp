#include "Commands/Material/DeleteMaterialExpressionCommand.h"
#include "Services/MaterialExpressionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FDeleteMaterialExpressionCommand::FDeleteMaterialExpressionCommand()
{
}

FString FDeleteMaterialExpressionCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionIdStr;
    if (!JsonObject->TryGetStringField(TEXT("expression_id"), ExpressionIdStr))
    {
        return CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }
    FGuid ExpressionId;
    FGuid::Parse(ExpressionIdStr, ExpressionId);

    FString Error;
    if (!FMaterialExpressionService::Get().DeleteExpression(MaterialPath, ExpressionId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse();
}

FString FDeleteMaterialExpressionCommand::GetCommandName() const
{
    return TEXT("delete_material_expression");
}

bool FDeleteMaterialExpressionCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("material_path")) && JsonObject->HasField(TEXT("expression_id"));
}

FString FDeleteMaterialExpressionCommand::CreateSuccessResponse() const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Expression deleted successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FDeleteMaterialExpressionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
