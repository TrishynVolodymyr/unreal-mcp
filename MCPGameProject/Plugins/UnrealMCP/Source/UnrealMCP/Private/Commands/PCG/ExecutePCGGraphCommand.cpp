#include "Commands/PCG/ExecutePCGGraphCommand.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FExecutePCGGraphCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    bool bForce = true;
    FString Error;

    if (!ParseParameters(Parameters, ActorName, bForce, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Get editor world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Find the actor by label or name
    AActor* FoundActor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
        {
            continue;
        }

        // Match by actor label (display name in editor) or by object name
        FString ActorLabel = Actor->GetActorNameOrLabel();
        FString ObjectName = Actor->GetName();

        if (ActorLabel.Equals(ActorName, ESearchCase::IgnoreCase) ||
            ObjectName.Equals(ActorName, ESearchCase::IgnoreCase))
        {
            FoundActor = Actor;
            break;
        }
    }

    if (!FoundActor)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get the PCG Component from the actor
    UPCGComponent* PCGComponent = FoundActor->FindComponentByClass<UPCGComponent>();
    if (!PCGComponent)
    {
        return CreateErrorResponse(FString::Printf(
            TEXT("No PCG Component found on actor: %s"), *ActorName));
    }

    // Check if the component has a graph assigned
    if (!PCGComponent->GetGraph())
    {
        return CreateErrorResponse(FString::Printf(
            TEXT("PCG Component on '%s' has no graph assigned"), *ActorName));
    }

    // Clean up previous generation first
    PCGComponent->CleanupLocalImmediate(/*bRemoveComponents=*/true);

    // Trigger generation
    PCGComponent->GenerateLocal(bForce);

    // Check if generation was actually scheduled
    bool bIsGenerating = PCGComponent->IsGenerating();

    FString ActorLabel = FoundActor->GetActorNameOrLabel();
    FString Message = FString::Printf(
        TEXT("PCG generation triggered on %s (force=%s, generating=%s)"),
        *ActorLabel,
        bForce ? TEXT("true") : TEXT("false"),
        bIsGenerating ? TEXT("true") : TEXT("false"));

    return CreateSuccessResponse(Message);
}

FString FExecutePCGGraphCommand::GetCommandName() const
{
    return TEXT("execute_pcg_graph");
}

bool FExecutePCGGraphCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    bool bForce;
    FString Error;
    return ParseParameters(Parameters, ActorName, bForce, Error);
}

bool FExecutePCGGraphCommand::ParseParameters(const FString& JsonString, FString& OutActorName,
                                               bool& OutForce, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("actor_name"), OutActorName))
    {
        OutError = TEXT("Missing 'actor_name' parameter");
        return false;
    }

    // Optional force parameter, defaults to true
    if (!JsonObject->TryGetBoolField(TEXT("force"), OutForce))
    {
        OutForce = true;
    }

    return true;
}

FString FExecutePCGGraphCommand::CreateSuccessResponse(const FString& Message) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), Message);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FExecutePCGGraphCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
