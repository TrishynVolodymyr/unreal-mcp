#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Dom/JsonObject.h"

/**
 * Parameters for actor spawning operations
 */
struct UNREALMCP_API FActorSpawnParams
{
    /** Name of the actor to spawn */
    FString Name;

    /** Type of actor to spawn
     *  Supports:
     *  - Friendly names: StaticMeshActor, PointLight, TriggerBox, PlayerStart, etc.
     *  - Blueprint paths: "Blueprint:/Game/Path/BP_Name"
     *  - Native class paths: "Class:/Script/Engine.TriggerBox"
     *  - Direct paths: "/Game/Path/BP_Name" (tries Blueprint then Class)
     */
    FString Type;

    /** Location to spawn the actor */
    FVector Location = FVector::ZeroVector;

    /** Rotation of the spawned actor */
    FRotator Rotation = FRotator::ZeroRotator;

    /** Scale of the spawned actor */
    FVector Scale = FVector::OneVector;

    // ============================================
    // StaticMeshActor parameters
    // ============================================

    /** Path to mesh asset (e.g., "/Engine/BasicShapes/Cube") */
    FString MeshPath;

    // ============================================
    // TextRenderActor parameters
    // ============================================

    /** Text content to display */
    FString TextContent;

    /** Text world size */
    float TextSize = 100.0f;

    /** Text color (RGBA) */
    FLinearColor TextColor = FLinearColor::White;

    /** Horizontal text alignment (0=Left, 1=Center, 2=Right) */
    int32 TextHAlign = 1;

    /** Vertical text alignment (0=Top, 1=Center, 2=Bottom) */
    int32 TextVAlign = 1;

    // ============================================
    // Volume parameters (TriggerBox, BlockingVolume, etc.)
    // ============================================

    /** Box extent for box-shaped volumes */
    FVector BoxExtent = FVector(100.0f);

    /** Sphere radius for sphere-shaped volumes */
    float SphereRadius = 100.0f;

    // ============================================
    // PlayerStart parameters
    // ============================================

    /** Tag for player start selection */
    FString PlayerStartTag;

    // ============================================
    // DecalActor parameters
    // ============================================

    /** Decal size (X, Y, Z) */
    FVector DecalSize = FVector(128.0f, 256.0f, 256.0f);

    /** Path to decal material */
    FString DecalMaterialPath;

    // ============================================
    // InvisibleWall / Collision parameters
    // ============================================

    /** Whether the actor should be hidden in game (useful for invisible walls) */
    bool bHiddenInGame = false;

    /** Whether the actor should block all collision (BlockAll profile) */
    bool bBlocksAll = false;

    /** Whether to show collision wireframe in editor (for invisible walls) */
    bool bShowCollisionInEditor = true;

    /** Default constructor */
    FActorSpawnParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for Blueprint actor spawning operations
 */
struct UNREALMCP_API FBlueprintActorSpawnParams
{
    /** Name of the Blueprint to spawn from */
    FString BlueprintName;
    
    /** Name of the actor instance */
    FString ActorName;
    
    /** Location to spawn the actor */
    FVector Location = FVector::ZeroVector;
    
    /** Rotation of the spawned actor */
    FRotator Rotation = FRotator::ZeroRotator;
    
    /** Scale of the spawned actor */
    FVector Scale = FVector::OneVector;
    
    /** Default constructor */
    FBlueprintActorSpawnParams() = default;
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Interface for Editor service operations
 * Provides abstraction for actor manipulation, viewport control, and asset discovery
 */
class UNREALMCP_API IEditorService
{
public:
    virtual ~IEditorService() = default;
    
    /**
     * Get all actors in the current level
     * @return Array of actors in the level
     */
    virtual TArray<AActor*> GetActorsInLevel() = 0;
    
    /**
     * Find actors by name pattern
     * @param Pattern - Name pattern to search for
     * @return Array of matching actors
     */
    virtual TArray<AActor*> FindActorsByName(const FString& Pattern) = 0;
    
    /**
     * Spawn a new actor
     * @param Params - Actor spawn parameters
     * @param OutError - Error message if spawning fails
     * @return Spawned actor or nullptr if failed
     */
    virtual AActor* SpawnActor(const FActorSpawnParams& Params, FString& OutError) = 0;
    
    /**
     * Spawn a Blueprint actor
     * @param Params - Blueprint actor spawn parameters
     * @param OutError - Error message if spawning fails
     * @return Spawned actor or nullptr if failed
     */
    virtual AActor* SpawnBlueprintActor(const FBlueprintActorSpawnParams& Params, FString& OutError) = 0;
    
    /**
     * Delete an actor by name
     * @param ActorName - Name of the actor to delete
     * @param OutError - Error message if deletion fails
     * @return true if actor was deleted successfully
     */
    virtual bool DeleteActor(const FString& ActorName, FString& OutError) = 0;
    
    /**
     * Find an actor by name
     * @param ActorName - Name of the actor to find
     * @return Found actor or nullptr
     */
    virtual AActor* FindActorByName(const FString& ActorName) = 0;
    
    /**
     * Set actor transform
     * @param Actor - Target actor
     * @param Location - New location (optional)
     * @param Rotation - New rotation (optional)
     * @param Scale - New scale (optional)
     * @return true if transform was set successfully
     */
    virtual bool SetActorTransform(AActor* Actor, const FVector* Location = nullptr, const FRotator* Rotation = nullptr, const FVector* Scale = nullptr) = 0;
    
    /**
     * Set a property on an actor
     * @param Actor - Target actor
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value to set (as JSON)
     * @param OutError - Error message if setting fails
     * @return true if property was set successfully
     */
    virtual bool SetActorProperty(AActor* Actor, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;
    
    /**
     * Set a property on a light component
     * @param Actor - Actor containing the light component
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value to set
     * @param OutError - Error message if setting fails
     * @return true if property was set successfully
     */
    virtual bool SetLightProperty(AActor* Actor, const FString& PropertyName, const FString& PropertyValue, FString& OutError) = 0;
    
    /**
     * Focus viewport on a target
     * @param TargetActor - Actor to focus on (optional)
     * @param Location - Location to focus on (optional)
     * @param Distance - Distance from target
     * @param Orientation - Camera orientation (optional)
     * @param OutError - Error message if focusing fails
     * @return true if viewport was focused successfully
     */
    virtual bool FocusViewport(AActor* TargetActor = nullptr, const FVector* Location = nullptr, float Distance = 1000.0f, const FRotator* Orientation = nullptr, FString* OutError = nullptr) = 0;
    
    /**
     * Take a screenshot
     * @param FilePath - Path to save the screenshot
     * @param OutError - Error message if screenshot fails
     * @return true if screenshot was taken successfully
     */
    virtual bool TakeScreenshot(const FString& FilePath, FString& OutError) = 0;
    
    /**
     * Find assets by type
     * @param AssetType - Type of assets to find
     * @param SearchPath - Path to search in
     * @return Array of asset paths
     */
    virtual TArray<FString> FindAssetsByType(const FString& AssetType, const FString& SearchPath = TEXT("/Game")) = 0;
    
    /**
     * Find assets by name
     * @param AssetName - Name pattern to search for
     * @param SearchPath - Path to search in
     * @return Array of asset paths
     */
    virtual TArray<FString> FindAssetsByName(const FString& AssetName, const FString& SearchPath = TEXT("/Game")) = 0;
    
    /**
     * Find Widget Blueprints
     * @param WidgetName - Widget name pattern (optional)
     * @param SearchPath - Path to search in
     * @return Array of widget blueprint paths
     */
    virtual TArray<FString> FindWidgetBlueprints(const FString& WidgetName = TEXT(""), const FString& SearchPath = TEXT("/Game")) = 0;
    
    /**
     * Find Blueprints
     * @param BlueprintName - Blueprint name pattern (optional)
     * @param SearchPath - Path to search in
     * @return Array of blueprint paths
     */
    virtual TArray<FString> FindBlueprints(const FString& BlueprintName = TEXT(""), const FString& SearchPath = TEXT("/Game")) = 0;
    
    /**
     * Find Data Tables
     * @param TableName - Table name pattern (optional)
     * @param SearchPath - Path to search in
     * @return Array of data table paths
     */
    virtual TArray<FString> FindDataTables(const FString& TableName = TEXT(""), const FString& SearchPath = TEXT("/Game")) = 0;
};
