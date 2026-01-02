#pragma once

#include "CoreMinimal.h"
#include "Services/IMaterialService.h"

/**
 * Concrete implementation of IMaterialService
 * Provides Material operations with proper error handling and logging
 */
class UNREALMCP_API FMaterialService : public IMaterialService
{
public:
    /** Constructor */
    FMaterialService();

    /** Destructor */
    virtual ~FMaterialService() = default;

    /**
     * Get singleton instance
     * @return Reference to the singleton instance
     */
    static FMaterialService& Get();

    // IMaterialService interface implementation
    virtual UMaterial* CreateMaterial(const FMaterialCreationParams& Params, FString& OutMaterialPath, FString& OutError) override;
    virtual UMaterialInterface* CreateMaterialInstance(const FMaterialInstanceCreationParams& Params, FString& OutInstancePath, FString& OutError) override;
    virtual UMaterialInterface* FindMaterial(const FString& MaterialPath) override;
    virtual bool GetMaterialMetadata(const FString& MaterialPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata) override;
    virtual bool SetScalarParameter(const FString& MaterialPath, const FString& ParameterName, float Value, FString& OutError) override;
    virtual bool SetVectorParameter(const FString& MaterialPath, const FString& ParameterName, const FLinearColor& Value, FString& OutError) override;
    virtual bool SetTextureParameter(const FString& MaterialPath, const FString& ParameterName, const FString& TexturePath, FString& OutError) override;
    virtual bool GetScalarParameter(const FString& MaterialPath, const FString& ParameterName, float& OutValue, FString& OutError) override;
    virtual bool GetVectorParameter(const FString& MaterialPath, const FString& ParameterName, FLinearColor& OutValue, FString& OutError) override;
    virtual bool GetTextureParameter(const FString& MaterialPath, const FString& ParameterName, FString& OutTexturePath, FString& OutError) override;
    virtual bool ApplyMaterialToActor(const FString& ActorName, const FString& MaterialPath, int32 SlotIndex, const FString& ComponentName, FString& OutError) override;

private:
    /** Singleton instance */
    static TUniquePtr<FMaterialService> Instance;

    /**
     * Convert blend mode string to EBlendMode enum
     * @param BlendModeString - String representation of blend mode
     * @return Blend mode enum value
     */
    EBlendMode GetBlendModeFromString(const FString& BlendModeString) const;

    /**
     * Convert shading model string to EMaterialShadingModel enum
     * @param ShadingModelString - String representation of shading model
     * @return Shading model enum value
     */
    EMaterialShadingModel GetShadingModelFromString(const FString& ShadingModelString) const;

    /**
     * Get blend mode as string
     * @param BlendMode - Blend mode enum value
     * @return String representation
     */
    FString GetBlendModeString(EBlendMode BlendMode) const;

    /**
     * Get shading model as string
     * @param ShadingModel - Shading model enum value
     * @return String representation
     */
    FString GetShadingModelString(EMaterialShadingModel ShadingModel) const;

    /**
     * Get material type as string
     * @param Material - Material interface to check
     * @return "Material", "MaterialInstanceConstant", or "MaterialInstanceDynamic"
     */
    FString GetMaterialTypeString(UMaterialInterface* Material) const;

    /**
     * Add parameter info to JSON object
     * @param Material - Material to get parameters from
     * @param OutMetadata - JSON object to add parameters to
     */
    void AddParameterInfoToMetadata(UMaterialInterface* Material, TSharedPtr<FJsonObject>& OutMetadata) const;

    /**
     * Find an actor by name in the editor world
     * @param ActorName - Name of the actor to find
     * @return Actor or nullptr if not found
     */
    AActor* FindActorByName(const FString& ActorName) const;

    /**
     * Get mesh component from actor
     * @param Actor - Actor to search
     * @param ComponentName - Optional specific component name
     * @return Mesh component or nullptr if not found
     */
    class UMeshComponent* GetMeshComponent(AActor* Actor, const FString& ComponentName) const;
};
