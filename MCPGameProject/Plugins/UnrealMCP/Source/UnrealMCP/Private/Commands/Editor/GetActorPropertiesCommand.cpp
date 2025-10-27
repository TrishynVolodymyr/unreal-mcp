#include "Commands/Editor/GetActorPropertiesCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetActorPropertiesCommand::FGetActorPropertiesCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FGetActorPropertiesCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    FString Error;
    
    if (!ParseParameters(Parameters, ActorName, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    // Find the actor
    AActor* Actor = EditorService.FindActorByName(ActorName);
    if (!Actor)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }
    
    return CreateSuccessResponse(Actor);
}

FString FGetActorPropertiesCommand::GetCommandName() const
{
    return TEXT("get_actor_properties");
}

bool FGetActorPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    FString Error;
    return ParseParameters(Parameters, ActorName, Error);
}

bool FGetActorPropertiesCommand::ParseParameters(const FString& JsonString, FString& OutActorName, FString& OutError) const
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
    
    return true;
}

FString FGetActorPropertiesCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    // Create comprehensive actor properties response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    // Basic actor info
    ResponseObj->SetStringField(TEXT("name"), Actor->GetName());
    ResponseObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    // Transform information
    FTransform Transform = Actor->GetTransform();
    TSharedPtr<FJsonObject> TransformObj = MakeShared<FJsonObject>();
    
    // Location
    FVector Location = Transform.GetLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    TransformObj->SetArrayField(TEXT("location"), LocationArray);
    
    // Rotation
    FRotator Rotation = Transform.GetRotation().Rotator();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    TransformObj->SetArrayField(TEXT("rotation"), RotationArray);
    
    // Scale
    FVector Scale = Transform.GetScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    TransformObj->SetArrayField(TEXT("scale"), ScaleArray);
    
    ResponseObj->SetObjectField(TEXT("transform"), TransformObj);
    
    // Additional properties
    ResponseObj->SetBoolField(TEXT("hidden"), Actor->IsHidden());
    ResponseObj->SetStringField(TEXT("mobility"), Actor->GetRootComponent() ? 
        UEnum::GetValueAsString(Actor->GetRootComponent()->Mobility) : TEXT("Unknown"));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetActorPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

