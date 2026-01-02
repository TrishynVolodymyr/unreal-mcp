#include "Commands/Material/ConnectMaterialExpressionsCommand.h"
#include "Services/MaterialExpressionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FConnectMaterialExpressionsCommand::FConnectMaterialExpressionsCommand()
{
}

FString FConnectMaterialExpressionsCommand::Execute(const FString& Parameters)
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

    // Check for batch mode (connections array)
    const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("connections"), ConnectionsArray) && ConnectionsArray && ConnectionsArray->Num() > 0)
    {
        // Batch mode - parse all connections
        TArray<FMaterialExpressionConnectionParams> AllConnections;
        for (const TSharedPtr<FJsonValue>& ConnValue : *ConnectionsArray)
        {
            const TSharedPtr<FJsonObject>* ConnObj = nullptr;
            if (!ConnValue->TryGetObject(ConnObj) || !ConnObj)
            {
                return CreateErrorResponse(TEXT("Invalid connection object in connections array"));
            }

            FMaterialExpressionConnectionParams Params;
            Params.MaterialPath = MaterialPath;

            FString SourceIdStr;
            if (!(*ConnObj)->TryGetStringField(TEXT("source_expression_id"), SourceIdStr))
            {
                return CreateErrorResponse(TEXT("Missing 'source_expression_id' in connection"));
            }
            FGuid::Parse(SourceIdStr, Params.SourceExpressionId);

            FString TargetIdStr;
            if (!(*ConnObj)->TryGetStringField(TEXT("target_expression_id"), TargetIdStr))
            {
                return CreateErrorResponse(TEXT("Missing 'target_expression_id' in connection"));
            }
            FGuid::Parse(TargetIdStr, Params.TargetExpressionId);

            int32 SourceIdx = 0;
            (*ConnObj)->TryGetNumberField(TEXT("source_output_index"), SourceIdx);
            Params.SourceOutputIndex = SourceIdx;

            if (!(*ConnObj)->TryGetStringField(TEXT("target_input_name"), Params.TargetInputName))
            {
                return CreateErrorResponse(TEXT("Missing 'target_input_name' in connection"));
            }

            AllConnections.Add(Params);
        }

        // Execute batch connection
        FString Error;
        TArray<FString> Results;
        if (!FMaterialExpressionService::Get().ConnectExpressionsBatch(MaterialPath, AllConnections, Results, Error))
        {
            return CreateErrorResponse(Error);
        }

        return CreateBatchSuccessResponse(Results);
    }
    else
    {
        // Single connection mode (backward compatible)
        FMaterialExpressionConnectionParams Params;
        FString Error;

        if (!ParseParameters(Parameters, Params, Error))
        {
            return CreateErrorResponse(Error);
        }

        if (!FMaterialExpressionService::Get().ConnectExpressions(Params, Error))
        {
            return CreateErrorResponse(Error);
        }

        return CreateSuccessResponse();
    }
}

FString FConnectMaterialExpressionsCommand::GetCommandName() const
{
    return TEXT("connect_material_expressions");
}

bool FConnectMaterialExpressionsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // Must have material_path
    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return false;
    }

    // Either connections array (batch mode) or single connection params
    const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("connections"), ConnectionsArray) && ConnectionsArray && ConnectionsArray->Num() > 0)
    {
        return true; // Batch mode - valid if we have connections array
    }

    // Single mode - validate individual params
    FMaterialExpressionConnectionParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FConnectMaterialExpressionsCommand::ParseParameters(const FString& JsonString, FMaterialExpressionConnectionParams& OutParams, FString& OutError) const
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

    FString SourceIdStr;
    if (!JsonObject->TryGetStringField(TEXT("source_expression_id"), SourceIdStr))
    {
        OutError = TEXT("Missing 'source_expression_id' parameter");
        return false;
    }
    FGuid::Parse(SourceIdStr, OutParams.SourceExpressionId);

    FString TargetIdStr;
    if (!JsonObject->TryGetStringField(TEXT("target_expression_id"), TargetIdStr))
    {
        OutError = TEXT("Missing 'target_expression_id' parameter");
        return false;
    }
    FGuid::Parse(TargetIdStr, OutParams.TargetExpressionId);

    // Optional source output index with default
    int32 SourceIdx = 0;
    if (JsonObject->TryGetNumberField(TEXT("source_output_index"), SourceIdx))
    {
        OutParams.SourceOutputIndex = SourceIdx;
    }

    // Target input name (required)
    if (!JsonObject->TryGetStringField(TEXT("target_input_name"), OutParams.TargetInputName))
    {
        OutError = TEXT("Missing 'target_input_name' parameter");
        return false;
    }

    return OutParams.IsValid(OutError);
}

FString FConnectMaterialExpressionsCommand::CreateSuccessResponse() const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Expressions connected successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FConnectMaterialExpressionsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FConnectMaterialExpressionsCommand::CreateBatchSuccessResponse(const TArray<FString>& Results) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetNumberField(TEXT("connections_made"), Results.Num());
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Connected %d expressions"), Results.Num()));

    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (const FString& Result : Results)
    {
        ResultsArray.Add(MakeShared<FJsonValueString>(Result));
    }
    ResponseObj->SetArrayField(TEXT("results"), ResultsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
