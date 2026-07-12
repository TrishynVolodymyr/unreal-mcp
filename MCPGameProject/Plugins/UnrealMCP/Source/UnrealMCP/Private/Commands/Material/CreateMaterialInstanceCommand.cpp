#include "Commands/Material/CreateMaterialInstanceCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateMaterialInstanceCommand::FCreateMaterialInstanceCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FCreateMaterialInstanceCommand::Execute(const FString& Parameters)
{
    FMaterialInstanceCreationParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString InstancePath;
    UMaterialInterface* Instance = MaterialService.CreateMaterialInstance(Params, InstancePath, Error);

    if (!Instance)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(InstancePath, Params.bIsDynamic, Params.ParentMaterialPath);
}

FString FCreateMaterialInstanceCommand::GetCommandName() const
{
    return TEXT("create_material_instance");
}

bool FCreateMaterialInstanceCommand::ValidateParams(const FString& Parameters) const
{
    FMaterialInstanceCreationParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FCreateMaterialInstanceCommand::ParseParameters(const FString& JsonString, FMaterialInstanceCreationParams& OutParams, FString& OutError) const
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
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("parent_material"), OutParams.ParentMaterialPath))
    {
        OutError = TEXT("Missing 'parent_material' parameter");
        return false;
    }

    // Optional parameters - accept both 'path' and 'folder_path' for compatibility with MCP
    if (!JsonObject->TryGetStringField(TEXT("path"), OutParams.Path))
    {
        JsonObject->TryGetStringField(TEXT("folder_path"), OutParams.Path);
    }
    JsonObject->TryGetBoolField(TEXT("is_dynamic"), OutParams.bIsDynamic);

    const TSharedPtr<FJsonObject>* ScalarObject = nullptr;
    if (JsonObject->HasField(TEXT("scalar_params")))
    {
        if (!JsonObject->TryGetObjectField(TEXT("scalar_params"), ScalarObject))
        {
            OutError = TEXT("'scalar_params' must be a JSON object");
            return false;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*ScalarObject)->Values)
        {
            if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Number)
            {
                OutError = FString::Printf(TEXT("scalar_params.%s must be a number"), *Pair.Key);
                return false;
            }
            OutParams.ScalarParameters.Add(Pair.Key, static_cast<float>(Pair.Value->AsNumber()));
        }
    }

    const TSharedPtr<FJsonObject>* VectorObject = nullptr;
    if (JsonObject->HasField(TEXT("vector_params")))
    {
        if (!JsonObject->TryGetObjectField(TEXT("vector_params"), VectorObject))
        {
            OutError = TEXT("'vector_params' must be a JSON object");
            return false;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*VectorObject)->Values)
        {
            const TArray<TSharedPtr<FJsonValue>>* Components = nullptr;
            if (!Pair.Value.IsValid() || !Pair.Value->TryGetArray(Components)
                || Components->Num() < 3 || Components->Num() > 4)
            {
                OutError = FString::Printf(TEXT("vector_params.%s must contain 3 or 4 numbers"), *Pair.Key);
                return false;
            }
            for (const TSharedPtr<FJsonValue>& Component : *Components)
            {
                if (!Component.IsValid() || Component->Type != EJson::Number)
                {
                    OutError = FString::Printf(TEXT("vector_params.%s components must be numbers"), *Pair.Key);
                    return false;
                }
            }
            OutParams.VectorParameters.Add(Pair.Key, FLinearColor(
                static_cast<float>((*Components)[0]->AsNumber()),
                static_cast<float>((*Components)[1]->AsNumber()),
                static_cast<float>((*Components)[2]->AsNumber()),
                Components->Num() == 4 ? static_cast<float>((*Components)[3]->AsNumber()) : 1.0f));
        }
    }

    const TSharedPtr<FJsonObject>* TextureObject = nullptr;
    if (JsonObject->HasField(TEXT("texture_params")))
    {
        if (!JsonObject->TryGetObjectField(TEXT("texture_params"), TextureObject))
        {
            OutError = TEXT("'texture_params' must be a JSON object");
            return false;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*TextureObject)->Values)
        {
            if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::String)
            {
                OutError = FString::Printf(TEXT("texture_params.%s must be an asset path string"), *Pair.Key);
                return false;
            }
            OutParams.TextureParameters.Add(Pair.Key, Pair.Value->AsString());
        }
    }

    return OutParams.IsValid(OutError);
}

FString FCreateMaterialInstanceCommand::CreateSuccessResponse(const FString& InstancePath, bool bIsDynamic, const FString& ParentPath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("instance_path"), InstancePath);
    ResponseObj->SetStringField(TEXT("instance_type"), bIsDynamic ? TEXT("MaterialInstanceDynamic") : TEXT("MaterialInstanceConstant"));
    ResponseObj->SetStringField(TEXT("parent_material"), ParentPath);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Material instance created at %s"), *InstancePath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateMaterialInstanceCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
