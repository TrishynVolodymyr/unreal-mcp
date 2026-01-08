#include "Services/EditorService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"

// Basic Actors
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"

// Volumes/BSP
#include "Engine/TriggerBox.h"
#include "Engine/TriggerSphere.h"
#include "Engine/TriggerCapsule.h"
#include "Engine/BlockingVolume.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/PhysicsVolume.h"
#include "Sound/AudioVolume.h"
#include "Engine/PostProcessVolume.h"
#include "Lightmass/LightmassImportanceVolume.h"
#include "GameFramework/KillZVolume.h"
#include "GameFramework/PainCausingVolume.h"

// Utility Actors
#include "Engine/TextRenderActor.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/TargetPoint.h"
#include "Engine/DecalActor.h"
#include "Engine/Note.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/SphereReflectionCapture.h"
#include "Engine/BoxReflectionCapture.h"

// Components for configuration
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"

#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"

TUniquePtr<FEditorService> FEditorService::Instance = nullptr;

FEditorService::FEditorService()
{
}

FEditorService& FEditorService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FEditorService>();
    }
    return *Instance;
}

UWorld* FEditorService::GetEditorWorld() const
{
    if (GEditor)
    {
        return GEditor->GetEditorWorldContext().World();
    }
    return nullptr;
}

TArray<AActor*> FEditorService::GetActorsInLevel()
{
    TArray<AActor*> AllActors;
    UWorld* World = GetEditorWorld();
    if (World)
    {
        UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    }
    return AllActors;
}

TArray<AActor*> FEditorService::FindActorsByName(const FString& Pattern)
{
    TArray<AActor*> AllActors = GetActorsInLevel();
    TArray<AActor*> MatchingActors;
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().MatchesWildcard(Pattern))
        {
            MatchingActors.Add(Actor);
        }
    }
    
    return MatchingActors;
}

AActor* FEditorService::FindActorByName(const FString& ActorName)
{
    TArray<AActor*> AllActors = GetActorsInLevel();
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return Actor;
        }
    }
    
    return nullptr;
}

UClass* FEditorService::GetActorClassFromType(const FString& TypeString) const
{
    // ========================================
    // FRIENDLY NAME ALIASES (Common Types)
    // ========================================

    // Basic Actors
    if (TypeString == TEXT("StaticMeshActor"))
    {
        return AStaticMeshActor::StaticClass();
    }
    else if (TypeString == TEXT("PointLight"))
    {
        return APointLight::StaticClass();
    }
    else if (TypeString == TEXT("SpotLight"))
    {
        return ASpotLight::StaticClass();
    }
    else if (TypeString == TEXT("DirectionalLight"))
    {
        return ADirectionalLight::StaticClass();
    }
    else if (TypeString == TEXT("CameraActor"))
    {
        return ACameraActor::StaticClass();
    }

    // Volumes/BSP
    else if (TypeString == TEXT("TriggerBox"))
    {
        return ATriggerBox::StaticClass();
    }
    else if (TypeString == TEXT("TriggerSphere"))
    {
        return ATriggerSphere::StaticClass();
    }
    else if (TypeString == TEXT("TriggerCapsule"))
    {
        return ATriggerCapsule::StaticClass();
    }
    else if (TypeString == TEXT("BlockingVolume"))
    {
        return ABlockingVolume::StaticClass();
    }
    else if (TypeString == TEXT("NavMeshBoundsVolume"))
    {
        return ANavMeshBoundsVolume::StaticClass();
    }
    else if (TypeString == TEXT("PhysicsVolume"))
    {
        return APhysicsVolume::StaticClass();
    }
    else if (TypeString == TEXT("AudioVolume"))
    {
        return AAudioVolume::StaticClass();
    }
    else if (TypeString == TEXT("PostProcessVolume"))
    {
        return APostProcessVolume::StaticClass();
    }
    else if (TypeString == TEXT("LightmassImportanceVolume"))
    {
        return ALightmassImportanceVolume::StaticClass();
    }
    else if (TypeString == TEXT("KillZVolume"))
    {
        return AKillZVolume::StaticClass();
    }
    else if (TypeString == TEXT("PainCausingVolume"))
    {
        return APainCausingVolume::StaticClass();
    }

    // Utility Actors
    else if (TypeString == TEXT("TextRenderActor"))
    {
        return ATextRenderActor::StaticClass();
    }
    else if (TypeString == TEXT("PlayerStart"))
    {
        return APlayerStart::StaticClass();
    }
    else if (TypeString == TEXT("TargetPoint"))
    {
        return ATargetPoint::StaticClass();
    }
    else if (TypeString == TEXT("DecalActor"))
    {
        return ADecalActor::StaticClass();
    }
    else if (TypeString == TEXT("Note"))
    {
        return ANote::StaticClass();
    }
    else if (TypeString == TEXT("ExponentialHeightFog"))
    {
        return AExponentialHeightFog::StaticClass();
    }
    else if (TypeString == TEXT("SkyLight"))
    {
        return ASkyLight::StaticClass();
    }
    else if (TypeString == TEXT("SphereReflectionCapture"))
    {
        return ASphereReflectionCapture::StaticClass();
    }
    else if (TypeString == TEXT("BoxReflectionCapture"))
    {
        return ABoxReflectionCapture::StaticClass();
    }

    // ========================================
    // SPECIAL TYPES
    // ========================================
    else if (TypeString == TEXT("InvisibleWall"))
    {
        // InvisibleWall uses StaticMeshActor with special configuration
        return AStaticMeshActor::StaticClass();
    }

    // ========================================
    // GENERIC CLASS PATH: "Class:/Script/Module.ClassName"
    // Allows spawning ANY native UClass without code changes
    // ========================================
    else if (TypeString.StartsWith(TEXT("Class:")))
    {
        FString ClassPath = TypeString.Mid(6); // Remove "Class:" prefix
        UClass* LoadedClass = LoadClass<AActor>(nullptr, *ClassPath);
        if (LoadedClass && LoadedClass->IsChildOf(AActor::StaticClass()))
        {
            return LoadedClass;
        }
        return nullptr;
    }

    // ========================================
    // BLUEPRINT: "Blueprint:/Game/Path/BP_Name"
    // ========================================
    else if (TypeString.StartsWith(TEXT("Blueprint:")))
    {
        FString BlueprintPath = TypeString.Mid(10); // Remove "Blueprint:" prefix
        UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
        if (Blueprint && Blueprint->GeneratedClass)
        {
            return Blueprint->GeneratedClass;
        }
        return nullptr;
    }

    // ========================================
    // FALLBACK: Try as Blueprint path (for convenience)
    // ========================================
    else if (TypeString.StartsWith(TEXT("/Game/")) || TypeString.StartsWith(TEXT("/Script/")))
    {
        // Try as Blueprint first
        UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(TypeString);
        if (Blueprint && Blueprint->GeneratedClass)
        {
            return Blueprint->GeneratedClass;
        }
        // Try as native class
        UClass* LoadedClass = LoadClass<AActor>(nullptr, *TypeString);
        if (LoadedClass)
        {
            return LoadedClass;
        }
    }

    return nullptr;
}

AActor* FEditorService::SpawnActorOfType(UClass* ActorClass, const FString& Name, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FActorSpawnParams& Params, FString& OutError)
{
    UWorld* World = GetEditorWorld();
    if (!World)
    {
        OutError = TEXT("Failed to get editor world");
        return nullptr;
    }

    // Check if an actor with this name already exists
    if (FindActorByName(Name))
    {
        OutError = FString::Printf(TEXT("Actor with name '%s' already exists"), *Name);
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Name = *Name;

    AActor* NewActor = World->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParameters);
    if (NewActor)
    {
        // Set the actor label (editor-visible name)
        NewActor->SetActorLabel(Name);

        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Apply type-specific configuration
        ConfigureSpawnedActor(NewActor, Params);

        return NewActor;
    }

    OutError = TEXT("Failed to spawn actor");
    return nullptr;
}

void FEditorService::ConfigureSpawnedActor(AActor* NewActor, const FActorSpawnParams& Params)
{
    if (!NewActor)
    {
        return;
    }

    // ========================================
    // StaticMeshActor - assign mesh if provided
    // ========================================
    if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor))
    {
        UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();

        if (!Params.MeshPath.IsEmpty() && MeshComp)
        {
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Params.MeshPath);
            if (Mesh)
            {
                MeshComp->SetStaticMesh(Mesh);
                MeshComp->SetMobility(EComponentMobility::Movable);
            }
        }

        // Apply collision settings (useful for InvisibleWall type)
        if (Params.bBlocksAll && MeshComp)
        {
            // Set collision to block all
            MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
            MeshComp->SetGenerateOverlapEvents(false);
        }

        // Apply visibility settings (useful for InvisibleWall type)
        if (Params.bHiddenInGame)
        {
            // Hide in game but keep visible in editor
            // SetActorHiddenInGame only affects gameplay, mesh remains visible in editor viewport
            MeshActor->SetActorHiddenInGame(true);

            if (MeshComp)
            {
                // Keep collision enabled for blocking
                MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                // Note: Do NOT call SetVisibility(false) here - that hides in editor too
                // SetActorHiddenInGame is sufficient for PIE/game hiding
            }
        }

        // Editor visibility options for invisible walls
        if (Params.bShowCollisionInEditor && MeshComp)
        {
            // Enable collision visualization in editor
            MeshComp->bVisualizeComponent = true;

            // Keep the mesh visible only in editor (wireframe in editor)
            // This is controlled by SetHiddenInGame above - editor will still show it
            // but we can also set a wireframe-like appearance
            #if WITH_EDITOR
            MeshComp->SetRenderCustomDepth(true);
            MeshComp->SetCustomDepthStencilValue(1);
            #endif
        }
    }
    // ========================================
    // TextRenderActor - set text properties
    // ========================================
    else if (ATextRenderActor* TextActor = Cast<ATextRenderActor>(NewActor))
    {
        UTextRenderComponent* TextComp = TextActor->GetTextRender();
        if (!TextComp)
        {
            // Fallback: find by class if GetTextRender() returns null
            TextComp = TextActor->FindComponentByClass<UTextRenderComponent>();
        }

        if (TextComp)
        {
            // Set text content
            if (!Params.TextContent.IsEmpty())
            {
                TextComp->SetText(FText::FromString(Params.TextContent));
            }

            // Set text size
            TextComp->SetWorldSize(Params.TextSize);

            // Set text color
            TextComp->SetTextRenderColor(Params.TextColor.ToFColor(true));

            // Convert int32 alignment values to enums
            EHorizTextAligment HAlign = EHTA_Center;
            if (Params.TextHAlign == 0) HAlign = EHTA_Left;
            else if (Params.TextHAlign == 2) HAlign = EHTA_Right;
            TextComp->SetHorizontalAlignment(HAlign);

            EVerticalTextAligment VAlign = EVRTA_TextCenter;
            if (Params.TextVAlign == 0) VAlign = EVRTA_TextTop;
            else if (Params.TextVAlign == 2) VAlign = EVRTA_TextBottom;
            TextComp->SetVerticalAlignment(VAlign);

            // Mark component as needing update
            TextComp->MarkRenderStateDirty();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnActor: TextRenderActor '%s' has no TextRenderComponent"), *NewActor->GetName());
        }
    }
    // ========================================
    // TriggerBox - set box extent
    // ========================================
    else if (ATriggerBox* TriggerBox = Cast<ATriggerBox>(NewActor))
    {
        if (UBoxComponent* BoxComp = Cast<UBoxComponent>(TriggerBox->GetCollisionComponent()))
        {
            BoxComp->SetBoxExtent(Params.BoxExtent);
        }
    }
    // ========================================
    // TriggerSphere - set sphere radius
    // ========================================
    else if (ATriggerSphere* TriggerSphere = Cast<ATriggerSphere>(NewActor))
    {
        if (USphereComponent* SphereComp = Cast<USphereComponent>(TriggerSphere->GetCollisionComponent()))
        {
            SphereComp->SetSphereRadius(Params.SphereRadius);
        }
    }
    // ========================================
    // TriggerCapsule - set capsule size
    // ========================================
    else if (ATriggerCapsule* TriggerCapsule = Cast<ATriggerCapsule>(NewActor))
    {
        if (UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(TriggerCapsule->GetCollisionComponent()))
        {
            // Use BoxExtent X for radius and Z for half-height
            CapsuleComp->SetCapsuleSize(Params.BoxExtent.X, Params.BoxExtent.Z);
        }
    }
    // ========================================
    // PlayerStart - set tag
    // ========================================
    else if (APlayerStart* PlayerStart = Cast<APlayerStart>(NewActor))
    {
        if (!Params.PlayerStartTag.IsEmpty())
        {
            PlayerStart->PlayerStartTag = FName(*Params.PlayerStartTag);
        }
    }
    // ========================================
    // DecalActor - set size and material
    // ========================================
    else if (ADecalActor* DecalActorPtr = Cast<ADecalActor>(NewActor))
    {
        if (UDecalComponent* DecalComp = DecalActorPtr->GetDecal())
        {
            DecalComp->DecalSize = Params.DecalSize;
            if (!Params.DecalMaterialPath.IsEmpty())
            {
                UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *Params.DecalMaterialPath);
                if (Mat)
                {
                    DecalComp->SetDecalMaterial(Mat);
                }
            }
        }
    }
    // ========================================
    // PostProcessVolume - enable by default
    // ========================================
    else if (APostProcessVolume* PPVolume = Cast<APostProcessVolume>(NewActor))
    {
        PPVolume->bEnabled = true;
        PPVolume->bUnbound = false; // Bounded by default
    }
}

AActor* FEditorService::SpawnActor(const FActorSpawnParams& Params, FString& OutError)
{
    UClass* ActorClass = GetActorClassFromType(Params.Type);
    if (!ActorClass)
    {
        OutError = FString::Printf(TEXT("Unknown actor type: %s. Supported types include StaticMeshActor, TriggerBox, PlayerStart, InvisibleWall, etc. Use 'Blueprint:/Game/Path' for Blueprints or 'Class:/Script/Module.ClassName' for any native class."), *Params.Type);
        return nullptr;
    }

    // Create a mutable copy of params to apply defaults for special types
    FActorSpawnParams ModifiedParams = Params;

    // InvisibleWall special type auto-configuration
    if (Params.Type == TEXT("InvisibleWall"))
    {
        // Auto-set mesh to cube if not specified
        if (ModifiedParams.MeshPath.IsEmpty())
        {
            ModifiedParams.MeshPath = TEXT("/Engine/BasicShapes/Cube");
        }
        // Auto-enable hidden in game
        ModifiedParams.bHiddenInGame = true;
        // Auto-enable BlockAll collision
        ModifiedParams.bBlocksAll = true;
        // Show collision in editor for visibility
        ModifiedParams.bShowCollisionInEditor = true;
    }

    return SpawnActorOfType(ActorClass, ModifiedParams.Name, ModifiedParams.Location, ModifiedParams.Rotation, ModifiedParams.Scale, ModifiedParams, OutError);
}

AActor* FEditorService::SpawnBlueprintActor(const FBlueprintActorSpawnParams& Params, FString& OutError)
{
    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(Params.BlueprintName);
    if (!Blueprint)
    {
        OutError = FString::Printf(TEXT("Blueprint not found: %s"), *Params.BlueprintName);
        return nullptr;
    }
    
    UWorld* World = GetEditorWorld();
    if (!World)
    {
        OutError = TEXT("Failed to get editor world");
        return nullptr;
    }
    
    // Check if an actor with this name already exists
    if (FindActorByName(Params.ActorName))
    {
        OutError = FString::Printf(TEXT("Actor with name '%s' already exists"), *Params.ActorName);
        return nullptr;
    }

    // Validate GeneratedClass exists (Blueprint might be in error state)
    if (!Blueprint->GeneratedClass)
    {
        OutError = FString::Printf(TEXT("Blueprint '%s' has no GeneratedClass - it may need to be compiled"), *Params.BlueprintName);
        return nullptr;
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Params.Location);
    SpawnTransform.SetRotation(Params.Rotation.Quaternion());
    SpawnTransform.SetScale3D(Params.Scale);
    
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Name = FName(*Params.ActorName);
    SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParameters);
    if (!NewActor)
    {
        OutError = TEXT("Failed to spawn blueprint actor");
        return nullptr;
    }

    // Set the actor label (editor-visible name)
    NewActor->SetActorLabel(Params.ActorName);

    return NewActor;
}

bool FEditorService::DeleteActor(const FString& ActorName, FString& OutError)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorName);
        return false;
    }

    // Rename actor before destroying to free up the name immediately
    // Destroy() is asynchronous, so without renaming, the name remains in use
    // until garbage collection runs, causing "Cannot generate unique name" crashes
    FString TempName = FString::Printf(TEXT("PendingDelete_%s_%d"), *ActorName, FMath::Rand());
    Actor->Rename(*TempName);
    Actor->SetActorLabel(TempName);

    Actor->Destroy();
    return true;
}

bool FEditorService::SetActorTransform(AActor* Actor, const FVector* Location, const FRotator* Rotation, const FVector* Scale)
{
    if (!Actor)
    {
        return false;
    }
    
    FTransform NewTransform = Actor->GetTransform();
    
    if (Location)
    {
        NewTransform.SetLocation(*Location);
    }
    if (Rotation)
    {
        NewTransform.SetRotation(FQuat(*Rotation));
    }
    if (Scale)
    {
        NewTransform.SetScale3D(*Scale);
    }
    
    Actor->SetActorTransform(NewTransform);
    return true;
}

bool FEditorService::SetActorProperty(AActor* Actor, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    if (!Actor)
    {
        OutError = TEXT("Invalid actor");
        return false;
    }
    
    return FUnrealMCPCommonUtils::SetObjectProperty(Actor, PropertyName, PropertyValue, OutError);
}

bool FEditorService::SetLightProperty(AActor* Actor, const FString& PropertyName, const FString& PropertyValue, FString& OutError)
{
    if (!Actor)
    {
        OutError = TEXT("Invalid actor");
        return false;
    }
    
    // Find the light component
    ULightComponent* LightComponent = nullptr;
    
    // Check if it's one of the built-in light types
    if (APointLight* PointLight = Cast<APointLight>(Actor))
    {
        LightComponent = PointLight->GetLightComponent();
    }
    else if (ASpotLight* SpotLight = Cast<ASpotLight>(Actor))
    {
        LightComponent = SpotLight->GetLightComponent();
    }
    else if (ADirectionalLight* DirLight = Cast<ADirectionalLight>(Actor))
    {
        LightComponent = DirLight->GetLightComponent();
    }
    else
    {
        // Try to find any light component in the actor
        LightComponent = Actor->FindComponentByClass<ULightComponent>();
    }
    
    if (!LightComponent)
    {
        OutError = FString::Printf(TEXT("Cannot find light component on actor: %s"), *Actor->GetName());
        return false;
    }
    
    // Set the property on the light component
    if (PropertyName == TEXT("Intensity"))
    {
        float Value = FCString::Atof(*PropertyValue);
        LightComponent->SetIntensity(Value);
    }
    else if (PropertyName == TEXT("LightColor"))
    {
        TArray<FString> ColorValues;
        PropertyValue.ParseIntoArray(ColorValues, TEXT(","), true);
        
        if (ColorValues.Num() >= 3)
        {
            float R = FCString::Atof(*ColorValues[0]);
            float G = FCString::Atof(*ColorValues[1]);
            float B = FCString::Atof(*ColorValues[2]);
            
            LightComponent->SetLightColor(FLinearColor(R, G, B));
        }
        else
        {
            OutError = TEXT("Invalid color format. Expected R,G,B values.");
            return false;
        }
    }
    else if (PropertyName == TEXT("AttenuationRadius"))
    {
        float Value = FCString::Atof(*PropertyValue);
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
        {
            PointLightComp->AttenuationRadius = Value;
            PointLightComp->MarkRenderStateDirty();
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
        {
            SpotLightComp->AttenuationRadius = Value;
            SpotLightComp->MarkRenderStateDirty();
        }
        else
        {
            OutError = TEXT("AttenuationRadius is only applicable for point and spot lights");
            return false;
        }
    }
    else if (PropertyName == TEXT("SourceRadius"))
    {
        float Value = FCString::Atof(*PropertyValue);
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
        {
            PointLightComp->SourceRadius = Value;
            PointLightComp->MarkRenderStateDirty();
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
        {
            SpotLightComp->SourceRadius = Value;
            SpotLightComp->MarkRenderStateDirty();
        }
        else
        {
            OutError = TEXT("SourceRadius is only applicable for point and spot lights");
            return false;
        }
    }
    else if (PropertyName == TEXT("SoftSourceRadius"))
    {
        float Value = FCString::Atof(*PropertyValue);
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
        {
            PointLightComp->SoftSourceRadius = Value;
            PointLightComp->MarkRenderStateDirty();
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
        {
            SpotLightComp->SoftSourceRadius = Value;
            SpotLightComp->MarkRenderStateDirty();
        }
        else
        {
            OutError = TEXT("SoftSourceRadius is only applicable for point and spot lights");
            return false;
        }
    }
    else if (PropertyName == TEXT("CastShadows"))
    {
        bool Value = PropertyValue.ToBool();
        LightComponent->SetCastShadows(Value);
    }
    else
    {
        OutError = FString::Printf(TEXT("Unknown light property: %s"), *PropertyName);
        return false;
    }
    
    // Mark the component as modified
    LightComponent->MarkPackageDirty();
    return true;
}

bool FEditorService::FocusViewport(AActor* TargetActor, const FVector* Location, float Distance, const FRotator* Orientation, FString* OutError)
{
    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        if (OutError)
        {
            *OutError = TEXT("Failed to get active viewport");
        }
        return false;
    }
    
    // If we have a target actor, focus on it
    if (TargetActor)
    {
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (Location)
    {
        ViewportClient->SetViewLocation(*Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        if (OutError)
        {
            *OutError = TEXT("Either target actor or location must be provided");
        }
        return false;
    }
    
    // Set orientation if provided
    if (Orientation)
    {
        ViewportClient->SetViewRotation(*Orientation);
    }
    
    // Force viewport to redraw
    ViewportClient->Invalidate();
    return true;
}

bool FEditorService::TakeScreenshot(const FString& FilePath, FString& OutError)
{
    FString ActualFilePath = FilePath;
    
    // Ensure the file path has a proper extension
    if (!ActualFilePath.EndsWith(TEXT(".png")))
    {
        ActualFilePath += TEXT(".png");
    }
    
    // Get the active viewport
    if (GEditor && GEditor->GetActiveViewport())
    {
        FViewport* Viewport = GEditor->GetActiveViewport();
        TArray<FColor> Bitmap;
        FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
        
        if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
        {
            TArray<uint8> CompressedBitmap;
            FImageUtils::CompressImageArray(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Bitmap, CompressedBitmap);
            
            if (FFileHelper::SaveArrayToFile(CompressedBitmap, *ActualFilePath))
            {
                return true;
            }
        }
    }
    
    OutError = TEXT("Failed to take screenshot");
    return false;
}

TArray<FString> FEditorService::FindAssetsByType(const FString& AssetType, const FString& SearchPath)
{
    return FUnrealMCPCommonUtils::FindAssetsByType(AssetType, SearchPath);
}

TArray<FString> FEditorService::FindAssetsByName(const FString& AssetName, const FString& SearchPath)
{
    return FUnrealMCPCommonUtils::FindAssetsByName(AssetName, SearchPath);
}

TArray<FString> FEditorService::FindWidgetBlueprints(const FString& WidgetName, const FString& SearchPath)
{
    return FUnrealMCPCommonUtils::FindWidgetBlueprints(WidgetName, SearchPath);
}

TArray<FString> FEditorService::FindBlueprints(const FString& BlueprintName, const FString& SearchPath)
{
    return FUnrealMCPCommonUtils::FindBlueprints(BlueprintName, SearchPath);
}

TArray<FString> FEditorService::FindDataTables(const FString& TableName, const FString& SearchPath)
{
    return FUnrealMCPCommonUtils::FindDataTables(TableName, SearchPath);
}
