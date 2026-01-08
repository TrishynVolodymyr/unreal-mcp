#include "Commands/Material/CompileMaterialCommand.h"
#include "Services/MaterialExpressionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCompileMaterialCommand::FCompileMaterialCommand()
{
}

FString FCompileMaterialCommand::Execute(const FString& Parameters)
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

    TSharedPtr<FJsonObject> Result;
    FString Error;
    bool bSuccess = FMaterialExpressionService::Get().CompileMaterial(MaterialPath, Result, Error);

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);

    return OutputString;
}

FString FCompileMaterialCommand::GetCommandName() const
{
    return TEXT("compile_material");
}

bool FCompileMaterialCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("material_path"));
}

FString FCompileMaterialCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
