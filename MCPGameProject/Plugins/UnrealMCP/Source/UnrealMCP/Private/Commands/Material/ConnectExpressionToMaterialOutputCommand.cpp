#include "Commands/Material/ConnectExpressionToMaterialOutputCommand.h"
#include "Services/MaterialExpressionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FConnectExpressionToMaterialOutputCommand::FConnectExpressionToMaterialOutputCommand()
{
}

FString FConnectExpressionToMaterialOutputCommand::Execute(const FString& Parameters)
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

    int32 OutputIndex = 0;
    JsonObject->TryGetNumberField(TEXT("output_index"), OutputIndex);

    FString MaterialProperty;
    if (!JsonObject->TryGetStringField(TEXT("material_property"), MaterialProperty))
    {
        return CreateErrorResponse(TEXT("Missing 'material_property' parameter"));
    }

    FString Error;
    if (!FMaterialExpressionService::Get().ConnectToMaterialOutput(MaterialPath, ExpressionId, OutputIndex, MaterialProperty, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(MaterialProperty);
}

FString FConnectExpressionToMaterialOutputCommand::GetCommandName() const
{
    return TEXT("connect_expression_to_material_output");
}

bool FConnectExpressionToMaterialOutputCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("material_path")) &&
           JsonObject->HasField(TEXT("expression_id")) &&
           JsonObject->HasField(TEXT("material_property"));
}

FString FConnectExpressionToMaterialOutputCommand::CreateSuccessResponse(const FString& MaterialProperty) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("connected_to"), MaterialProperty);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Expression connected to %s"), *MaterialProperty));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FConnectExpressionToMaterialOutputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
