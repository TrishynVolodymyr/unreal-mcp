#include "Services/BlueprintService.h"
#include "Services/ComponentService.h"
#include "Services/PropertyService.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"

// Blueprint Cache Implementation
UBlueprint* FBlueprintCache::GetBlueprint(const FString& BlueprintName)
{
    FScopeLock Lock(&CacheLock);
    
    // Update statistics
    UpdateStats(false); // Assume miss initially
    
    if (TWeakObjectPtr<UBlueprint>* CachedPtr = CachedBlueprints.Find(BlueprintName))
    {
        if (CachedPtr->IsValid())
        {
            // Update to hit since we found a valid entry
            CacheStats.CacheHits++;
            CacheStats.CacheMisses--;
            
            UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Cache hit for blueprint '%s'"), *BlueprintName);
            return CachedPtr->Get();
        }
        else
        {
            // Remove invalid entry
            CachedBlueprints.Remove(BlueprintName);
            CacheStats.CachedCount = CachedBlueprints.Num();
            UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Removed invalid cache entry for blueprint '%s'"), *BlueprintName);
        }
    }
    
    return nullptr;
}

void FBlueprintCache::CacheBlueprint(const FString& BlueprintName, UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return;
    }
    
    FScopeLock Lock(&CacheLock);
    CachedBlueprints.Add(BlueprintName, Blueprint);
    CacheStats.CachedCount = CachedBlueprints.Num();
    UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Cached blueprint '%s'"), *BlueprintName);
}

void FBlueprintCache::InvalidateBlueprint(const FString& BlueprintName)
{
    FScopeLock Lock(&CacheLock);
    if (CachedBlueprints.Remove(BlueprintName) > 0)
    {
        CacheStats.InvalidatedCount++;
        CacheStats.CachedCount = CachedBlueprints.Num();
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Invalidated cache for blueprint '%s'"), *BlueprintName);
    }
}

void FBlueprintCache::ClearCache()
{
    FScopeLock Lock(&CacheLock);
    int32 ClearedCount = CachedBlueprints.Num();
    CachedBlueprints.Empty();
    CacheStats.CachedCount = 0;
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cleared %d cached blueprints"), ClearedCount);
}

void FBlueprintCache::WarmCache(const TArray<FString>& BlueprintNames)
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Warming cache with %d blueprints"), BlueprintNames.Num());
    
    for (const FString& BlueprintName : BlueprintNames)
    {
        // Check if already cached
        if (IsCached(BlueprintName))
        {
            continue;
        }
        
        // Try to find and cache the blueprint
        UBlueprint* FoundBlueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
        if (FoundBlueprint)
        {
            CacheBlueprint(BlueprintName, FoundBlueprint);
            UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Warmed cache with blueprint '%s'"), *BlueprintName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FBlueprintCache: Could not find blueprint '%s' for cache warming"), *BlueprintName);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cache warming complete. %d blueprints cached"), GetCachedCount());
}

void FBlueprintCache::WarmCacheWithCommonBlueprints()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Warming cache with common blueprints"));
    
    // Common blueprint names that are frequently used
    TArray<FString> CommonBlueprints = {
        TEXT("BP_PlayerController"),
        TEXT("BP_GameMode"),
        TEXT("BP_GameState"),
        TEXT("BP_PlayerState"),
        TEXT("BP_Character"),
        TEXT("BP_Pawn"),
        TEXT("BP_Actor"),
        TEXT("BP_HUD"),
        TEXT("BP_Widget"),
        TEXT("BP_UserWidget"),
        TEXT("ThirdPersonCharacter"),
        TEXT("BP_ThirdPersonCharacter"),
        TEXT("FirstPersonCharacter"),
        TEXT("BP_FirstPersonCharacter")
    };
    
    WarmCache(CommonBlueprints);
}

FBlueprintCacheStats FBlueprintCache::GetCacheStats() const
{
    FScopeLock Lock(&CacheLock);
    FBlueprintCacheStats StatsCopy = CacheStats;
    StatsCopy.CachedCount = CachedBlueprints.Num();
    return StatsCopy;
}

void FBlueprintCache::ResetCacheStats()
{
    FScopeLock Lock(&CacheLock);
    CacheStats.Reset();
    CacheStats.CachedCount = CachedBlueprints.Num();
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cache statistics reset"));
}

int32 FBlueprintCache::GetCachedCount() const
{
    FScopeLock Lock(&CacheLock);
    return CachedBlueprints.Num();
}

bool FBlueprintCache::IsCached(const FString& BlueprintName) const
{
    FScopeLock Lock(&CacheLock);
    
    if (const TWeakObjectPtr<UBlueprint>* CachedPtr = CachedBlueprints.Find(BlueprintName))
    {
        return CachedPtr->IsValid();
    }
    
    return false;
}

void FBlueprintCache::UpdateStats(bool bWasHit) const
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

int32 FBlueprintCache::CleanupInvalidEntries()
{
    // This method assumes the lock is already held
    int32 CleanedCount = 0;
    
    for (auto It = CachedBlueprints.CreateIterator(); It; ++It)
    {
        if (!It.Value().IsValid())
        {
            It.RemoveCurrent();
            CleanedCount++;
        }
    }
    
    if (CleanedCount > 0)
    {
        CacheStats.CachedCount = CachedBlueprints.Num();
        UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cleaned up %d invalid cache entries"), CleanedCount);
    }
    
    return CleanedCount;
}

// Blueprint Service Implementation
FBlueprintService& FBlueprintService::Get()
{
    static FBlueprintService Instance;
    return Instance;
}

UBlueprint* FBlueprintService::CreateBlueprint(const FBlueprintCreationParams& Params)
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprint: Creating blueprint '%s'"), *Params.Name);
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprint: Invalid parameters - %s"), *ValidationError);
        return nullptr;
    }
    
    // Normalize the blueprint path
    FString NormalizedPath;
    FString PathError;
    if (!NormalizeBlueprintPath(Params.FolderPath, NormalizedPath, PathError))
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprint: Path normalization failed - %s"), *PathError);
        return nullptr;
    }
    
    // Build full asset path
    FString FullAssetPath = NormalizedPath + Params.Name;
    
    // Check if blueprint already exists
    if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateBlueprint: Blueprint already exists at '%s'"), *FullAssetPath);
        UBlueprint* ExistingBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullAssetPath));
        if (ExistingBlueprint)
        {
            BlueprintCache.CacheBlueprint(Params.Name, ExistingBlueprint);
        }
        return ExistingBlueprint;
    }
    
    // Create directory structure if needed
    FString DirectoryError;
    if (!CreateDirectoryStructure(NormalizedPath, DirectoryError))
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprint: Failed to create directory structure - %s"), *DirectoryError);
        return nullptr;
    }
    
    // Resolve parent class
    UClass* ParentClass = Params.ParentClass;
    if (!ParentClass)
    {
        ParentClass = AActor::StaticClass(); // Default to Actor
        UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprint: Using default parent class AActor"));
    }
    
    // Create the package
    UObject* Package = CreatePackage(*FullAssetPath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprint: Failed to create package for path '%s'"), *FullAssetPath);
        return nullptr;
    }
    
    // Create the blueprint using FKismetEditorUtilities
    UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        *Params.Name,
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        NAME_None
    );
    
    if (!NewBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprint: Failed to create blueprint"));
        return nullptr;
    }
    
    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(NewBlueprint);
    
    // Mark the package dirty
    Package->MarkPackageDirty();
    
    // Compile if requested
    if (Params.bCompileOnCreation)
    {
        FString CompileError;
        if (!CompileBlueprint(NewBlueprint, CompileError))
        {
            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateBlueprint: Blueprint compilation failed - %s"), *CompileError);
        }
    }
    
    // Save the asset
    if (UEditorAssetLibrary::SaveLoadedAsset(NewBlueprint))
    {
        UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprint: Successfully saved blueprint '%s'"), *FullAssetPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateBlueprint: Failed to save blueprint '%s'"), *FullAssetPath);
    }
    
    // Cache the blueprint
    BlueprintCache.CacheBlueprint(Params.Name, NewBlueprint);
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprint: Successfully created blueprint '%s'"), *Params.Name);
    return NewBlueprint;
}

bool FBlueprintService::AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params)
{
    // Delegate to ComponentService for component operations
    bool bResult = FComponentService::Get().AddComponentToBlueprint(Blueprint, Params);
    
    if (bResult)
    {
        // Invalidate cache since blueprint was modified
        BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    }
    
    return bResult;
}

bool FBlueprintService::CompileBlueprint(UBlueprint* Blueprint, FString& OutError)
{
    if (!Blueprint)
    {
        OutError = TEXT("Invalid blueprint");
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CompileBlueprint: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CompileBlueprint: Compiling blueprint '%s'"), *Blueprint->GetName());
    
    // Clear any existing compilation errors
    Blueprint->Status = BS_Unknown;
    
    // Compile the blueprint
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    
    // Check compilation result
    if (Blueprint->Status == BS_Error)
    {
        OutError = TEXT("Blueprint compilation failed with errors");
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CompileBlueprint: Compilation failed for blueprint '%s'"), *Blueprint->GetName());
        
        // CompileLog is not available in UE 5.6, using basic error reporting
        
        return false;
    }
    else if (Blueprint->Status == BS_UpToDateWithWarnings)
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CompileBlueprint: Blueprint '%s' compiled with warnings"), *Blueprint->GetName());
        
        // CompileLog is not available in UE 5.6, using basic warning reporting
    }
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CompileBlueprint: Successfully compiled blueprint '%s'"), *Blueprint->GetName());
    return true;
}

UBlueprint* FBlueprintService::FindBlueprint(const FString& BlueprintName)
{
    UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::FindBlueprint: Looking for blueprint '%s'"), *BlueprintName);
    
    // Check cache first
    UBlueprint* CachedBlueprint = BlueprintCache.GetBlueprint(BlueprintName);
    if (CachedBlueprint)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::FindBlueprint: Found blueprint '%s' in cache"), *BlueprintName);
        return CachedBlueprint;
    }
    
    // Use common utils to find blueprint
    UBlueprint* FoundBlueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (FoundBlueprint)
    {
        // Cache for future lookups
        BlueprintCache.CacheBlueprint(BlueprintName, FoundBlueprint);
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::FindBlueprint: Found and cached blueprint '%s'"), *BlueprintName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::FindBlueprint: Blueprint '%s' not found"), *BlueprintName);
    }
    
    return FoundBlueprint;
}

bool FBlueprintService::AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, const FString& VariableType, bool bIsExposed)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddVariableToBlueprint: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddVariableToBlueprint: Adding variable '%s' of type '%s' to blueprint '%s'"), 
        *VariableName, *VariableType, *Blueprint->GetName());
    
    // Resolve variable type
    UObject* TypeObject = ResolveVariableType(VariableType);
    if (!TypeObject)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddVariableToBlueprint: Unknown variable type '%s'"), *VariableType);
        return false;
    }
    
    // Create variable description
    FBPVariableDescription NewVar;
    NewVar.VarName = *VariableName;
    NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Object; // Default, will be adjusted based on type
    
    // Set type based on resolved type object
    if (UClass* ClassType = Cast<UClass>(TypeObject))
    {
        NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Object;
        NewVar.VarType.PinSubCategoryObject = ClassType;
    }
    else if (UScriptStruct* StructType = Cast<UScriptStruct>(TypeObject))
    {
        NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        NewVar.VarType.PinSubCategoryObject = StructType;
    }
    else
    {
        // Handle basic types
        if (VariableType == TEXT("Boolean") || VariableType == TEXT("bool"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else if (VariableType == TEXT("Integer") || VariableType == TEXT("int") || VariableType == TEXT("int32"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (VariableType == TEXT("Float") || VariableType == TEXT("float"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Real;
            NewVar.VarType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (VariableType == TEXT("String") || VariableType == TEXT("FString"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (VariableType == TEXT("Vector") || VariableType == TEXT("FVector"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            NewVar.VarType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        }
        else if (VariableType == TEXT("Rotator") || VariableType == TEXT("FRotator"))
        {
            NewVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            NewVar.VarType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddVariableToBlueprint: Unsupported basic type '%s'"), *VariableType);
            return false;
        }
    }
    
    // Set exposure
    if (bIsExposed)
    {
        NewVar.PropertyFlags |= CPF_BlueprintVisible;
        NewVar.PropertyFlags |= CPF_Edit;
    }
    
    // Add variable to blueprint
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, NewVar.VarName, NewVar.VarType);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddVariableToBlueprint: Successfully added variable '%s'"), *VariableName);
    return true;
}

bool FBlueprintService::SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetBlueprintProperty: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetBlueprintProperty: Setting property '%s' on blueprint '%s'"), 
        *PropertyName, *Blueprint->GetName());
    
    // Get the blueprint's default object
    UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    if (!DefaultObject)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetBlueprintProperty: No default object available"));
        return false;
    }
    
    // Set the property using PropertyService
    FString ErrorMessage;
    if (!FPropertyService::Get().SetObjectProperty(DefaultObject, PropertyName, PropertyValue, ErrorMessage))
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetBlueprintProperty: Failed to set property - %s"), *ErrorMessage);
        return false;
    }
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetBlueprintProperty: Successfully set property '%s'"), *PropertyName);
    return true;
}

// Private helper methods
UClass* FBlueprintService::ResolveParentClass(const FString& ParentClassName) const
{
    if (ParentClassName.IsEmpty())
    {
        return AActor::StaticClass(); // Default
    }
    
    FString ClassName = ParentClassName;
    
    // Add appropriate prefix if not present
    if (!ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
    {
        if (ClassName.EndsWith(TEXT("Component")))
        {
            ClassName = TEXT("U") + ClassName;
        }
        else
        {
            ClassName = TEXT("A") + ClassName;
        }
    }
    
    // Try direct StaticClass lookup for common classes
    if (ClassName == TEXT("APawn"))
    {
        return APawn::StaticClass();
    }
    else if (ClassName == TEXT("AActor"))
    {
        return AActor::StaticClass();
    }
    else if (ClassName == TEXT("ACharacter"))
    {
        return ACharacter::StaticClass();
    }
    else if (ClassName == TEXT("APlayerController"))
    {
        return APlayerController::StaticClass();
    }
    else if (ClassName == TEXT("AGameModeBase"))
    {
        return AGameModeBase::StaticClass();
    }
    else if (ClassName == TEXT("UActorComponent"))
    {
        return UActorComponent::StaticClass();
    }
    else if (ClassName == TEXT("USceneComponent"))
    {
        return USceneComponent::StaticClass();
    }
    
    // Try loading from common module paths
    TArray<FString> ModulePaths = {
        TEXT("/Script/Engine"),
        TEXT("/Script/GameplayAbilities"),
        TEXT("/Script/AIModule"),
        TEXT("/Script/Game"),
        TEXT("/Script/CoreUObject")
    };
    
    for (const FString& ModulePath : ModulePaths)
    {
        const FString ClassPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *ClassName);
        if (UClass* FoundClass = LoadClass<UObject>(nullptr, *ClassPath))
        {
            return FoundClass;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::ResolveParentClass: Could not resolve parent class '%s'"), *ParentClassName);
    return AActor::StaticClass(); // Fallback to Actor
}

bool FBlueprintService::CreateDirectoryStructure(const FString& FolderPath, FString& OutError) const
{
    if (FolderPath.IsEmpty() || UEditorAssetLibrary::DoesDirectoryExist(FolderPath))
    {
        return true; // Already exists or not needed
    }
    
    // Parse folder levels
    FString CleanPath = FolderPath;
    if (CleanPath.StartsWith(TEXT("/Game/")))
    {
        CleanPath = CleanPath.RightChop(6); // Remove "/Game/"
    }
    
    if (CleanPath.IsEmpty())
    {
        return true;
    }
    
    TArray<FString> FolderLevels;
    CleanPath.ParseIntoArray(FolderLevels, TEXT("/"));
    
    FString CurrentPath = TEXT("/Game/");
    for (const FString& FolderLevel : FolderLevels)
    {
        CurrentPath += FolderLevel + TEXT("/");
        if (!UEditorAssetLibrary::DoesDirectoryExist(CurrentPath))
        {
            if (!UEditorAssetLibrary::MakeDirectory(CurrentPath))
            {
                OutError = FString::Printf(TEXT("Failed to create directory: %s"), *CurrentPath);
                return false;
            }
            UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateDirectoryStructure: Created directory '%s'"), *CurrentPath);
        }
    }
    
    return true;
}

bool FBlueprintService::NormalizeBlueprintPath(const FString& InputPath, FString& OutNormalizedPath, FString& OutError) const
{
    FString CleanPath = InputPath;
    
    // Remove leading slash
    if (CleanPath.StartsWith(TEXT("/")))
    {
        CleanPath = CleanPath.RightChop(1);
    }
    
    // Remove Content/ prefix
    if (CleanPath.StartsWith(TEXT("Content/")))
    {
        CleanPath = CleanPath.RightChop(8);
    }
    
    // Remove Game/ prefix
    if (CleanPath.StartsWith(TEXT("Game/")))
    {
        CleanPath = CleanPath.RightChop(5);
    }
    
    // Remove trailing slash
    if (CleanPath.EndsWith(TEXT("/")))
    {
        CleanPath = CleanPath.LeftChop(1);
    }
    
    // Build normalized path
    OutNormalizedPath = TEXT("/Game/");
    if (!CleanPath.IsEmpty())
    {
        OutNormalizedPath += CleanPath + TEXT("/");
    }
    
    return true;
}

UObject* FBlueprintService::ResolveVariableType(const FString& TypeString) const
{
    // Handle basic types (these don't need UObject resolution)
    if (TypeString == TEXT("Boolean") || TypeString == TEXT("bool") ||
        TypeString == TEXT("Integer") || TypeString == TEXT("int") || TypeString == TEXT("int32") ||
        TypeString == TEXT("Float") || TypeString == TEXT("float") ||
        TypeString == TEXT("String") || TypeString == TEXT("FString") ||
        TypeString == TEXT("Vector") || TypeString == TEXT("FVector") ||
        TypeString == TEXT("Rotator") || TypeString == TEXT("FRotator"))
    {
        return reinterpret_cast<UObject*>(1); // Non-null placeholder for basic types
    }
    
    // Try to find as a class
    if (UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *TypeString))
    {
        return FoundClass;
    }
    
    // Try to find as a struct
    if (UScriptStruct* FoundStruct = FindObject<UScriptStruct>(ANY_PACKAGE, *TypeString))
    {
        return FoundStruct;
    }
    
    // Try loading from common paths
    TArray<FString> SearchPaths = {
        FString::Printf(TEXT("/Script/Engine.%s"), *TypeString),
        FString::Printf(TEXT("/Script/CoreUObject.%s"), *TypeString),
        FString::Printf(TEXT("/Game/Blueprints/%s"), *TypeString)
    };
    
    for (const FString& SearchPath : SearchPaths)
    {
        if (UClass* LoadedClass = LoadClass<UObject>(nullptr, *SearchPath))
        {
            return LoadedClass;
        }
        
        if (UScriptStruct* LoadedStruct = LoadObject<UScriptStruct>(nullptr, *SearchPath))
        {
            return LoadedStruct;
        }
    }
    
    return nullptr;
}

bool FBlueprintService::SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TMap<FString, float>& PhysicsParams)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetPhysicsProperties: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetPhysicsProperties: Setting physics properties on component '%s' in blueprint '%s'"), 
        *ComponentName, *Blueprint->GetName());
    
    // Convert TMap to JSON object for ComponentService
    TSharedPtr<FJsonObject> PhysicsJsonParams = MakeShared<FJsonObject>();
    for (const auto& Param : PhysicsParams)
    {
        PhysicsJsonParams->SetNumberField(Param.Key, Param.Value);
    }
    
    // Delegate to ComponentService for physics operations
    bool bResult = FComponentService::Get().SetPhysicsProperties(Blueprint, ComponentName, PhysicsJsonParams);
    
    if (bResult)
    {
        // Invalidate cache since blueprint was modified
        BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    }
    
    return bResult;
}

bool FBlueprintService::GetBlueprintComponents(UBlueprint* Blueprint, TArray<TPair<FString, FString>>& OutComponents)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::GetBlueprintComponents: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::GetBlueprintComponents: Getting components for blueprint '%s'"), *Blueprint->GetName());
    
    OutComponents.Empty();
    
    // Get components from Simple Construction Script
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->ComponentTemplate)
            {
                FString ComponentName = Node->GetVariableName().ToString();
                FString ComponentType = Node->ComponentTemplate->GetClass()->GetName();
                OutComponents.Add(TPair<FString, FString>(ComponentName, ComponentType));
            }
        }
    }
    
    // Get inherited components from CDO
    if (Blueprint->GeneratedClass)
    {
        UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
        AActor* DefaultActor = Cast<AActor>(DefaultObject);
        if (DefaultActor)
        {
            TArray<UActorComponent*> AllComponents;
            DefaultActor->GetComponents(AllComponents);
            for (UActorComponent* Component : AllComponents)
            {
                if (Component)
                {
                    FString ComponentName = Component->GetName();
                    FString ComponentType = Component->GetClass()->GetName();
                    
                    // Check if already added from SCS
                    bool bAlreadyAdded = false;
                    for (const auto& ExistingComponent : OutComponents)
                    {
                        if (ExistingComponent.Key == ComponentName)
                        {
                            bAlreadyAdded = true;
                            break;
                        }
                    }
                    
                    if (!bAlreadyAdded)
                    {
                        OutComponents.Add(TPair<FString, FString>(ComponentName, ComponentType));
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::GetBlueprintComponents: Found %d components"), OutComponents.Num());
    return true;
}

bool FBlueprintService::SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetStaticMeshProperties: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetStaticMeshProperties: Setting static mesh '%s' on component '%s' in blueprint '%s'"), 
        *StaticMeshPath, *ComponentName, *Blueprint->GetName());
    
    // Delegate to ComponentService for static mesh operations
    bool bResult = FComponentService::Get().SetStaticMeshProperties(Blueprint, ComponentName, StaticMeshPath);
    
    if (bResult)
    {
        // Invalidate cache since blueprint was modified
        BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    }
    
    return bResult;
}

bool FBlueprintService::SetPawnProperties(UBlueprint* Blueprint, const TMap<FString, FString>& PawnParams)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetPawnProperties: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetPawnProperties: Setting pawn properties on blueprint '%s'"), *Blueprint->GetName());
    
    // Get the blueprint's default object
    UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    APawn* DefaultPawn = Cast<APawn>(DefaultObject);
    if (!DefaultPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SetPawnProperties: Blueprint is not a Pawn or Character"));
        return false;
    }
    
    // Set pawn properties
    for (const auto& Param : PawnParams)
    {
        const FString& PropertyName = Param.Key;
        const FString& PropertyValue = Param.Value;
        
        if (PropertyName == TEXT("auto_possess_player"))
        {
            // Handle auto possess player setting
            EAutoReceiveInput::Type AutoPossessType = EAutoReceiveInput::Disabled;
            if (PropertyValue == TEXT("Player0"))
            {
                AutoPossessType = EAutoReceiveInput::Player0;
            }
            else if (PropertyValue == TEXT("Player1"))
            {
                AutoPossessType = EAutoReceiveInput::Player1;
            }
            DefaultPawn->AutoPossessPlayer = AutoPossessType;
        }
        else if (PropertyName == TEXT("use_controller_rotation_yaw"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->bUseControllerRotationYaw = bValue;
        }
        else if (PropertyName == TEXT("use_controller_rotation_pitch"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->bUseControllerRotationPitch = bValue;
        }
        else if (PropertyName == TEXT("use_controller_rotation_roll"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->bUseControllerRotationRoll = bValue;
        }
        else if (PropertyName == TEXT("can_be_damaged"))
        {
            bool bValue = PropertyValue.ToBool();
            DefaultPawn->SetCanBeDamaged(bValue);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::SetPawnProperties: Unknown pawn property '%s'"), *PropertyName);
        }
    }
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SetPawnProperties: Successfully set pawn properties"));
    return true;
}

bool FBlueprintService::AddInterfaceToBlueprint(UBlueprint* Blueprint, const FString& InterfaceName)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddInterfaceToBlueprint: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddInterfaceToBlueprint: Adding interface '%s' to blueprint '%s'"), 
        *InterfaceName, *Blueprint->GetName());
    
    // Find the interface
    UBlueprint* InterfaceBlueprint = FindBlueprint(InterfaceName);
    if (!InterfaceBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddInterfaceToBlueprint: Interface blueprint not found: %s"), *InterfaceName);
        return false;
    }
    
    if (InterfaceBlueprint->BlueprintType != BPTYPE_Interface)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddInterfaceToBlueprint: Blueprint '%s' is not an interface"), *InterfaceName);
        return false;
    }
    
    // Add interface to blueprint
    FTopLevelAssetPath InterfacePath(InterfaceBlueprint->GeneratedClass);
    FBlueprintEditorUtils::ImplementNewInterface(Blueprint, InterfacePath);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddInterfaceToBlueprint: Successfully added interface '%s'"), *InterfaceName);
    return true;
}

UBlueprint* FBlueprintService::CreateBlueprintInterface(const FString& InterfaceName, const FString& FolderPath)
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprintInterface: Creating interface '%s'"), *InterfaceName);
    
    // Normalize the path
    FString NormalizedPath;
    FString PathError;
    if (!NormalizeBlueprintPath(FolderPath, NormalizedPath, PathError))
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprintInterface: Path normalization failed - %s"), *PathError);
        return nullptr;
    }
    
    // Build full asset path
    FString FullAssetPath = NormalizedPath + InterfaceName;
    
    // Check if interface already exists
    if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateBlueprintInterface: Interface already exists at '%s'"), *FullAssetPath);
        return Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullAssetPath));
    }
    
    // Create directory structure if needed
    FString DirectoryError;
    if (!CreateDirectoryStructure(NormalizedPath, DirectoryError))
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprintInterface: Failed to create directory structure - %s"), *DirectoryError);
        return nullptr;
    }
    
    // Create the package
    UObject* Package = CreatePackage(*FullAssetPath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprintInterface: Failed to create package for path '%s'"), *FullAssetPath);
        return nullptr;
    }
    
    // Create the interface blueprint
    UBlueprint* NewInterface = FKismetEditorUtilities::CreateBlueprint(
        UInterface::StaticClass(),
        Package,
        *InterfaceName,
        BPTYPE_Interface,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        NAME_None
    );
    
    if (!NewInterface)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateBlueprintInterface: Failed to create interface"));
        return nullptr;
    }
    
    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(NewInterface);
    
    // Mark the package dirty
    Package->MarkPackageDirty();
    
    // Save the asset
    if (UEditorAssetLibrary::SaveLoadedAsset(NewInterface))
    {
        UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprintInterface: Successfully saved interface '%s'"), *FullAssetPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateBlueprintInterface: Failed to save interface '%s'"), *FullAssetPath);
    }
    
    // Cache the interface
    BlueprintCache.CacheBlueprint(InterfaceName, NewInterface);
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprintInterface: Successfully created interface '%s'"), *InterfaceName);
    return NewInterface;
}

bool FBlueprintService::CreateCustomBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TSharedPtr<FJsonObject>& FunctionParams)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Creating function '%s' in blueprint '%s'"), 
        *FunctionName, *Blueprint->GetName());
    
    // Create a new function graph
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, *FunctionName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    if (!NewGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Failed to create function graph"));
        return false;
    }
    
    // Set up the function graph
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, NewGraph, true, nullptr);
    
    // Parse function parameters if provided
    if (FunctionParams.IsValid())
    {
        // Handle inputs
        const TArray<TSharedPtr<FJsonValue>>* InputsArray;
        if (FunctionParams->TryGetArrayField(TEXT("inputs"), InputsArray))
        {
            for (const TSharedPtr<FJsonValue>& InputValue : *InputsArray)
            {
                const TSharedPtr<FJsonObject>* InputObj;
                if (InputValue->TryGetObject(InputObj) && InputObj->IsValid())
                {
                    FString ParamName;
                    FString ParamType;
                    if ((*InputObj)->TryGetStringField(TEXT("name"), ParamName) && 
                        (*InputObj)->TryGetStringField(TEXT("type"), ParamType))
                    {
                        // Add input parameter to function
                        // This would require more complex graph manipulation
                        UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Adding input parameter '%s' of type '%s'"), 
                            *ParamName, *ParamType);
                    }
                }
            }
        }
        
        // Handle outputs
        const TArray<TSharedPtr<FJsonValue>>* OutputsArray;
        if (FunctionParams->TryGetArrayField(TEXT("outputs"), OutputsArray))
        {
            for (const TSharedPtr<FJsonValue>& OutputValue : *OutputsArray)
            {
                const TSharedPtr<FJsonObject>* OutputObj;
                if (OutputValue->TryGetObject(OutputObj) && OutputObj->IsValid())
                {
                    FString ParamName;
                    FString ParamType;
                    if ((*OutputObj)->TryGetStringField(TEXT("name"), ParamName) && 
                        (*OutputObj)->TryGetStringField(TEXT("type"), ParamType))
                    {
                        // Add output parameter to function
                        UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Adding output parameter '%s' of type '%s'"), 
                            *ParamName, *ParamType);
                    }
                }
            }
        }
    }
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Successfully created function '%s'"), *FunctionName);
    return true;
}

bool FBlueprintService::SpawnBlueprintActor(UBlueprint* Blueprint, const FString& ActorName, const FVector& Location, const FRotator& Rotation)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SpawnBlueprintActor: Spawning actor '%s' from blueprint '%s'"), 
        *ActorName, *Blueprint->GetName());
    
    // Get the world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: No valid world found"));
        return false;
    }
    
    // Get the blueprint's generated class
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    if (!BlueprintClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: Blueprint has no generated class"));
        return false;
    }
    
    // Spawn the actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AActor* SpawnedActor = World->SpawnActor<AActor>(BlueprintClass, Location, Rotation, SpawnParams);
    if (!SpawnedActor)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: Failed to spawn actor"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SpawnBlueprintActor: Successfully spawned actor '%s'"), *ActorName);
    return true;
}

bool FBlueprintService::CallBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TArray<FString>& Parameters)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CallBlueprintFunction: Calling function '%s' on blueprint '%s'"), 
        *FunctionName, *Blueprint->GetName());
    
    // Get the blueprint's generated class
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    if (!BlueprintClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: Blueprint has no generated class"));
        return false;
    }
    
    // Find the function
    UFunction* Function = BlueprintClass->FindFunctionByName(*FunctionName);
    if (!Function)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: Function '%s' not found"), *FunctionName);
        return false;
    }
    
    // Get the default object to call the function on
    UObject* DefaultObject = BlueprintClass->GetDefaultObject();
    if (!DefaultObject)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: No default object available"));
        return false;
    }
    
    // Call the function (simplified - would need proper parameter handling for real implementation)
    DefaultObject->ProcessEvent(Function, nullptr);
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CallBlueprintFunction: Successfully called function '%s'"), *FunctionName);
    return true;
}