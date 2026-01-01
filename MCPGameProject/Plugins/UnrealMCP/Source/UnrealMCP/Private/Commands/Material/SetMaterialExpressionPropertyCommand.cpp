#include "Commands/Material/SetMaterialExpressionPropertyCommand.h"
#include "Services/MaterialExpressionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetMaterialExpressionPropertyCommand::FSetMaterialExpressionPropertyCommand()
{
}

FString FSetMaterialExpressionPropertyCommand::Execute(const FString& Parameters)
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

    FString PropertyName;
    if (!JsonObject->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    TSharedPtr<FJsonValue> PropertyValue = JsonObject->TryGetField(TEXT("property_value"));
    if (!PropertyValue.IsValid())
    {
        return CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    FString Error;
    if (!FMaterialExpressionService::Get().SetExpressionProperty(MaterialPath, ExpressionId, PropertyName, PropertyValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(PropertyName);
}

FString FSetMaterialExpressionPropertyCommand::GetCommandName() const
{
    return TEXT("set_material_expression_property");
}

bool FSetMaterialExpressionPropertyCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("material_path")) &&
           JsonObject->HasField(TEXT("expression_id")) &&
           JsonObject->HasField(TEXT("property_name")) &&
           JsonObject->HasField(TEXT("property_value"));
}

FString FSetMaterialExpressionPropertyCommand::CreateSuccessResponse(const FString& PropertyName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("property_name"), PropertyName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Property %s set successfully"), *PropertyName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMaterialExpressionPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
