#include "Factories/ComponentFactory.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
// #include "Components/ParticleSystemComponent.h" // Not available in UE 5.7
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
// #include "Components/MovementComponent.h" // Not available in UE 5.7
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SplineComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/Blueprint.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"

FComponentFactory& FComponentFactory::Get()
{
    static FComponentFactory Instance;
    return Instance;
}

void FComponentFactory::RegisterComponentType(const FString& TypeName, UClass* ComponentClass)
{
    if (!ComponentClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentFactory::RegisterComponentType: Attempted to register null ComponentClass for type '%s'"), *TypeName);
        return;
    }

    if (!ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentFactory::RegisterComponentType: Class '%s' is not a valid ActorComponent"), *ComponentClass->GetName());
        return;
    }

    FScopeLock Lock(&ComponentMapLock);
    ComponentTypeMap.Add(TypeName, ComponentClass);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentFactory: Registered component type '%s' -> '%s'"), *TypeName, *ComponentClass->GetName());
}

UClass* FComponentFactory::GetComponentClass(const FString& TypeName) const
{
    FScopeLock Lock(&ComponentMapLock);
    
    // Ensure default types are initialized
    if (!bDefaultTypesInitialized)
    {
        // Cast away const to initialize - this is safe as we're using a lock
        const_cast<FComponentFactory*>(this)->InitializeDefaultTypes();
    }

    if (UClass* const* FoundClass = ComponentTypeMap.Find(TypeName))
    {
        return *FoundClass;
    }

    // If not found in registry, try to load as Blueprint component
    // Support both short names (BP_MyComponent) and full paths (/Game/Path/BP_MyComponent)
    UClass* LoadedClass = nullptr;
    
    if (TypeName.StartsWith(TEXT("/Game/")) || TypeName.StartsWith(TEXT("/Script/")))
    {
        // Full path provided - try to load as Blueprint class
        FString ClassPath = TypeName;
        if (!ClassPath.EndsWith(TEXT("_C")))
        {
            // Add _C suffix for generated Blueprint class
            FString BaseName = FPaths::GetBaseFilename(TypeName);
            ClassPath = FString::Printf(TEXT("%s.%s_C"), *TypeName, *BaseName);
        }
        
        LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
        
        if (!LoadedClass)
        {
            // Try loading as Blueprint asset
            UObject* Asset = LoadObject<UObject>(nullptr, *TypeName);
            if (UBlueprint* BP = Cast<UBlueprint>(Asset))
            {
                LoadedClass = BP->GeneratedClass;
            }
        }
    }
    else
    {
        // Short name provided - search for Blueprint component
        // Fix: Ensure TypeName doesn't already start with /Game/ or Game/
        FString CleanTypeName = TypeName;
        if (CleanTypeName.StartsWith(TEXT("/Game/")))
        {
            // Already a full path - extract just the asset name
            CleanTypeName = FPaths::GetBaseFilename(TypeName);
        }
        else if (CleanTypeName.StartsWith(TEXT("Game/")))
        {
            // Has Game/ prefix without leading slash - extract asset name
            CleanTypeName = FPaths::GetBaseFilename(TypeName);
        }
        
        // Try common component paths
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/Blueprints/%s"), *CleanTypeName),
            FString::Printf(TEXT("/Game/Components/%s"), *CleanTypeName),
            FString::Printf(TEXT("/Game/%s"), *CleanTypeName)
        };
        
        for (const FString& SearchPath : SearchPaths)
        {
            FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *SearchPath, *CleanTypeName);
            LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
            
            if (!LoadedClass)
            {
                UObject* Asset = LoadObject<UObject>(nullptr, *SearchPath);
                if (UBlueprint* BP = Cast<UBlueprint>(Asset))
                {
                    LoadedClass = BP->GeneratedClass;
                }
            }
            
            if (LoadedClass)
            {
                break;
            }
        }
    }
    
    // Validate that loaded class is an ActorComponent
    if (LoadedClass && LoadedClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogTemp, Log, TEXT("FComponentFactory::GetComponentClass: Loaded Blueprint component class '%s' for type '%s'"), 
               *LoadedClass->GetName(), *TypeName);
        return LoadedClass;
    }
    
    if (LoadedClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentFactory::GetComponentClass: Loaded class '%s' is not an ActorComponent"), 
               *LoadedClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentFactory::GetComponentClass: Component type '%s' not found"), *TypeName);
    }
    
    return nullptr;
}

TArray<FString> FComponentFactory::GetAvailableTypes() const
{
    FScopeLock Lock(&ComponentMapLock);
    
    // Ensure default types are initialized
    if (!bDefaultTypesInitialized)
    {
        // Cast away const to initialize - this is safe as we're using a lock
        const_cast<FComponentFactory*>(this)->InitializeDefaultTypes();
    }

    TArray<FString> AvailableTypes;
    ComponentTypeMap.GetKeys(AvailableTypes);
    
    // Sort alphabetically for consistent output
    AvailableTypes.Sort();
    
    return AvailableTypes;
}

void FComponentFactory::InitializeDefaultTypes()
{
    FScopeLock Lock(&ComponentMapLock);
    
    if (bDefaultTypesInitialized)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("FComponentFactory: Initializing default component types"));

    // Scene Components
    RegisterComponentType(TEXT("SceneComponent"), USceneComponent::StaticClass());
    
    // Mesh Components
    RegisterComponentType(TEXT("StaticMeshComponent"), UStaticMeshComponent::StaticClass());
    RegisterComponentType(TEXT("SkeletalMeshComponent"), USkeletalMeshComponent::StaticClass());
    
    // Light Components
    RegisterComponentType(TEXT("PointLightComponent"), UPointLightComponent::StaticClass());
    RegisterComponentType(TEXT("SpotLightComponent"), USpotLightComponent::StaticClass());
    RegisterComponentType(TEXT("DirectionalLightComponent"), UDirectionalLightComponent::StaticClass());
    
    // Collision Components
    RegisterComponentType(TEXT("SphereComponent"), USphereComponent::StaticClass());
    RegisterComponentType(TEXT("BoxComponent"), UBoxComponent::StaticClass());
    RegisterComponentType(TEXT("CapsuleComponent"), UCapsuleComponent::StaticClass());
    
    // Audio Components
    RegisterComponentType(TEXT("AudioComponent"), UAudioComponent::StaticClass());
    
    // Particle Components (UParticleSystemComponent not available in UE 5.7)
    // RegisterComponentType(TEXT("ParticleSystemComponent"), UParticleSystemComponent::StaticClass());
    
    // Camera Components
    RegisterComponentType(TEXT("CameraComponent"), UCameraComponent::StaticClass());
    
    // Movement Components
    RegisterComponentType(TEXT("CharacterMovementComponent"), UCharacterMovementComponent::StaticClass());
    RegisterComponentType(TEXT("FloatingPawnMovement"), UFloatingPawnMovement::StaticClass());
    RegisterComponentType(TEXT("ProjectileMovementComponent"), UProjectileMovementComponent::StaticClass());
    RegisterComponentType(TEXT("RotatingMovementComponent"), URotatingMovementComponent::StaticClass());
    
    // UI Components
    RegisterComponentType(TEXT("WidgetComponent"), UWidgetComponent::StaticClass());
    
    // Other Components
    RegisterComponentType(TEXT("DecalComponent"), UDecalComponent::StaticClass());
    RegisterComponentType(TEXT("SplineComponent"), USplineComponent::StaticClass());
    RegisterComponentType(TEXT("TimelineComponent"), UTimelineComponent::StaticClass());
    RegisterComponentType(TEXT("InputComponent"), UInputComponent::StaticClass());

    bDefaultTypesInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("FComponentFactory: Initialized %d default component types"), ComponentTypeMap.Num());
}