#pragma once

#include "CoreMinimal.h"
#include "Services/IEditorService.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

/**
 * Concrete implementation of IEditorService
 * Provides Editor operations with proper error handling and logging
 */
class UNREALMCP_API FEditorService : public IEditorService
{
public:
    /** Constructor */
    FEditorService();
    
    /** Destructor */
    virtual ~FEditorService() = default;
    
    /**
     * Get singleton instance
     * @return Reference to the singleton instance
     */
    static FEditorService& Get();
    
    // IEditorService interface implementation
    virtual TArray<AActor*> GetActorsInLevel() override;
    virtual TArray<AActor*> FindActorsByName(const FString& Pattern) override;
    virtual AActor* SpawnActor(const FActorSpawnParams& Params, FString& OutError) override;
    virtual AActor* SpawnBlueprintActor(const FBlueprintActorSpawnParams& Params, FString& OutError) override;
    virtual bool DeleteActor(const FString& ActorName, FString& OutError) override;
    virtual AActor* FindActorByName(const FString& ActorName) override;
    virtual bool SetActorTransform(AActor* Actor, const FVector* Location = nullptr, const FRotator* Rotation = nullptr, const FVector* Scale = nullptr) override;
    virtual bool SetActorProperty(AActor* Actor, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) override;
    virtual bool SetLightProperty(AActor* Actor, const FString& PropertyName, const FString& PropertyValue, FString& OutError) override;
    virtual bool FocusViewport(AActor* TargetActor = nullptr, const FVector* Location = nullptr, float Distance = 1000.0f, const FRotator* Orientation = nullptr, FString* OutError = nullptr) override;
    virtual bool TakeScreenshot(const FString& FilePath, FString& OutError) override;
    virtual TArray<FString> FindAssetsByType(const FString& AssetType, const FString& SearchPath = TEXT("/Game")) override;
    virtual TArray<FString> FindAssetsByName(const FString& AssetName, const FString& SearchPath = TEXT("/Game")) override;
    virtual TArray<FString> FindWidgetBlueprints(const FString& WidgetName = TEXT(""), const FString& SearchPath = TEXT("/Game")) override;
    virtual TArray<FString> FindBlueprints(const FString& BlueprintName = TEXT(""), const FString& SearchPath = TEXT("/Game")) override;
    virtual TArray<FString> FindDataTables(const FString& TableName = TEXT(""), const FString& SearchPath = TEXT("/Game")) override;

private:
    /** Singleton instance */
    static TUniquePtr<FEditorService> Instance;

    /**
     * Get the current editor world
     * @return Editor world or nullptr if not available
     */
    UWorld* GetEditorWorld() const;

    /**
     * Spawn actor of specific type
     * @param ActorClass - Class of actor to spawn
     * @param Name - Name for the actor
     * @param Location - Spawn location
     * @param Rotation - Spawn rotation
     * @param Scale - Spawn scale
     * @param Params - Full spawn parameters for type-specific configuration
     * @param OutError - Error message if spawning fails
     * @return Spawned actor or nullptr
     */
    AActor* SpawnActorOfType(UClass* ActorClass, const FString& Name, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FActorSpawnParams& Params, FString& OutError);

    /**
     * Configure spawned actor with type-specific settings
     * @param NewActor - The spawned actor to configure
     * @param Params - Parameters containing type-specific configuration
     */
    void ConfigureSpawnedActor(AActor* NewActor, const FActorSpawnParams& Params);

    /**
     * Get actor class from type string
     * Supports:
     *   - Friendly name aliases (StaticMeshActor, TriggerBox, etc.)
     *   - Blueprint paths: "Blueprint:/Game/Path/BP_Name"
     *   - Native class paths: "Class:/Script/Engine.TriggerBox"
     *   - Direct paths: "/Game/Path" (tries Blueprint then Class)
     * @param TypeString - String representation of actor type
     * @return Actor class or nullptr if not found
     */
    UClass* GetActorClassFromType(const FString& TypeString) const;
};
