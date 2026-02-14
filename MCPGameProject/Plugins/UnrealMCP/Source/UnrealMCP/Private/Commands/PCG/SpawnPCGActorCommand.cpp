#include "Commands/PCG/SpawnPCGActorCommand.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FSpawnPCGActorCommand::Execute(const FString& Parameters)
{
	FSpawnPCGActorParams Params;
	FString Error;

	if (!ParseParameters(Parameters, Params, Error))
	{
		return CreateErrorResponse(Error);
	}

	// Get the editor world
	if (!GEditor)
	{
		return CreateErrorResponse(TEXT("GEditor is not available"));
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return CreateErrorResponse(TEXT("Failed to get editor world"));
	}

	// Spawn a blank AActor
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewActor = World->SpawnActor<AActor>(AActor::StaticClass(), Params.Location, Params.Rotation, SpawnInfo);
	if (!NewActor)
	{
		return CreateErrorResponse(TEXT("Failed to spawn actor"));
	}

	// Set actor label
	if (!Params.ActorLabel.IsEmpty())
	{
#if WITH_EDITOR
		NewActor->SetActorLabel(*Params.ActorLabel);
#endif
	}

	// Add a box component as root — PCG requires valid bounds from a UPrimitiveComponent
	// to register the PCG Component with the subsystem. Without this, GetGridBounds()
	// returns invalid bounds and generation silently fails.
	UBoxComponent* BoxComp = NewObject<UBoxComponent>(NewActor, UBoxComponent::StaticClass(), TEXT("PCGBoundsVolume"));
	if (BoxComp)
	{
		// Default extents: 10m x 10m x 10m (1000 UU per side) — can be overridden via volume_extents param
		FVector Extents(500.0, 500.0, 500.0);
		if (Params.VolumeExtents.X > 0 && Params.VolumeExtents.Y > 0 && Params.VolumeExtents.Z > 0)
		{
			Extents = Params.VolumeExtents;
		}
		BoxComp->SetBoxExtent(Extents);
		BoxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoxComp->SetVisibility(false);
		BoxComp->RegisterComponent();
		NewActor->SetRootComponent(BoxComp);
	}

	// Create and attach the PCG Component
	UPCGComponent* PCGComp = NewObject<UPCGComponent>(NewActor, UPCGComponent::StaticClass());
	if (!PCGComp)
	{
		NewActor->Destroy();
		return CreateErrorResponse(TEXT("Failed to create PCG Component"));
	}

	PCGComp->RegisterComponent();
	NewActor->AddInstanceComponent(PCGComp);

	// Load the PCG Graph asset
	UPCGGraphInterface* Graph = LoadObject<UPCGGraph>(nullptr, *Params.GraphPath);
	if (!Graph)
	{
		// The path might point to a UPCGGraphInstance, try loading as UPCGGraphInterface
		Graph = LoadObject<UPCGGraphInterface>(nullptr, *Params.GraphPath);
	}

	if (!Graph)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPCGActor: Could not load PCG graph at '%s'. Actor spawned without graph."), *Params.GraphPath);
	}
	else
	{
		PCGComp->SetGraph(Graph);
	}

	// Set generation trigger
	if (Params.GenerationTrigger.Equals(TEXT("GenerateOnDemand"), ESearchCase::IgnoreCase))
	{
		PCGComp->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
	}
	else if (Params.GenerationTrigger.Equals(TEXT("GenerateAtRuntime"), ESearchCase::IgnoreCase))
	{
		PCGComp->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateAtRuntime;
	}
	else
	{
		PCGComp->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnLoad;
	}

	// Mark world dirty
	World->MarkPackageDirty();

	// Build response
	FString ActorName = NewActor->GetActorLabel();
	if (ActorName.IsEmpty())
	{
		ActorName = NewActor->GetName();
	}

	FString ActorPath = NewActor->GetPathName();
	FString ComponentName = PCGComp->GetName();

	return CreateSuccessResponse(ActorName, ActorPath, ComponentName);
}

FString FSpawnPCGActorCommand::GetCommandName() const
{
	return TEXT("spawn_pcg_actor");
}

bool FSpawnPCGActorCommand::ValidateParams(const FString& Parameters) const
{
	FSpawnPCGActorParams Params;
	FString Error;
	return ParseParameters(Parameters, Params, Error);
}

bool FSpawnPCGActorCommand::ParseParameters(const FString& JsonString, FSpawnPCGActorParams& OutParams, FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	// Required: graph_path
	if (!JsonObject->TryGetStringField(TEXT("graph_path"), OutParams.GraphPath))
	{
		OutError = TEXT("Missing required 'graph_path' parameter");
		return false;
	}

	// Optional: location [X, Y, Z]
	if (JsonObject->HasField(TEXT("location")))
	{
		const TArray<TSharedPtr<FJsonValue>>* LocationArray;
		if (JsonObject->TryGetArrayField(TEXT("location"), LocationArray) && LocationArray->Num() >= 3)
		{
			OutParams.Location.X = (*LocationArray)[0]->AsNumber();
			OutParams.Location.Y = (*LocationArray)[1]->AsNumber();
			OutParams.Location.Z = (*LocationArray)[2]->AsNumber();
		}
	}

	// Optional: rotation [Pitch, Yaw, Roll]
	if (JsonObject->HasField(TEXT("rotation")))
	{
		const TArray<TSharedPtr<FJsonValue>>* RotationArray;
		if (JsonObject->TryGetArrayField(TEXT("rotation"), RotationArray) && RotationArray->Num() >= 3)
		{
			OutParams.Rotation.Pitch = (*RotationArray)[0]->AsNumber();
			OutParams.Rotation.Yaw = (*RotationArray)[1]->AsNumber();
			OutParams.Rotation.Roll = (*RotationArray)[2]->AsNumber();
		}
	}

	// Optional: actor_label (also accept actor_name for convenience)
	if (!JsonObject->TryGetStringField(TEXT("actor_label"), OutParams.ActorLabel))
	{
		JsonObject->TryGetStringField(TEXT("actor_name"), OutParams.ActorLabel);
	}

	// Optional: generation_trigger
	JsonObject->TryGetStringField(TEXT("generation_trigger"), OutParams.GenerationTrigger);

	// Optional: volume_extents [X, Y, Z] — half-extents of the bounds box in UU
	if (JsonObject->HasField(TEXT("volume_extents")))
	{
		const TArray<TSharedPtr<FJsonValue>>* ExtentsArray;
		if (JsonObject->TryGetArrayField(TEXT("volume_extents"), ExtentsArray) && ExtentsArray->Num() >= 3)
		{
			OutParams.VolumeExtents.X = (*ExtentsArray)[0]->AsNumber();
			OutParams.VolumeExtents.Y = (*ExtentsArray)[1]->AsNumber();
			OutParams.VolumeExtents.Z = (*ExtentsArray)[2]->AsNumber();
		}
	}

	return true;
}

FString FSpawnPCGActorCommand::CreateSuccessResponse(const FString& ActorName, const FString& ActorPath, const FString& ComponentName) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("actor_name"), ActorName);
	ResponseObj->SetStringField(TEXT("actor_path"), ActorPath);
	ResponseObj->SetStringField(TEXT("component_name"), ComponentName);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

	return OutputString;
}

FString FSpawnPCGActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

	return OutputString;
}
