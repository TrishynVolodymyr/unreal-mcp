#include "Commands/Material/AddMaterialExpressionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FAddMaterialExpressionCommand::FAddMaterialExpressionCommand()
{
}

FString FAddMaterialExpressionCommand::Execute(const FString& Parameters)
{
    FMaterialExpressionCreationParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ExpressionInfo;
    UMaterialExpression* Expression = FMaterialExpressionService::Get().AddExpression(Params, ExpressionInfo, Error);

    if (!Expression)
    {
        return CreateErrorResponse(Error);
    }

    // Return the expression info JSON directly
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ExpressionInfo.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddMaterialExpressionCommand::GetCommandName() const
{
    return TEXT("add_material_expression");
}

bool FAddMaterialExpressionCommand::ValidateParams(const FString& Parameters) const
{
    FMaterialExpressionCreationParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FAddMaterialExpressionCommand::ParseParameters(const FString& JsonString, FMaterialExpressionCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("material_path"), OutParams.MaterialPath))
    {
        OutError = TEXT("Missing 'material_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("expression_type"), OutParams.ExpressionType))
    {
        OutError = TEXT("Missing 'expression_type' parameter");
        return false;
    }

    // Parse optional position
    const TArray<TSharedPtr<FJsonValue>>* PositionArray;
    if (JsonObject->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
    {
        OutParams.Position.X = (*PositionArray)[0]->AsNumber();
        OutParams.Position.Y = (*PositionArray)[1]->AsNumber();
    }

    // Parse optional properties
    const TSharedPtr<FJsonObject>* PropertiesObj;
    if (JsonObject->TryGetObjectField(TEXT("properties"), PropertiesObj))
    {
        OutParams.Properties = *PropertiesObj;
    }

    return OutParams.IsValid(OutError);
}

FString FAddMaterialExpressionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
