#include "Services/BlueprintService.h"
#include "Services/ComponentService.h"
#include "Services/PropertyService.h"
#include "Utils/UnrealMCPCommonUtils.h"
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
#include "K2Node_FunctionEntry.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_FunctionResult.h"
#include "Engine/UserDefinedStruct.h"
#include "EdGraphSchema_K2.h"
#include "UObject/StructOnScope.h"
#include "Engine/Engine.h"

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
    
    // Store pre-compilation status for comparison
    EBlueprintStatus PreCompileStatus = Blueprint->Status;
    
    // Clear any existing compilation errors and reset status
    Blueprint->Status = BS_Unknown;
    
    // Clear any existing compilation state
    Blueprint->bIsRegeneratingOnLoad = false;
    
    // Compile the blueprint
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    
    // Log the compilation status for debugging
    FString StatusName;
    switch (Blueprint->Status)
    {
        case BS_Unknown: StatusName = TEXT("BS_Unknown"); break;
        case BS_Dirty: StatusName = TEXT("BS_Dirty"); break;
        case BS_Error: StatusName = TEXT("BS_Error"); break;
        case BS_UpToDate: StatusName = TEXT("BS_UpToDate"); break;
        case BS_BeingCreated: StatusName = TEXT("BS_BeingCreated"); break;
        case BS_UpToDateWithWarnings: StatusName = TEXT("BS_UpToDateWithWarnings"); break;
        default: StatusName = FString::Printf(TEXT("Unknown(%d)"), (int32)Blueprint->Status); break;
    }
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CompileBlueprint: Post-compilation status: %s (%d)"), *StatusName, (int32)Blueprint->Status);
    
    // Check compilation result and provide detailed error information
    if (Blueprint->Status == BS_Error)
    {
        TArray<FString> DetailedErrors;
        
        // Add basic error information
        DetailedErrors.Add(FString::Printf(TEXT("Blueprint '%s' failed to compile"), *Blueprint->GetName()));
        
        // Check for common issues
        if (!Blueprint->ParentClass)
        {
            DetailedErrors.Add(TEXT("Missing parent class"));
        }
        
        if (Blueprint->UbergraphPages.Num() == 0 && Blueprint->FunctionGraphs.Num() == 0)
        {
            DetailedErrors.Add(TEXT("Blueprint has no graphs"));
        }
        
        // Create comprehensive error message
        if (DetailedErrors.Num() > 0)
        {
            OutError = FString::Printf(TEXT("Blueprint compilation failed with %d error(s):\n%s"), 
                DetailedErrors.Num(), 
                *FString::Join(DetailedErrors, TEXT("\n")));
        }
        else
        {
            OutError = TEXT("Blueprint compilation failed with unknown errors");
        }
        
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CompileBlueprint: Compilation failed for blueprint '%s' - %s"), 
            *Blueprint->GetName(), *OutError);
        
        return false;
    }
    else if (Blueprint->Status == BS_UpToDateWithWarnings)
    {
        OutError = FString::Printf(TEXT("Blueprint '%s' compiled with warnings"), *Blueprint->GetName());
        
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CompileBlueprint: Blueprint '%s' compiled with warnings"), *Blueprint->GetName());
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
    
    // Get optional parameters
    bool bIsPure = false;
    if (FunctionParams.IsValid())
    {
        FunctionParams->TryGetBoolField(TEXT("is_pure"), bIsPure);
    }
    
    FString Category = TEXT("Default");
    if (FunctionParams.IsValid())
    {
        FunctionParams->TryGetStringField(TEXT("category"), Category);
    }
    
    // Check if a function graph with this name already exists
    UEdGraph* ExistingGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            ExistingGraph = Graph;
            break;
        }
    }
    
    if (ExistingGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Function '%s' already exists in Blueprint '%s'"), *FunctionName, *Blueprint->GetName());
        return false;
    }
    
    // Create the function graph using the working UMG pattern
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    
    if (!FuncGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Failed to create function graph"));
        return false;
    }
    
    // Add the function graph to the blueprint as a user-defined function
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, bIsPure, nullptr);
    
    // Mark this as a user-defined function to make it editable
    FuncGraph->Schema = UEdGraphSchema_K2::StaticClass();
    FuncGraph->bAllowDeletion = true;
    FuncGraph->bAllowRenaming = true;
    
    // Find the automatically created function entry node instead of creating a new one
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        if (UK2Node_FunctionEntry* ExistingEntry = Cast<UK2Node_FunctionEntry>(Node))
        {
            EntryNode = ExistingEntry;
            break;
        }
    }
    
    if (!EntryNode)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Failed to find function entry node"));
        return false;
    }
    
    // Position the entry node
    EntryNode->NodePosX = 0;
    EntryNode->NodePosY = 0;
    
    // Make sure the function is marked as BlueprintCallable and user-defined
    EntryNode->SetExtraFlags(FUNC_BlueprintCallable);
    
    // Remove any flags that would make it non-editable
    EntryNode->ClearExtraFlags(FUNC_BlueprintEvent);
    
    // Set metadata to ensure it's treated as a user function
    EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_CallInEditor, FString(TEXT("true")));
    EntryNode->MetaData.RemoveMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly);
    
    // Ensure the function signature is properly set up for user editing
    EntryNode->bCanRenameNode = true;
    
    // Clear any existing user defined pins to avoid duplicates
    EntryNode->UserDefinedPins.Empty();
    
    // Process input parameters
    if (FunctionParams.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* InputsArray = nullptr;
        if (FunctionParams->TryGetArrayField(TEXT("inputs"), InputsArray))
        {
            for (const auto& InputValue : *InputsArray)
            {
                const TSharedPtr<FJsonObject>& InputObj = InputValue->AsObject();
                if (InputObj.IsValid())
                {
                    FString ParamName;
                    FString ParamType;
                    if (InputObj->TryGetStringField(TEXT("name"), ParamName) && InputObj->TryGetStringField(TEXT("type"), ParamType))
                    {
                        // Convert string to pin type
                        FEdGraphPinType PinType;
                        if (ConvertStringToPinType(ParamType, PinType))
                        {
                            // Add input parameter to function entry
                            EntryNode->UserDefinedPins.Add(MakeShared<FUserPinInfo>());
                            FUserPinInfo& NewPin = *EntryNode->UserDefinedPins.Last();
                            NewPin.PinName = FName(*ParamName);
                            NewPin.PinType = PinType;
                            NewPin.DesiredPinDirection = EGPD_Output; // Entry node outputs are function inputs
                            
                            UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Added input parameter '%s' of type '%s'"), 
                                *ParamName, *ParamType);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Unknown parameter type '%s' for input '%s'"), 
                                *ParamType, *ParamName);
                        }
                    }
                }
            }
        }
        
        // Process output parameters
        const TArray<TSharedPtr<FJsonValue>>* OutputsArray = nullptr;
        if (FunctionParams->TryGetArrayField(TEXT("outputs"), OutputsArray))
        {
            // Create only one function result node for all outputs
            UK2Node_FunctionResult* ResultNode = nullptr;
            if (OutputsArray->Num() > 0)
            {
                ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
                FuncGraph->AddNode(ResultNode, false, false);
                ResultNode->NodePosX = 400;
                ResultNode->NodePosY = 0;
                
                // Clear any existing user defined pins to avoid duplicates
                ResultNode->UserDefinedPins.Empty();
            }
            
            for (const auto& OutputValue : *OutputsArray)
            {
                const TSharedPtr<FJsonObject>& OutputObj = OutputValue->AsObject();
                if (OutputObj.IsValid() && ResultNode)
                {
                    FString ParamName;
                    FString ParamType;
                    if (OutputObj->TryGetStringField(TEXT("name"), ParamName) && OutputObj->TryGetStringField(TEXT("type"), ParamType))
                    {
                        // Convert string to pin type for output
                        FEdGraphPinType PinType;
                        if (ConvertStringToPinType(ParamType, PinType))
                        {
                            // Add output parameter to function result
                            ResultNode->UserDefinedPins.Add(MakeShared<FUserPinInfo>());
                            FUserPinInfo& NewPin = *ResultNode->UserDefinedPins.Last();
                            NewPin.PinName = FName(*ParamName);
                            NewPin.PinType = PinType;
                            NewPin.DesiredPinDirection = EGPD_Input; // Result node inputs are function outputs
                            
                            UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Added output parameter '%s' of type '%s'"), 
                                *ParamName, *ParamType);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Unknown parameter type '%s' for output '%s'"), 
                                *ParamType, *ParamName);
                        }
                    }
                }
            }
            
            // Allocate pins for result node after adding all outputs
            if (ResultNode)
            {
                ResultNode->AllocateDefaultPins();
                // Reconstruct the result node to immediately update the visual representation
                ResultNode->ReconstructNode();
            }
        }
    }
    
    // Allocate pins for entry node AFTER setting up user defined pins
    EntryNode->AllocateDefaultPins();
    // Reconstruct the entry node to immediately update the visual representation
    EntryNode->ReconstructNode();
    
    // Mark blueprint as structurally modified
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    
    // Force refresh of blueprint nodes and UI to ensure proper pin connectivity
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Successfully created function '%s' with parameters"), *FunctionName);
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

bool FBlueprintService::ConvertStringToPinType(const FString& TypeString, FEdGraphPinType& OutPinType) const
{
    // Clear the output pin type
    OutPinType = FEdGraphPinType();
    
    // Handle basic types
    if (TypeString == TEXT("Boolean") || TypeString == TEXT("Bool"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        return true;
    }
    else if (TypeString == TEXT("Integer") || TypeString == TEXT("Int") || TypeString == TEXT("Int32"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        return true;
    }
    else if (TypeString == TEXT("Float"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        return true;
    }
    else if (TypeString == TEXT("String"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        return true;
    }
    else if (TypeString == TEXT("Text"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        return true;
    }
    else if (TypeString == TEXT("Name"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
        return true;
    }
    else if (TypeString == TEXT("Vector"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        return true;
    }
    else if (TypeString == TEXT("Vector2D"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
        return true;
    }
    else if (TypeString == TEXT("Rotator"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        return true;
    }
    else if (TypeString == TEXT("Transform"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
        return true;
    }
    else if (TypeString == TEXT("Color"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FColor>::Get();
        return true;
    }
    else if (TypeString == TEXT("LinearColor"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
        return true;
    }
    else if (TypeString == TEXT("Byte"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
        return true;
    }
    else if (TypeString == TEXT("Object"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = UObject::StaticClass();
        return true;
    }
    else if (TypeString == TEXT("Actor"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = AActor::StaticClass();
        return true;
    }
    else if (TypeString == TEXT("Pawn"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = APawn::StaticClass();
        return true;
    }
    else if (TypeString == TEXT("PlayerController"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = APlayerController::StaticClass();
        return true;
    }
    
    // Handle array types (e.g., "String[]", "Integer[]")
    if (TypeString.EndsWith(TEXT("[]")))
    {
        FString BaseType = TypeString.LeftChop(2); // Remove "[]"
        FEdGraphPinType BasePinType;
        if (ConvertStringToPinType(BaseType, BasePinType))
        {
            OutPinType = BasePinType;
            OutPinType.ContainerType = EPinContainerType::Array;
            return true;
        }
    }
    
    // Try to find custom struct or class by name
    UObject* FoundType = ResolveVariableType(TypeString);
    if (FoundType)
    {
        if (UScriptStruct* Struct = Cast<UScriptStruct>(FoundType))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = Struct;
            return true;
        }
        else if (UClass* Class = Cast<UClass>(FoundType))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            OutPinType.PinSubCategoryObject = Class;
            return true;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::ConvertStringToPinType: Unknown type '%s'"), *TypeString);
    return false;
}
