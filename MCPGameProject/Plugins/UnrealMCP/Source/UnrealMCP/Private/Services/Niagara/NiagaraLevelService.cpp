// NiagaraLevelService.cpp - Level Integration (Feature 6)
// SpawnActor

#include "Services/NiagaraService.h"

#include "Editor.h"
#include "EngineUtils.h"  // For TActorIterator
#include "NiagaraSystem.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"

// ============================================================================
// Level Integration (Feature 6)
// ============================================================================

ANiagaraActor* FNiagaraService::SpawnActor(const FNiagaraActorSpawnParams& Params, FString& OutActorName, FString& OutError)
{
    // Validate params
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Get editor world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No valid editor world");
        return nullptr;
    }

    // Find the system
    UNiagaraSystem* System = FindSystem(Params.SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("Niagara System not found: %s"), *Params.SystemPath);
        return nullptr;
    }

    // Check if actor with same name already exists
    FName RequestedName(*Params.ActorName);
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetFName() == RequestedName || It->GetActorLabel() == Params.ActorName)
        {
            OutError = FString::Printf(TEXT("Actor with name '%s' already exists. Delete it first or use a different name."), *Params.ActorName);
            return nullptr;
        }
    }

    // Spawn the actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = RequestedName;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ANiagaraActor* NiagaraActor = World->SpawnActor<ANiagaraActor>(
        ANiagaraActor::StaticClass(),
        Params.Location,
        Params.Rotation,
        SpawnParams
    );

    if (!NiagaraActor)
    {
        OutError = TEXT("Failed to spawn Niagara Actor");
        return nullptr;
    }

    // Set the system asset
    UNiagaraComponent* NiagaraComponent = NiagaraActor->GetNiagaraComponent();
    if (NiagaraComponent)
    {
        NiagaraComponent->SetAsset(System);
        NiagaraComponent->SetAutoActivate(Params.bAutoActivate);

        if (Params.bAutoActivate)
        {
            NiagaraComponent->Activate(true);
        }
    }

    // Set actor label
    NiagaraActor->SetActorLabel(Params.ActorName);
    OutActorName = NiagaraActor->GetActorLabel();

    UE_LOG(LogNiagaraService, Log, TEXT("Spawned Niagara Actor '%s' with system '%s' at (%f, %f, %f)"),
        *OutActorName, *Params.SystemPath,
        Params.Location.X, Params.Location.Y, Params.Location.Z);

    return NiagaraActor;
}
