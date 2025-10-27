#include "Commands/Editor/SetActorTransformCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetActorTransformCommand::FSetActorTransformCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FSetActorTransformCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    TOptional<FVector> Location;
    TOptional<FRotator> Rotation;
    TOptional<FVector> Scale;
    FString Error;
    
    if (!ParseParameters(Parameters, ActorName, Location, Rotation, Scale, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    // Find the actor
    AActor* Actor = EditorService.FindActorByName(ActorName);
    if (!Actor)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }
    
    // Set the transform
    const FVector* LocationPtr = Location.IsSet() ? &Location.GetValue() : nullptr;
    const FRotator* RotationPtr = Rotation.IsSet() ? &Rotation.GetValue() : nullptr;
    const FVector* ScalePtr = Scale.IsSet() ? &Scale.GetValue() : nullptr;
    
    if (!EditorService.SetActorTransform(Actor, LocationPtr, RotationPtr, ScalePtr))
    {
        return CreateErrorResponse(TEXT("Failed to set actor transform"));
    }
    
    return CreateSuccessResponse(Actor);
}

FString FSetActorTransformCommand::GetCommandName() const
{
    return TEXT("set_actor_transform");
}

bool FSetActorTransformCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    TOptional<FVector> Location;
    TOptional<FRotator> Rotation;
    TOptional<FVector> Scale;
    FString Error;
    return ParseParameters(Parameters, ActorName, Location, Rotation, Scale, Error);
}

bool FSetActorTransformCommand::ParseParameters(const FString& JsonString, FString& OutActorName, 
                                               TOptional<FVector>& OutLocation, TOptional<FRotator>& OutRotation, 
                                               TOptional<FVector>& OutScale, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Get required actor name
    if (!JsonObject->TryGetStringField(TEXT("name"), OutActorName))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }
    
    // Get optional transform parameters
    if (JsonObject->HasField(TEXT("location")))
    {
        OutLocation = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("location"));
    }
    
    if (JsonObject->HasField(TEXT("rotation")))
    {
        OutRotation = FUnrealMCPCommonUtils::GetRotatorFromJson(JsonObject, TEXT("rotation"));
    }
    
    if (JsonObject->HasField(TEXT("scale")))
    {
        OutScale = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("scale"));
    }
    
    return true;
}

FString FSetActorTransformCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Actor transform updated successfully"));
    
    // Add updated actor info
    TSharedPtr<FJsonObject> ActorJson = FUnrealMCPCommonUtils::ActorToJsonObject(Actor, true);
    ResponseObj->SetObjectField(TEXT("actor"), ActorJson);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetActorTransformCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

