#include "Services/ComponentService.h"
#include "Services/IPropertyService.h"
#include "Services/PropertyService.h"
#include "Services/AssetDiscoveryService.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "GameFramework/Actor.h"

// Component Type Cache Implementation
UClass* FComponentTypeCache::GetComponentClass(const FString& ComponentType)
{
    FScopeLock Lock(&CacheLock);
    
    // Update statistics
    UpdateStats(false); // Assume miss initially
    
    if (TWeakObjectPtr<UClass>* CachedPtr = CachedComponentClasses.Find(ComponentType))
    {
        if (CachedPtr->IsValid())
        {
            // Update to hit since we found a valid entry
            CacheStats.CacheHits++;
            CacheStats.CacheMisses--;
            
            UE_LOG(LogTemp, Verbose, TEXT("FComponentTypeCache: Cache hit for component type '%s'"), *ComponentType);
            return CachedPtr->Get();
        }
        else
        {
            // Remove invalid entry
            CachedComponentClasses.Remove(ComponentType);
            CacheStats.CachedCount = CachedComponentClasses.Num();
            UE_LOG(LogTemp, Verbose, TEXT("FComponentTypeCache: Removed invalid cache entry for component type '%s'"), *ComponentType);
        }
    }
    
    // Try lazy loading
    UClass* LoadedClass = LazyLoadComponentClass(ComponentType);
    if (LoadedClass)
    {
        // Cache the loaded class
        CachedComponentClasses.Add(ComponentType, LoadedClass);
        CacheStats.CachedCount = CachedComponentClasses.Num();
        
        // Update to hit since we successfully loaded
        CacheStats.CacheHits++;
        CacheStats.CacheMisses--;
        
        UE_LOG(LogTemp, Verbose, TEXT("FComponentTypeCache: Lazy loaded and cached component type '%s'"), *ComponentType);
    }
    
    return LoadedClass;
}

void FComponentTypeCache::CacheComponentClass(const FString& ComponentType, UClass* ComponentClass)
{
    if (!ComponentClass)
    {
        return;
    }
    
    FScopeLock Lock(&CacheLock);
    CachedComponentClasses.Add(ComponentType, ComponentClass);
    CacheStats.CachedCount = CachedComponentClasses.Num();
    UE_LOG(LogTemp, Verbose, TEXT("FComponentTypeCache: Cached component type '%s'"), *ComponentType);
}

void FComponentTypeCache::RefreshCache()
{
    FScopeLock Lock(&CacheLock);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache: Refreshing cache"));
    
    // Clear invalid entries
    int32 CleanedCount = 0;
    for (auto It = CachedComponentClasses.CreateIterator(); It; ++It)
    {
        if (!It.Value().IsValid())
        {
            It.RemoveCurrent();
            CleanedCount++;
        }
    }
    
    CacheStats.RefreshCount++;
    CacheStats.CachedCount = CachedComponentClasses.Num();
    bNeedsRefresh = false;
    
    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache: Cache refresh complete. Cleaned %d invalid entries. %d types cached"), 
        CleanedCount, CacheStats.CachedCount);
}

void FComponentTypeCache::PreloadCommonComponentTypes()
{
    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache: Preloading common component types"));
    
    // Common component types that are frequently used
    TArray<FString> CommonComponentTypes = {
        TEXT("StaticMeshComponent"),
        TEXT("PointLightComponent"),
        TEXT("SpotLightComponent"),
        TEXT("DirectionalLightComponent"),
        TEXT("BoxComponent"),
        TEXT("SphereComponent"),
        TEXT("CapsuleComponent"),
        TEXT("CameraComponent"),
        TEXT("AudioComponent"),
        TEXT("SceneComponent"),
        TEXT("BillboardComponent"),
        TEXT("StaticMesh"),
        TEXT("PointLight"),
        TEXT("SpotLight"),
        TEXT("DirectionalLight"),
        TEXT("Box"),
        TEXT("Sphere"),
        TEXT("Capsule"),
        TEXT("Camera"),
        TEXT("Audio"),
        TEXT("Scene"),
        TEXT("Billboard")
    };
    
    int32 PreloadedCount = 0;
    for (const FString& ComponentType : CommonComponentTypes)
    {
        if (!IsCached(ComponentType))
        {
            UClass* ComponentClass = GetComponentClass(ComponentType);
            if (ComponentClass)
            {
                PreloadedCount++;
                UE_LOG(LogTemp, Verbose, TEXT("FComponentTypeCache: Preloaded component type '%s'"), *ComponentType);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache: Preloading complete. %d component types preloaded"), PreloadedCount);
}

bool FComponentTypeCache::IsCached(const FString& ComponentType) const
{
    FScopeLock Lock(&CacheLock);
    
    if (const TWeakObjectPtr<UClass>* CachedPtr = CachedComponentClasses.Find(ComponentType))
    {
        return CachedPtr->IsValid();
    }
    
    return false;
}

void FComponentTypeCache::ClearCache()
{
    FScopeLock Lock(&CacheLock);
    int32 ClearedCount = CachedComponentClasses.Num();
    CachedComponentClasses.Empty();
    CacheStats.CachedCount = 0;
    bNeedsRefresh = false;
    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache: Cleared %d cached component types"), ClearedCount);
}

FComponentTypeCacheStats FComponentTypeCache::GetCacheStats() const
{
    FScopeLock Lock(&CacheLock);
    FComponentTypeCacheStats StatsCopy = CacheStats;
    StatsCopy.CachedCount = CachedComponentClasses.Num();
    return StatsCopy;
}

void FComponentTypeCache::ResetCacheStats()
{
    FScopeLock Lock(&CacheLock);
    CacheStats.Reset();
    CacheStats.CachedCount = CachedComponentClasses.Num();
    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache: Cache statistics reset"));
}

int32 FComponentTypeCache::GetCachedCount() const
{
    FScopeLock Lock(&CacheLock);
    return CachedComponentClasses.Num();
}

void FComponentTypeCache::UpdateStats(bool bWasHit) const
{
    // This method assumes the lock is already held
    CacheStats.TotalRequests++;
    if (bWasHit)
    {
        CacheStats.CacheHits++;
    }
    else
    {
        CacheStats.CacheMisses++;
    }
}

UClass* FComponentTypeCache::LazyLoadComponentClass(const FString& ComponentType)
{
    // This method assumes the lock is already held
    return ResolveComponentClassInternal(ComponentType);
}

TMap<FString, FString> FComponentTypeCache::GetSupportedComponentTypes() const
{
    static TMap<FString, FString> SupportedTypes;
    
    if (SupportedTypes.Num() == 0)
    {
        // Initialize supported component types mapping
        SupportedTypes.Add(TEXT("StaticMesh"), TEXT("StaticMeshComponent"));
        SupportedTypes.Add(TEXT("StaticMeshComponent"), TEXT("StaticMeshComponent"));
        SupportedTypes.Add(TEXT("PointLight"), TEXT("PointLightComponent"));
        SupportedTypes.Add(TEXT("PointLightComponent"), TEXT("PointLightComponent"));
        SupportedTypes.Add(TEXT("SpotLight"), TEXT("SpotLightComponent"));
        SupportedTypes.Add(TEXT("SpotLightComponent"), TEXT("SpotLightComponent"));
        SupportedTypes.Add(TEXT("DirectionalLight"), TEXT("DirectionalLightComponent"));
        SupportedTypes.Add(TEXT("DirectionalLightComponent"), TEXT("DirectionalLightComponent"));
        SupportedTypes.Add(TEXT("Box"), TEXT("BoxComponent"));
        SupportedTypes.Add(TEXT("BoxComponent"), TEXT("BoxComponent"));
        SupportedTypes.Add(TEXT("Sphere"), TEXT("SphereComponent"));
        SupportedTypes.Add(TEXT("SphereComponent"), TEXT("SphereComponent"));
        SupportedTypes.Add(TEXT("Capsule"), TEXT("CapsuleComponent"));
        SupportedTypes.Add(TEXT("CapsuleComponent"), TEXT("CapsuleComponent"));
        SupportedTypes.Add(TEXT("Camera"), TEXT("CameraComponent"));
        SupportedTypes.Add(TEXT("CameraComponent"), TEXT("CameraComponent"));
        SupportedTypes.Add(TEXT("Audio"), TEXT("AudioComponent"));
        SupportedTypes.Add(TEXT("AudioComponent"), TEXT("AudioComponent"));
        SupportedTypes.Add(TEXT("Scene"), TEXT("SceneComponent"));
        SupportedTypes.Add(TEXT("SceneComponent"), TEXT("SceneComponent"));
        SupportedTypes.Add(TEXT("Billboard"), TEXT("BillboardComponent"));
        SupportedTypes.Add(TEXT("BillboardComponent"), TEXT("BillboardComponent"));
    }
    
    return SupportedTypes;
}

UClass* FComponentTypeCache::ResolveComponentClassInternal(const FString& ComponentType) const
{
    // Get the mapped component type
    TMap<FString, FString> SupportedTypes = GetSupportedComponentTypes();
    const FString* MappedType = SupportedTypes.Find(ComponentType);
    FString ActualComponentType = MappedType ? *MappedType : ComponentType;
    
    // Try direct class lookups for common components
    if (ActualComponentType == TEXT("StaticMeshComponent"))
    {
        return UStaticMeshComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("PointLightComponent"))
    {
        return UPointLightComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("SpotLightComponent"))
    {
        return USpotLightComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("DirectionalLightComponent"))
    {
        return UDirectionalLightComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("BoxComponent"))
    {
        return UBoxComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("SphereComponent"))
    {
        return USphereComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("CapsuleComponent"))
    {
        return UCapsuleComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("CameraComponent"))
    {
        return UCameraComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("AudioComponent"))
    {
        return UAudioComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("SceneComponent"))
    {
        return USceneComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("BillboardComponent"))
    {
        return UBillboardComponent::StaticClass();
    }
    
    // Try loading from Engine module
    FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ActualComponentType);
    UClass* ComponentClass = LoadObject<UClass>(nullptr, *EnginePath);
    
    if (!ComponentClass)
    {
        // Try with U prefix
        FString WithUPrefix = FString::Printf(TEXT("U%s"), *ActualComponentType);
        EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *WithUPrefix);
        ComponentClass = LoadObject<UClass>(nullptr, *EnginePath);
    }
    
        // If not found in Engine, try loading as a Blueprint class
        if (!ComponentClass)
        {
            // Try direct Blueprint path (e.g., "/Game/DialogueSystem/DialogueComponent")
            if (ActualComponentType.StartsWith(TEXT("/")))
            {
                // Try both the Blueprint asset path and the generated class path
                ComponentClass = LoadObject<UClass>(nullptr, *ActualComponentType);
                if (!ComponentClass)
                {
                    FString GeneratedClassPath = ActualComponentType + TEXT("_C");
                    ComponentClass = LoadObject<UClass>(nullptr, *GeneratedClassPath);
                }
            }
	else
	{
		// Use AssetDiscoveryService to find Blueprint components
		TArray<FString> BlueprintPaths = FAssetDiscoveryService::Get().FindBlueprints(ActualComponentType);
		TArray<FString> BlueprintSearchPaths;
		
		// Convert Blueprint paths to generated class paths
		for (const FString& BPPath : BlueprintPaths)
		{
			FString GeneratedClassPath = BPPath + TEXT("_C");
			BlueprintSearchPaths.Add(GeneratedClassPath);
		}                for (const FString& SearchPath : BlueprintSearchPaths)
                {
                    ComponentClass = LoadObject<UClass>(nullptr, *SearchPath);
                    if (ComponentClass)
                    {
                        UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Found Blueprint component at '%s'"), *SearchPath);
                        break;
                    }
                }
            }
        }    // Verify that the class is a valid component type
    if (ComponentClass && !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Class '%s' is not a component type"), *ActualComponentType);
        return nullptr;
    }
    
    return ComponentClass;
}

FComponentService& FComponentService::Get()
{
    static FComponentService Instance;
    return Instance;
}

bool FComponentService::AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Adding component '%s' of type '%s' to blueprint '%s'"), 
        *Params.ComponentName, *Params.ComponentType, *Blueprint->GetName());
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: Invalid parameters - %s"), *ValidationError);
        return false;
    }
    
    // Get component class
    UClass* ComponentClass = GetComponentClass(Params.ComponentType);
    if (!ComponentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: Unknown component type '%s'"), *Params.ComponentType);
        return false;
    }
    
    // Ensure blueprint has a construction script
    if (!Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: Blueprint has no construction script"));
        return false;
    }
    
    // Create the component node
    USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *Params.ComponentName);
    if (!NewNode)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: Failed to create component node"));
        return false;
    }
    
    // Set transform if it's a scene component
    if (USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate))
    {
        SetComponentTransform(SceneComponent, Params.Location, Params.Rotation, Params.Scale);
    }
    
    // Set additional properties if provided
    if (Params.ComponentProperties.IsValid())
    {
        TArray<FString> SuccessProperties;
        TMap<FString, FString> FailedProperties;
        
        FPropertyService::Get().SetObjectProperties(NewNode->ComponentTemplate, Params.ComponentProperties,
                                                   SuccessProperties, FailedProperties);
        
        // Log any failed properties
        for (const auto& FailedProp : FailedProperties)
        {
            UE_LOG(LogTemp, Warning, TEXT("FComponentService::AddComponentToBlueprint: Failed to set property '%s' - %s"), 
                *FailedProp.Key, *FailedProp.Value);
        }
    }
    
    // Find parent component and establish hierarchy
    USCS_Node* ParentNode = nullptr;
    
    if (!Params.ParentComponentName.IsEmpty())
    {
        // Find specific parent component by name
        ParentNode = Blueprint->SimpleConstructionScript->FindSCSNode(*Params.ParentComponentName);
        if (!ParentNode)
        {
            UE_LOG(LogTemp, Warning, TEXT("FComponentService::AddComponentToBlueprint: Parent component '%s' not found, using default scene root"), 
                *Params.ParentComponentName);
        }
    }
    
    // If no specific parent or parent not found, use default scene root or first root node
    if (!ParentNode)
    {
        // Try to get the default scene root (for scene components)
        if (USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate))
        {
            ParentNode = Blueprint->SimpleConstructionScript->GetDefaultSceneRootNode();
            
            // If no default scene root, find the first scene component in root nodes
            if (!ParentNode)
            {
                TArray<USCS_Node*> RootNodes = Blueprint->SimpleConstructionScript->GetRootNodes();
                for (USCS_Node* RootNode : RootNodes)
                {
                    if (RootNode && RootNode->ComponentTemplate && 
                        Cast<USceneComponent>(RootNode->ComponentTemplate))
                    {
                        ParentNode = RootNode;
                        break;
                    }
                }
            }
        }
    }
    
    // Establish parent-child relationship
    if (ParentNode && Cast<USceneComponent>(NewNode->ComponentTemplate))
    {
        // Set parent relationship
        NewNode->SetParent(ParentNode);
        
        // Add as child to parent node
        ParentNode->AddChildNode(NewNode);
        
        UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Attached component '%s' to parent '%s'"), 
            *Params.ComponentName, *ParentNode->GetVariableName().ToString());
    }
    else
    {
        // Add as root node if no parent or not a scene component
        Blueprint->SimpleConstructionScript->AddNode(NewNode);
        
        UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Added component '%s' as root node"), 
            *Params.ComponentName);
    }
    
    // Mark blueprint as modified and refresh UI
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Force refresh of blueprint nodes and UI to ensure proper display and connectivity
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Successfully added component '%s'"), *Params.ComponentName);
    return true;
}

bool FComponentService::RemoveComponentFromBlueprint(UBlueprint* Blueprint, const FString& ComponentName)
{
    if (!Blueprint || !Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::RemoveComponentFromBlueprint: Invalid blueprint or construction script"));
        return false;
    }
    
    // Find the component node
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }
    
    if (!ComponentNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentService::RemoveComponentFromBlueprint: Component '%s' not found"), *ComponentName);
        return false;
    }
    
    // Remove the node
    Blueprint->SimpleConstructionScript->RemoveNode(ComponentNode);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::RemoveComponentFromBlueprint: Successfully removed component '%s'"), *ComponentName);
    return true;
}

UObject* FComponentService::FindComponentInBlueprint(UBlueprint* Blueprint, const FString& ComponentName)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Search in construction script first
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName().ToString() == ComponentName)
            {
                return Node->ComponentTemplate;
            }
        }
    }
    
    // Search inherited components on the CDO
    UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    AActor* DefaultActor = Cast<AActor>(DefaultObject);
    if (DefaultActor)
    {
        TArray<UActorComponent*> AllComponents;
        DefaultActor->GetComponents(AllComponents);
        for (UActorComponent* Comp : AllComponents)
        {
            if (Comp && Comp->GetName() == ComponentName)
            {
                return Comp;
            }
        }
    }
    
    return nullptr;
}

TArray<TPair<FString, FString>> FComponentService::GetBlueprintComponents(UBlueprint* Blueprint)
{
    TArray<TPair<FString, FString>> Components;
    
    if (!Blueprint)
    {
        return Components;
    }
    
    // Get components from construction script
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->ComponentTemplate)
            {
                FString ComponentName = Node->GetVariableName().ToString();
                FString ComponentType = Node->ComponentTemplate->GetClass()->GetName();
                Components.Add(TPair<FString, FString>(ComponentName, ComponentType));
            }
        }
    }
    
    // Get inherited components from CDO
    UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    AActor* DefaultActor = Cast<AActor>(DefaultObject);
    if (DefaultActor)
    {
        TArray<UActorComponent*> AllComponents;
        DefaultActor->GetComponents(AllComponents);
        for (UActorComponent* Comp : AllComponents)
        {
            if (Comp)
            {
                FString ComponentName = Comp->GetName();
                FString ComponentType = Comp->GetClass()->GetName();
                
                // Check if we already have this component (avoid duplicates)
                bool bAlreadyExists = false;
                for (const auto& ExistingComp : Components)
                {
                    if (ExistingComp.Key == ComponentName)
                    {
                        bAlreadyExists = true;
                        break;
                    }
                }
                
                if (!bAlreadyExists)
                {
                    Components.Add(TPair<FString, FString>(ComponentName, ComponentType));
                }
            }
        }
    }
    
    return Components;
}

bool FComponentService::IsValidComponentType(const FString& ComponentType)
{
    TMap<FString, FString> SupportedTypes = GetSupportedComponentTypes();
    return SupportedTypes.Contains(ComponentType);
}

UClass* FComponentService::GetComponentClass(const FString& ComponentType)
{
    return ComponentTypeCache.GetComponentClass(ComponentType);
}

TMap<FString, FString> FComponentService::GetSupportedComponentTypes() const
{
    static TMap<FString, FString> SupportedTypes;
    
    if (SupportedTypes.Num() == 0)
    {
        // Initialize supported component types mapping
        SupportedTypes.Add(TEXT("StaticMesh"), TEXT("StaticMeshComponent"));
        SupportedTypes.Add(TEXT("StaticMeshComponent"), TEXT("StaticMeshComponent"));
        SupportedTypes.Add(TEXT("PointLight"), TEXT("PointLightComponent"));
        SupportedTypes.Add(TEXT("PointLightComponent"), TEXT("PointLightComponent"));
        SupportedTypes.Add(TEXT("SpotLight"), TEXT("SpotLightComponent"));
        SupportedTypes.Add(TEXT("SpotLightComponent"), TEXT("SpotLightComponent"));
        SupportedTypes.Add(TEXT("DirectionalLight"), TEXT("DirectionalLightComponent"));
        SupportedTypes.Add(TEXT("DirectionalLightComponent"), TEXT("DirectionalLightComponent"));
        SupportedTypes.Add(TEXT("Box"), TEXT("BoxComponent"));
        SupportedTypes.Add(TEXT("BoxComponent"), TEXT("BoxComponent"));
        SupportedTypes.Add(TEXT("Sphere"), TEXT("SphereComponent"));
        SupportedTypes.Add(TEXT("SphereComponent"), TEXT("SphereComponent"));
        SupportedTypes.Add(TEXT("Capsule"), TEXT("CapsuleComponent"));
        SupportedTypes.Add(TEXT("CapsuleComponent"), TEXT("CapsuleComponent"));
        SupportedTypes.Add(TEXT("Camera"), TEXT("CameraComponent"));
        SupportedTypes.Add(TEXT("CameraComponent"), TEXT("CameraComponent"));
        SupportedTypes.Add(TEXT("Audio"), TEXT("AudioComponent"));
        SupportedTypes.Add(TEXT("AudioComponent"), TEXT("AudioComponent"));
        SupportedTypes.Add(TEXT("Scene"), TEXT("SceneComponent"));
        SupportedTypes.Add(TEXT("SceneComponent"), TEXT("SceneComponent"));
        SupportedTypes.Add(TEXT("Billboard"), TEXT("BillboardComponent"));
        SupportedTypes.Add(TEXT("BillboardComponent"), TEXT("BillboardComponent"));
    }
    
    return SupportedTypes;
}

UClass* FComponentService::ResolveComponentClass(const FString& ComponentType) const
{
    // Get the mapped component type
    TMap<FString, FString> SupportedTypes = GetSupportedComponentTypes();
    const FString* MappedType = SupportedTypes.Find(ComponentType);
    FString ActualComponentType = MappedType ? *MappedType : ComponentType;
    
    // Try direct class lookups for common components
    if (ActualComponentType == TEXT("StaticMeshComponent"))
    {
        return UStaticMeshComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("PointLightComponent"))
    {
        return UPointLightComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("SpotLightComponent"))
    {
        return USpotLightComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("DirectionalLightComponent"))
    {
        return UDirectionalLightComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("BoxComponent"))
    {
        return UBoxComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("SphereComponent"))
    {
        return USphereComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("CapsuleComponent"))
    {
        return UCapsuleComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("CameraComponent"))
    {
        return UCameraComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("AudioComponent"))
    {
        return UAudioComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("SceneComponent"))
    {
        return USceneComponent::StaticClass();
    }
    else if (ActualComponentType == TEXT("BillboardComponent"))
    {
        return UBillboardComponent::StaticClass();
    }
    
    // Try loading from Engine module
    FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ActualComponentType);
    UClass* ComponentClass = LoadObject<UClass>(nullptr, *EnginePath);
    
    if (!ComponentClass)
    {
        // Try with U prefix
        FString WithUPrefix = FString::Printf(TEXT("U%s"), *ActualComponentType);
        EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *WithUPrefix);
        ComponentClass = LoadObject<UClass>(nullptr, *EnginePath);
    }
    
    // Verify that the class is a valid component type
    if (ComponentClass && !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentService::ResolveComponentClass: Class '%s' is not a component type"), *ActualComponentType);
        return nullptr;
    }
    
    return ComponentClass;
}

void FComponentService::SetComponentTransform(USceneComponent* SceneComponent, 
                                            const TArray<float>& Location,
                                            const TArray<float>& Rotation,
                                            const TArray<float>& Scale) const
{
    if (!SceneComponent)
    {
        return;
    }
    
    // Set location
    if (Location.Num() == 3)
    {
        FVector LocationVector(Location[0], Location[1], Location[2]);
        SceneComponent->SetRelativeLocation(LocationVector);
    }
    
    // Set rotation
    if (Rotation.Num() == 3)
    {
        FRotator RotationRotator(Rotation[0], Rotation[1], Rotation[2]);
        SceneComponent->SetRelativeRotation(RotationRotator);
    }
    
    // Set scale
    if (Scale.Num() == 3)
    {
        FVector ScaleVector(Scale[0], Scale[1], Scale[2]);
        SceneComponent->SetRelativeScale3D(ScaleVector);
    }
}

bool FComponentService::SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& PhysicsParams)
{
    if (!Blueprint || !PhysicsParams.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetPhysicsProperties: Invalid parameters"));
        return false;
    }
    
    // Find the component
    UObject* Component = FindComponentInBlueprint(Blueprint, ComponentName);
    if (!Component)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetPhysicsProperties: Component '%s' not found"), *ComponentName);
        return false;
    }
    
    // Check if it's a primitive component (supports physics)
    UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
    if (!PrimitiveComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetPhysicsProperties: Component '%s' is not a primitive component"), *ComponentName);
        return false;
    }
    
    // Set physics properties
    bool bSimulatePhysics = false;
    if (PhysicsParams->TryGetBoolField(TEXT("simulate_physics"), bSimulatePhysics))
    {
        PrimitiveComponent->SetSimulatePhysics(bSimulatePhysics);
    }
    
    bool bGravityEnabled = true;
    if (PhysicsParams->TryGetBoolField(TEXT("gravity_enabled"), bGravityEnabled))
    {
        PrimitiveComponent->SetEnableGravity(bGravityEnabled);
    }
    
    double Mass = 1.0;
    if (PhysicsParams->TryGetNumberField(TEXT("mass"), Mass))
    {
        PrimitiveComponent->SetMassOverrideInKg(NAME_None, static_cast<float>(Mass), true);
    }
    
    double LinearDamping = 0.01;
    if (PhysicsParams->TryGetNumberField(TEXT("linear_damping"), LinearDamping))
    {
        PrimitiveComponent->SetLinearDamping(static_cast<float>(LinearDamping));
    }
    
    double AngularDamping = 0.0;
    if (PhysicsParams->TryGetNumberField(TEXT("angular_damping"), AngularDamping))
    {
        PrimitiveComponent->SetAngularDamping(static_cast<float>(AngularDamping));
    }
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::SetPhysicsProperties: Successfully set physics properties for component '%s'"), *ComponentName);
    return true;
}

bool FComponentService::SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath)
{
    if (!Blueprint || StaticMeshPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetStaticMeshProperties: Invalid parameters"));
        return false;
    }
    
    // Find the component
    UObject* Component = FindComponentInBlueprint(Blueprint, ComponentName);
    if (!Component)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetStaticMeshProperties: Component '%s' not found"), *ComponentName);
        return false;
    }
    
    // Check if it's a static mesh component
    UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component);
    if (!StaticMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetStaticMeshProperties: Component '%s' is not a static mesh component"), *ComponentName);
        return false;
    }
    
    // Load the static mesh
    UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *StaticMeshPath);
    if (!StaticMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("FComponentService::SetStaticMeshProperties: Failed to load static mesh '%s'"), *StaticMeshPath);
        return false;
    }
    
    // Set the static mesh
    StaticMeshComponent->SetStaticMesh(StaticMesh);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::SetStaticMeshProperties: Successfully set static mesh '%s' for component '%s'"), *StaticMeshPath, *ComponentName);
    return true;
}
