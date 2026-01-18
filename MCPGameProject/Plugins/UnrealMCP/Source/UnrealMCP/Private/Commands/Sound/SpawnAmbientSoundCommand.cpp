#include "Commands/Sound/SpawnAmbientSoundCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FSpawnAmbientSoundCommand::FSpawnAmbientSoundCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FSpawnAmbientSoundCommand::GetCommandName() const
{
    return TEXT("spawn_ambient_sound");
}

bool FSpawnAmbientSoundCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundPath, ActorName, AttenuationPath, Error;
    FVector Location;
    FRotator Rotation;
    bool bAutoActivate;
    return ParseParameters(Parameters, SoundPath, ActorName, Location, Rotation, bAutoActivate, AttenuationPath, Error);
}

FString FSpawnAmbientSoundCommand::Execute(const FString& Parameters)
{
    FString SoundPath, ActorName, AttenuationPath, Error;
    FVector Location;
    FRotator Rotation;
    bool bAutoActivate;

    if (!ParseParameters(Parameters, SoundPath, ActorName, Location, Rotation, bAutoActivate, AttenuationPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    FAmbientSoundSpawnParams Params;
    Params.SoundPath = SoundPath;
    Params.ActorName = ActorName;
    Params.Location = Location;
    Params.Rotation = Rotation;
    Params.bAutoActivate = bAutoActivate;
    Params.AttenuationPath = AttenuationPath;

    FString OutActorName;
    AAmbientSound* Actor = SoundService.SpawnAmbientSound(Params, OutActorName, Error);
    if (!Actor)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(OutActorName, Location);
}

bool FSpawnAmbientSoundCommand::ParseParameters(const FString& JsonString, FString& OutSoundPath,
    FString& OutActorName, FVector& OutLocation, FRotator& OutRotation, bool& OutAutoActivate,
    FString& OutAttenuationPath, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("sound_path"), OutSoundPath) || OutSoundPath.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: sound_path");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("actor_name"), OutActorName) || OutActorName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: actor_name");
        return false;
    }

    // Optional location (default to origin)
    OutLocation = FVector::ZeroVector;
    if (JsonObject->HasField(TEXT("location")))
    {
        const TSharedPtr<FJsonObject>* LocationObj;
        if (JsonObject->TryGetObjectField(TEXT("location"), LocationObj))
        {
            (*LocationObj)->TryGetNumberField(TEXT("x"), OutLocation.X);
            (*LocationObj)->TryGetNumberField(TEXT("y"), OutLocation.Y);
            (*LocationObj)->TryGetNumberField(TEXT("z"), OutLocation.Z);
        }
    }

    // Optional rotation (default to zero)
    OutRotation = FRotator::ZeroRotator;
    if (JsonObject->HasField(TEXT("rotation")))
    {
        const TSharedPtr<FJsonObject>* RotationObj;
        if (JsonObject->TryGetObjectField(TEXT("rotation"), RotationObj))
        {
            (*RotationObj)->TryGetNumberField(TEXT("pitch"), OutRotation.Pitch);
            (*RotationObj)->TryGetNumberField(TEXT("yaw"), OutRotation.Yaw);
            (*RotationObj)->TryGetNumberField(TEXT("roll"), OutRotation.Roll);
        }
    }

    // Optional auto-activate (default true)
    OutAutoActivate = true;
    JsonObject->TryGetBoolField(TEXT("auto_activate"), OutAutoActivate);

    // Optional attenuation path
    JsonObject->TryGetStringField(TEXT("attenuation_path"), OutAttenuationPath);

    return true;
}

FString FSpawnAmbientSoundCommand::CreateSuccessResponse(const FString& ActorName, const FVector& Location) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("actor_name"), ActorName);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Spawned ambient sound: %s"), *ActorName));

    TSharedPtr<FJsonObject> LocationObj = MakeShared<FJsonObject>();
    LocationObj->SetNumberField(TEXT("x"), Location.X);
    LocationObj->SetNumberField(TEXT("y"), Location.Y);
    LocationObj->SetNumberField(TEXT("z"), Location.Z);
    Response->SetObjectField(TEXT("location"), LocationObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FSpawnAmbientSoundCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
