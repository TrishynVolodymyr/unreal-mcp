#include "Services/Blueprint/BlueprintCreationService.h"
#include "Services/Blueprint/BlueprintCacheService.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"

UBlueprint* FBlueprintCreationService::CreateBlueprint(
    const FBlueprintCreationParams& Params,
    FBlueprintCache& Cache,
    TFunction<bool(UBlueprint*, FString&)> CompileFunc)
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
            Cache.CacheBlueprint(Params.Name, ExistingBlueprint);
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
        if (!CompileFunc(NewBlueprint, CompileError))
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
    Cache.CacheBlueprint(Params.Name, NewBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprint: Successfully created blueprint '%s'"), *Params.Name);
    return NewBlueprint;
}

UBlueprint* FBlueprintCreationService::CreateBlueprintInterface(const FString& InterfaceName, const FString& FolderPath, FBlueprintCache& Cache)
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
    Cache.CacheBlueprint(InterfaceName, NewInterface);

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateBlueprintInterface: Successfully created interface '%s'"), *InterfaceName);
    return NewInterface;
}

// Private helper methods
UClass* FBlueprintCreationService::ResolveParentClass(const FString& ParentClassName) const
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

bool FBlueprintCreationService::CreateDirectoryStructure(const FString& FolderPath, FString& OutError) const
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

bool FBlueprintCreationService::NormalizeBlueprintPath(const FString& InputPath, FString& OutNormalizedPath, FString& OutError) const
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
