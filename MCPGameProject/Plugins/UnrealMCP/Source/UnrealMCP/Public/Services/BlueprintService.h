#pragma once

#include "CoreMinimal.h"
#include "Services/IBlueprintService.h"
#include "Services/Blueprint/BlueprintCacheService.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"

// Forward declarations
class FBlueprintCreationService;
class FBlueprintPropertyService;
class FBlueprintFunctionService;

/**
 * Concrete implementation of IBlueprintService
 * Provides Blueprint creation, modification, and management functionality
 * with proper error handling, caching, and logging
 */
class UNREALMCP_API FBlueprintService : public IBlueprintService
{
public:
    /**
     * Get the singleton instance of the blueprint service
     * @return Reference to the singleton instance
     */
    static FBlueprintService& Get();
    
    // IBlueprintService interface implementation
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override;
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params, FString& OutErrorMessage) override;
    virtual bool CompileBlueprint(UBlueprint* Blueprint, FString& OutError) override;
    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) override;
    virtual bool AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, const FString& VariableType, bool bIsExposed = false) override;
    virtual bool SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutErrorMessage) override;
    virtual bool SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TMap<FString, float>& PhysicsParams) override;
    virtual bool GetBlueprintComponents(UBlueprint* Blueprint, TArray<TPair<FString, FString>>& OutComponents) override;
    virtual bool SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath) override;
    virtual bool SetPawnProperties(UBlueprint* Blueprint, const TMap<FString, FString>& PawnParams) override;
    virtual bool AddInterfaceToBlueprint(UBlueprint* Blueprint, const FString& InterfaceName) override;
    virtual UBlueprint* CreateBlueprintInterface(const FString& InterfaceName, const FString& FolderPath) override;
    virtual bool CreateCustomBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TSharedPtr<FJsonObject>& FunctionParams) override;
    virtual bool SpawnBlueprintActor(UBlueprint* Blueprint, const FString& ActorName, const FVector& Location, const FRotator& Rotation) override;
    virtual bool CallBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TArray<FString>& Parameters) override;

    /**
     * Convert string type to Blueprint pin type
     * Supports all Blueprint types including primitives, objects, structs, arrays, and custom types
     * @param TypeString - String representation of the type (e.g., "Actor", "Float", "Vector", "String[]", custom struct names)
     * @param OutPinType - Output pin type structure
     * @return true if conversion was successful
     */
    bool ConvertStringToPinType(const FString& TypeString, FEdGraphPinType& OutPinType) const;

    /** Invalidate the internal metadata cache for a specific blueprint */
    void InvalidateBlueprintCache(const FString& BlueprintName);

private:
    /** Private constructor for singleton pattern */
    FBlueprintService();

    /** Destructor */
    ~FBlueprintService();

    /** Blueprint cache for performance optimization */
    FBlueprintCache BlueprintCache;

    /** Blueprint creation service */
    TUniquePtr<FBlueprintCreationService> CreationService;

    /** Blueprint property service */
    TUniquePtr<FBlueprintPropertyService> PropertyService;

    /** Blueprint function service */
    TUniquePtr<FBlueprintFunctionService> FunctionService;
};
