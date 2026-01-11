#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"

/**
 * Parameters for creating a new Material asset
 */
struct UNREALMCP_API FMaterialCreationParams
{
    /** Name of the material to create */
    FString Name;

    /** Content path where the material should be created */
    FString Path = TEXT("/Game/Materials");

    /** Blend mode for the material */
    FString BlendMode = TEXT("Opaque");

    /** Shading model for the material */
    FString ShadingModel = TEXT("DefaultLit");

    /** Usage flags - enable shader compilation for specific use cases */
    bool bUsedWithNiagaraSprites = false;
    bool bUsedWithNiagaraRibbons = false;
    bool bUsedWithNiagaraMeshParticles = false;
    bool bUsedWithParticleSprites = false;
    bool bUsedWithMeshParticles = false;
    bool bUsedWithSkeletalMesh = false;
    bool bUsedWithStaticLighting = false;

    /** Default constructor */
    FMaterialCreationParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (Name.IsEmpty())
        {
            OutError = TEXT("Material name cannot be empty");
            return false;
        }
        if (Path.IsEmpty())
        {
            OutError = TEXT("Material path cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating a Material Instance
 */
struct UNREALMCP_API FMaterialInstanceCreationParams
{
    /** Name of the material instance to create */
    FString Name;

    /** Path to the parent material */
    FString ParentMaterialPath;

    /** Content path where the instance should be created */
    FString Path = TEXT("/Game/Materials");

    /** Whether to create a dynamic (runtime modifiable) instance */
    bool bIsDynamic = false;

    /** Default constructor */
    FMaterialInstanceCreationParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (Name.IsEmpty())
        {
            OutError = TEXT("Material instance name cannot be empty");
            return false;
        }
        if (ParentMaterialPath.IsEmpty())
        {
            OutError = TEXT("Parent material path cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for setting a material parameter
 */
struct UNREALMCP_API FMaterialParameterSetParams
{
    /** Path to the material or material instance */
    FString MaterialPath;

    /** Name of the parameter to set */
    FString ParameterName;

    /** Type of parameter (scalar, vector, texture) */
    FString ParameterType;

    /** Value to set (stored as JSON for flexibility) */
    TSharedPtr<FJsonValue> Value;

    /** Default constructor */
    FMaterialParameterSetParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (MaterialPath.IsEmpty())
        {
            OutError = TEXT("Material path cannot be empty");
            return false;
        }
        if (ParameterName.IsEmpty())
        {
            OutError = TEXT("Parameter name cannot be empty");
            return false;
        }
        if (ParameterType.IsEmpty())
        {
            OutError = TEXT("Parameter type cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Interface for Material service operations
 * Provides abstraction for material creation, modification, and parameter management
 */
class UNREALMCP_API IMaterialService
{
public:
    virtual ~IMaterialService() = default;

    /**
     * Create a new Material asset
     * @param Params - Material creation parameters
     * @param OutMaterialPath - Output path of the created material
     * @param OutError - Error message if creation fails
     * @return Created material or nullptr if failed
     */
    virtual UMaterial* CreateMaterial(const FMaterialCreationParams& Params, FString& OutMaterialPath, FString& OutError) = 0;

    /**
     * Create a Material Instance
     * @param Params - Material instance creation parameters
     * @param OutInstancePath - Output path of the created instance
     * @param OutError - Error message if creation fails
     * @return Created material interface or nullptr if failed
     */
    virtual UMaterialInterface* CreateMaterialInstance(const FMaterialInstanceCreationParams& Params, FString& OutInstancePath, FString& OutError) = 0;

    /**
     * Find a material by path
     * @param MaterialPath - Path to the material
     * @return Material interface or nullptr if not found
     */
    virtual UMaterialInterface* FindMaterial(const FString& MaterialPath) = 0;

    /**
     * Get metadata about a material
     * @param MaterialPath - Path to the material
     * @param Fields - Optional fields to include (nullptr = all)
     * @param OutMetadata - Output JSON object with metadata
     * @return true if metadata was retrieved successfully
     */
    virtual bool GetMaterialMetadata(const FString& MaterialPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata) = 0;

    /**
     * Set a scalar parameter on a material instance
     * @param MaterialPath - Path to the material instance
     * @param ParameterName - Name of the parameter
     * @param Value - Value to set
     * @param OutError - Error message if setting fails
     * @return true if parameter was set successfully
     */
    virtual bool SetScalarParameter(const FString& MaterialPath, const FString& ParameterName, float Value, FString& OutError) = 0;

    /**
     * Set a vector parameter on a material instance
     * @param MaterialPath - Path to the material instance
     * @param ParameterName - Name of the parameter
     * @param Value - Value to set (RGBA)
     * @param OutError - Error message if setting fails
     * @return true if parameter was set successfully
     */
    virtual bool SetVectorParameter(const FString& MaterialPath, const FString& ParameterName, const FLinearColor& Value, FString& OutError) = 0;

    /**
     * Set a texture parameter on a material instance
     * @param MaterialPath - Path to the material instance
     * @param ParameterName - Name of the parameter
     * @param TexturePath - Path to the texture asset
     * @param OutError - Error message if setting fails
     * @return true if parameter was set successfully
     */
    virtual bool SetTextureParameter(const FString& MaterialPath, const FString& ParameterName, const FString& TexturePath, FString& OutError) = 0;

    /**
     * Get a scalar parameter value from a material
     * @param MaterialPath - Path to the material
     * @param ParameterName - Name of the parameter
     * @param OutValue - Output value
     * @param OutError - Error message if getting fails
     * @return true if parameter was retrieved successfully
     */
    virtual bool GetScalarParameter(const FString& MaterialPath, const FString& ParameterName, float& OutValue, FString& OutError) = 0;

    /**
     * Get a vector parameter value from a material
     * @param MaterialPath - Path to the material
     * @param ParameterName - Name of the parameter
     * @param OutValue - Output value (RGBA)
     * @param OutError - Error message if getting fails
     * @return true if parameter was retrieved successfully
     */
    virtual bool GetVectorParameter(const FString& MaterialPath, const FString& ParameterName, FLinearColor& OutValue, FString& OutError) = 0;

    /**
     * Get a texture parameter value from a material
     * @param MaterialPath - Path to the material
     * @param ParameterName - Name of the parameter
     * @param OutTexturePath - Output texture path
     * @param OutError - Error message if getting fails
     * @return true if parameter was retrieved successfully
     */
    virtual bool GetTextureParameter(const FString& MaterialPath, const FString& ParameterName, FString& OutTexturePath, FString& OutError) = 0;

    /**
     * Apply a material to an actor's mesh component
     * @param ActorName - Name of the actor
     * @param MaterialPath - Path to the material
     * @param SlotIndex - Material slot index
     * @param ComponentName - Optional specific component name
     * @param OutError - Error message if applying fails
     * @return true if material was applied successfully
     */
    virtual bool ApplyMaterialToActor(const FString& ActorName, const FString& MaterialPath, int32 SlotIndex, const FString& ComponentName, FString& OutError) = 0;

    /**
     * Duplicate a material instance to create a variation
     * @param SourcePath - Path to the source material instance
     * @param NewName - Name for the new duplicated instance
     * @param FolderPath - Optional folder path (uses source folder if empty)
     * @param OutAssetPath - Output path of the created instance
     * @param OutParentMaterial - Output path of the parent material
     * @param OutError - Error message if duplication fails
     * @return true if material instance was duplicated successfully
     */
    virtual bool DuplicateMaterialInstance(const FString& SourcePath, const FString& NewName, const FString& FolderPath, FString& OutAssetPath, FString& OutParentMaterial, FString& OutError) = 0;
};
