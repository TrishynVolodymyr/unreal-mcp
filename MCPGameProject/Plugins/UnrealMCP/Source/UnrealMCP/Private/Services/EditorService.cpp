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
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"

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

    // First try to match by actor label (display name in World Outliner)
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetActorLabel() == ActorName)
        {
            return Actor;
        }
    }

    // Fallback: try to match by internal FName
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
    
    return nullptr;
}

AActor* FEditorService::SpawnActorOfType(UClass* ActorClass, const FString& Name, const FVector& Location, const FRotator& Rotation, const FVector& Scale, FString& OutError)
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
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *Name;
    
    AActor* NewActor = World->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParams);
    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);
        
        return NewActor;
    }
    
    OutError = TEXT("Failed to spawn actor");
    return nullptr;
}

AActor* FEditorService::SpawnActor(const FActorSpawnParams& Params, FString& OutError)
{
    UClass* ActorClass = GetActorClassFromType(Params.Type);
    if (!ActorClass)
    {
        OutError = FString::Printf(TEXT("Unknown actor type: %s"), *Params.Type);
        return nullptr;
    }
    
    return SpawnActorOfType(ActorClass, Params.Name, Params.Location, Params.Rotation, Params.Scale, OutError);
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

    // Set the actor label (display name in World Outliner) to the requested name
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
