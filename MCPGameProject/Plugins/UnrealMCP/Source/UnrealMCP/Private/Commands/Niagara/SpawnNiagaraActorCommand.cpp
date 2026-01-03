#include "Commands/Niagara/SpawnNiagaraActorCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSpawnNiagaraActorCommand::FSpawnNiagaraActorCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSpawnNiagaraActorCommand::Execute(const FString& Parameters)
{
    FNiagaraActorSpawnParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString ActorName;
    ANiagaraActor* Actor = NiagaraService.SpawnActor(Params, ActorName, Error);

    if (!Actor)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ActorName);
}

FString FSpawnNiagaraActorCommand::GetCommandName() const
{
    return TEXT("spawn_niagara_actor");
}

bool FSpawnNiagaraActorCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraActorSpawnParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSpawnNiagaraActorCommand::ParseParameters(const FString& JsonString, FNiagaraActorSpawnParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("system_path"), OutParams.SystemPath))
    {
        OutError = TEXT("Missing 'system_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("actor_name"), OutParams.ActorName))
    {
        OutError = TEXT("Missing 'actor_name' parameter");
        return false;
    }

    // Optional: location
    const TArray<TSharedPtr<FJsonValue>>* LocationArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("location"), LocationArray) && LocationArray->Num() >= 3)
    {
        OutParams.Location.X = (*LocationArray)[0]->AsNumber();
        OutParams.Location.Y = (*LocationArray)[1]->AsNumber();
        OutParams.Location.Z = (*LocationArray)[2]->AsNumber();
    }

    // Optional: rotation
    const TArray<TSharedPtr<FJsonValue>>* RotationArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("rotation"), RotationArray) && RotationArray->Num() >= 3)
    {
        OutParams.Rotation.Pitch = (*RotationArray)[0]->AsNumber();
        OutParams.Rotation.Yaw = (*RotationArray)[1]->AsNumber();
        OutParams.Rotation.Roll = (*RotationArray)[2]->AsNumber();
    }

    // Optional: auto_activate
    JsonObject->TryGetBoolField(TEXT("auto_activate"), OutParams.bAutoActivate);

    return OutParams.IsValid(OutError);
}

FString FSpawnNiagaraActorCommand::CreateSuccessResponse(const FString& ActorName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("actor_name"), ActorName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Niagara actor '%s' spawned successfully"), *ActorName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSpawnNiagaraActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
