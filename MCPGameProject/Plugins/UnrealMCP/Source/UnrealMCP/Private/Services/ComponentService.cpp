#include "Services/ComponentService.h"
#include "Services/IPropertyService.h"
#include "Services/PropertyService.h"
#include "Services/AssetDiscoveryService.h"
#include "Factories/ComponentFactory.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "GameFramework/Actor.h"
#include "SubobjectDataSubsystem.h"
#include "SubobjectData.h"
#include "Engine/Engine.h"

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
        SupportedTypes.Add(TEXT("Widget"), TEXT("WidgetComponent"));
        SupportedTypes.Add(TEXT("WidgetComponent"), TEXT("WidgetComponent"));
    }
    
    return SupportedTypes;
}

UClass* FComponentTypeCache::ResolveComponentClassInternal(const FString& ComponentType) const
{
    // Get the mapped component type (for aliases like "StaticMesh" -> "StaticMeshComponent")
    TMap<FString, FString> SupportedTypes = GetSupportedComponentTypes();
    const FString* MappedType = SupportedTypes.Find(ComponentType);
    FString ActualComponentType = MappedType ? *MappedType : ComponentType;
    
    // First, try using ComponentFactory which has all default types registered
    UClass* ComponentClass = FComponentFactory::Get().GetComponentClass(ActualComponentType);
    if (ComponentClass)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Found component '%s' via ComponentFactory"), *ActualComponentType);
        return ComponentClass;
    }
    
    // If not in ComponentFactory, try loading from Engine module
    FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ActualComponentType);
    ComponentClass = LoadObject<UClass>(nullptr, *EnginePath);
    
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
        UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Component '%s' not found in Engine, searching for Blueprint..."), *ActualComponentType);
        
        // Try direct Blueprint path (e.g., "/Game/DialogueSystem/DialogueComponent")
        if (ActualComponentType.StartsWith(TEXT("/")))
        {
            UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Trying direct path: %s"), *ActualComponentType);
            
            // Try both the Blueprint asset path and the generated class path
            ComponentClass = LoadObject<UClass>(nullptr, *ActualComponentType);
            if (!ComponentClass)
            {
                FString GeneratedClassPath = ActualComponentType + TEXT("_C");
                UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Trying generated class path: %s"), *GeneratedClassPath);
                ComponentClass = LoadObject<UClass>(nullptr, *GeneratedClassPath);
            }
            
            if (ComponentClass)
            {
                UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Found Blueprint component via direct path"));
            }
        }
        else
        {
            // Use AssetDiscoveryService to find Blueprint components
            UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Using AssetDiscoveryService to find '%s'"), *ActualComponentType);
            
            TArray<FString> BlueprintPaths = FAssetDiscoveryService::Get().FindBlueprints(ActualComponentType);
            UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: AssetDiscoveryService found %d Blueprint paths"), BlueprintPaths.Num());
            
            TArray<FString> BlueprintSearchPaths;
            
            // Convert Blueprint paths to generated class paths
            for (const FString& BPPath : BlueprintPaths)
            {
                FString GeneratedClassPath = BPPath + TEXT("_C");
                BlueprintSearchPaths.Add(GeneratedClassPath);
                UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Will try generated class path: %s"), *GeneratedClassPath);
            }
            
            for (const FString& SearchPath : BlueprintSearchPaths)
            {
                UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Attempting to load: %s"), *SearchPath);
                ComponentClass = LoadObject<UClass>(nullptr, *SearchPath);
                if (ComponentClass)
                {
                    UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Found Blueprint component at '%s'"), *SearchPath);
                    break;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Failed to load from '%s'"), *SearchPath);
                }
            }
        }
    }
    
    // Verify that the class is a valid component type
    if (ComponentClass && !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Class '%s' is not a component type (found class: %s)"), 
            *ActualComponentType, *ComponentClass->GetName());
        return nullptr;
    }
    
    if (!ComponentClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Failed to resolve component type '%s'"), *ActualComponentType);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("FComponentTypeCache::ResolveComponentClassInternal: Successfully resolved '%s' to class '%s'"), 
            *ActualComponentType, *ComponentClass->GetName());
    }
    
    return ComponentClass;
}

FComponentService& FComponentService::Get()
{
    static FComponentService Instance;
    return Instance;
}

bool FComponentService::AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params, FString& OutErrorMessage)
{
    if (!Blueprint)
    {
        OutErrorMessage = TEXT("Invalid blueprint");
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Adding component '%s' of type '%s' to blueprint '%s'"), 
        *Params.ComponentName, *Params.ComponentType, *Blueprint->GetName());
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        OutErrorMessage = FString::Printf(TEXT("Invalid parameters: %s"), *ValidationError);
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    // Get component class
    UClass* ComponentClass = GetComponentClass(Params.ComponentType);
    if (!ComponentClass)
    {
        OutErrorMessage = FString::Printf(TEXT("Unknown component type '%s'"), *Params.ComponentType);
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    // Check if this is an ActorComponent blueprint - they don't support child components
    if (Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        OutErrorMessage = FString::Printf(
            TEXT("Cannot add components to ActorComponent blueprints. Blueprint '%s' with parent class '%s' does not support child components. Consider using an Actor-based blueprint instead."), 
            *Blueprint->GetName(), *Blueprint->ParentClass->GetName());
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    // Get the Subobject Data Subsystem (UE 5.6+ API)
    // Note: USubobjectDataSubsystem is a UEngineSubsystem, not UEditorSubsystem
    USubobjectDataSubsystem* SubobjectDataSubsystem = GEngine->GetEngineSubsystem<USubobjectDataSubsystem>();
    if (!SubobjectDataSubsystem)
    {
        OutErrorMessage = TEXT("Failed to get SubobjectDataSubsystem");
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    // Gather subobject data for the blueprint using K2_GatherSubobjectDataForBlueprint
    TArray<FSubobjectDataHandle> SubobjectHandles;
    SubobjectDataSubsystem->K2_GatherSubobjectDataForBlueprint(Blueprint, SubobjectHandles);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Found %d existing subobjects"), SubobjectHandles.Num());
    
    // Find the parent handle
    FSubobjectDataHandle ParentHandle;
    
    if (!Params.ParentComponentName.IsEmpty())
    {
        // Look for specific parent component by name
        for (const FSubobjectDataHandle& Handle : SubobjectHandles)
        {
            FSubobjectData* Data = Handle.GetData();
            if (Data && Data->GetVariableName().ToString() == Params.ParentComponentName)
            {
                ParentHandle = Handle;
                UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Found specified parent '%s'"), 
                    *Params.ParentComponentName);
                break;
            }
        }
        
        if (!ParentHandle.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("FComponentService::AddComponentToBlueprint: Specified parent '%s' not found, will find scene root"), 
                *Params.ParentComponentName);
        }
    }
    
    // If no parent specified or parent not found, we need to find a suitable parent
    if (!ParentHandle.IsValid())
    {
        if (ComponentClass->IsChildOf(USceneComponent::StaticClass()))
        {
            // For scene components, find scene root
            if (SubobjectHandles.Num() > 0)
            {
                ParentHandle = SubobjectDataSubsystem->FindSceneRootForSubobject(SubobjectHandles[0]);
                
                if (ParentHandle.IsValid())
                {
                    FSubobjectData* ParentData = ParentHandle.GetData();
                    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Found scene root '%s'"), 
                        ParentData ? *ParentData->GetVariableName().ToString() : TEXT("Unknown"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("FComponentService::AddComponentToBlueprint: FindSceneRootForSubobject returned invalid handle"));
                }
            }
        }
        else
        {
            // For non-scene ActorComponents, use the first valid subobject as parent (usually the actor root)
            UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Component is ActorComponent (not SceneComponent), searching for actor root"));
            for (const FSubobjectDataHandle& Handle : SubobjectHandles)
            {
                FSubobjectData* Data = Handle.GetData();
                if (Data)
                {
                    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Checking subobject '%s', IsRootComponent=%d, IsDefaultSceneRoot=%d"), 
                        *Data->GetVariableName().ToString(),
                        Data->IsRootComponent(),
                        Data->IsDefaultSceneRoot());
                    
                    // Use the first subobject that represents the actor itself (default scene root or actor root)
                    if (Data->IsDefaultSceneRoot() || Data->IsRootComponent())
                    {
                        ParentHandle = Handle;
                        UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Using '%s' as parent for ActorComponent"), 
                            *Data->GetVariableName().ToString());
                        break;
                    }
                }
            }
            
            // If still no parent, just use the first subobject
            if (!ParentHandle.IsValid() && SubobjectHandles.Num() > 0)
            {
                ParentHandle = SubobjectHandles[0];
                FSubobjectData* Data = ParentHandle.GetData();
                UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: No root found, using first subobject '%s' as parent"), 
                    Data ? *Data->GetVariableName().ToString() : TEXT("Unknown"));
            }
        }
    }
    
    // Prepare parameters for adding new subobject
    FAddNewSubobjectParams AddParams;
    AddParams.ParentHandle = ParentHandle;
    AddParams.NewClass = ComponentClass;
    AddParams.BlueprintContext = Blueprint;
    AddParams.bSkipMarkBlueprintModified = false; // We want to mark blueprint as modified
    AddParams.bConformTransformToParent = false;   // Keep our custom transform
    
    // Log detailed information before attempting to add
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Attempting to add component with:"));
    UE_LOG(LogTemp, Log, TEXT("  - ComponentClass: %s (valid: %d)"), ComponentClass ? *ComponentClass->GetName() : TEXT("NULL"), ComponentClass != nullptr);
    UE_LOG(LogTemp, Log, TEXT("  - ComponentClass->IsChildOf(UActorComponent): %d"), ComponentClass ? ComponentClass->IsChildOf(UActorComponent::StaticClass()) : false);
    UE_LOG(LogTemp, Log, TEXT("  - ParentHandle.IsValid(): %d"), ParentHandle.IsValid());
    UE_LOG(LogTemp, Log, TEXT("  - BlueprintContext: %s"), Blueprint ? *Blueprint->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Log, TEXT("  - Blueprint->ParentClass: %s"), Blueprint->ParentClass ? *Blueprint->ParentClass->GetName() : TEXT("NULL"));
    
    // Add the new component using the subsystem
    FText FailReason;
    FSubobjectDataHandle NewHandle = SubobjectDataSubsystem->AddNewSubobject(AddParams, FailReason);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: AddNewSubobject returned handle valid=%d, FailReason='%s'"), 
        NewHandle.IsValid(), *FailReason.ToString());
    
    if (!NewHandle.IsValid())
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to create component: %s"), *FailReason.ToString());
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    // Get the newly created component template to set properties
    FSubobjectData* NewSubobjectData = NewHandle.GetData();
    if (!NewSubobjectData)
    {
        OutErrorMessage = TEXT("Failed to get subobject data for new component");
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    const UObject* ConstComponentTemplate = NewSubobjectData->GetObject();
    if (!ConstComponentTemplate)
    {
        OutErrorMessage = TEXT("Failed to get component template");
        UE_LOG(LogTemp, Error, TEXT("FComponentService::AddComponentToBlueprint: %s"), *OutErrorMessage);
        return false;
    }
    
    // Remove const qualifier - we need to modify the component
    UObject* ComponentTemplate = const_cast<UObject*>(ConstComponentTemplate);
    
    // Rename the component to match requested name
    if (!Params.ComponentName.IsEmpty())
    {
        if (!SubobjectDataSubsystem->RenameSubobject(NewHandle, FText::FromString(Params.ComponentName)))
        {
            UE_LOG(LogTemp, Warning, TEXT("FComponentService::AddComponentToBlueprint: Failed to rename component to '%s'"), 
                *Params.ComponentName);
        }
    }
    
    // Set transform if it's a scene component
    if (USceneComponent* SceneComponent = Cast<USceneComponent>(ComponentTemplate))
    {
        SetComponentTransform(SceneComponent, Params.Location, Params.Rotation, Params.Scale);
    }
    
    // Set additional properties if provided
    if (Params.ComponentProperties.IsValid())
    {
        TArray<FString> SuccessProperties;
        TMap<FString, FString> FailedProperties;
        
        FPropertyService::Get().SetObjectProperties(ComponentTemplate, Params.ComponentProperties,
                                                   SuccessProperties, FailedProperties);
        
        // Log any failed properties
        for (const auto& FailedProp : FailedProperties)
        {
            UE_LOG(LogTemp, Warning, TEXT("FComponentService::AddComponentToBlueprint: Failed to set property '%s' - %s"), 
                *FailedProp.Key, *FailedProp.Value);
        }
    }
    
    // Mark blueprint as modified and refresh
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("FComponentService::AddComponentToBlueprint: Successfully added component '%s' using SubobjectDataSubsystem"), 
        *Params.ComponentName);
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
        SupportedTypes.Add(TEXT("Widget"), TEXT("WidgetComponent"));
        SupportedTypes.Add(TEXT("WidgetComponent"), TEXT("WidgetComponent"));
    }
    
    return SupportedTypes;
}

UClass* FComponentService::ResolveComponentClass(const FString& ComponentType) const
{
    // Get the mapped component type (for aliases like "StaticMesh" -> "StaticMeshComponent")
    TMap<FString, FString> SupportedTypes = GetSupportedComponentTypes();
    const FString* MappedType = SupportedTypes.Find(ComponentType);
    FString ActualComponentType = MappedType ? *MappedType : ComponentType;
    
    // First, try using ComponentFactory which has all default types registered
    UClass* ComponentClass = FComponentFactory::Get().GetComponentClass(ActualComponentType);
    if (ComponentClass)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FComponentService::ResolveComponentClass: Found component '%s' via ComponentFactory"), *ActualComponentType);
        return ComponentClass;
    }
    
    // If not in ComponentFactory, try loading from Engine module
    FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ActualComponentType);
    ComponentClass = LoadObject<UClass>(nullptr, *EnginePath);
    
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
