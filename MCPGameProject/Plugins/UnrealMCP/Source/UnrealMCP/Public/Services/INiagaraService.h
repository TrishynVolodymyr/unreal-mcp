#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class UNiagaraSystem;
class UNiagaraEmitter;
class ANiagaraActor;

/**
 * Parameters for creating a Niagara System
 */
struct UNREALMCP_API FNiagaraSystemCreationParams
{
    /** Name of the system to create */
    FString Name;

    /** Content path where the system should be created */
    FString Path = TEXT("/Game/Niagara");

    /** Optional template system to copy from */
    FString Template;

    /** Default constructor */
    FNiagaraSystemCreationParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (Name.IsEmpty())
        {
            OutError = TEXT("System name cannot be empty");
            return false;
        }
        if (Path.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for creating a Niagara Emitter
 */
struct UNREALMCP_API FNiagaraEmitterCreationParams
{
    /** Name of the emitter to create */
    FString Name;

    /** Content path where the emitter should be created */
    FString Path = TEXT("/Game/Niagara");

    /** Optional template emitter to copy from */
    FString Template;

    /** Default constructor */
    FNiagaraEmitterCreationParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (Name.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (Path.IsEmpty())
        {
            OutError = TEXT("Emitter path cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding a module to an emitter
 */
struct UNREALMCP_API FNiagaraModuleAddParams
{
    /** Path to the system containing the emitter */
    FString SystemPath;

    /** Name of the target emitter within the system */
    FString EmitterName;

    /** Path to the module script to add */
    FString ModulePath;

    /** Stage to add the module to: "Spawn", "Update", or "Event" */
    FString Stage;

    /** Index position for the module (-1 for end) */
    int32 Index = -1;

    /** Default constructor */
    FNiagaraModuleAddParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (ModulePath.IsEmpty())
        {
            OutError = TEXT("Module path cannot be empty");
            return false;
        }
        if (Stage.IsEmpty())
        {
            OutError = TEXT("Stage cannot be empty");
            return false;
        }
        // Validate stage value
        if (Stage != TEXT("Spawn") && Stage != TEXT("Update") && Stage != TEXT("Event"))
        {
            OutError = FString::Printf(TEXT("Invalid stage '%s'. Must be 'Spawn', 'Update', or 'Event'"), *Stage);
            return false;
        }
        return true;
    }
};

/**
 * Parameters for moving a module within an emitter stack
 */
struct UNREALMCP_API FNiagaraModuleMoveParams
{
    /** Path to the system containing the emitter */
    FString SystemPath;

    /** Name of the emitter containing the module */
    FString EmitterName;

    /** Name of the module to move */
    FString ModuleName;

    /** Stage the module is currently in: "Spawn", "Update", or "Event" */
    FString Stage;

    /** New index position for the module (0-based) */
    int32 NewIndex = 0;

    /** Default constructor */
    FNiagaraModuleMoveParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (ModuleName.IsEmpty())
        {
            OutError = TEXT("Module name cannot be empty");
            return false;
        }
        if (Stage.IsEmpty())
        {
            OutError = TEXT("Stage cannot be empty");
            return false;
        }
        // Validate stage value
        if (Stage != TEXT("Spawn") && Stage != TEXT("Update") && Stage != TEXT("Event"))
        {
            OutError = FString::Printf(TEXT("Invalid stage '%s'. Must be 'Spawn', 'Update', or 'Event'"), *Stage);
            return false;
        }
        if (NewIndex < 0)
        {
            OutError = TEXT("New index must be >= 0");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for setting a module input
 */
struct UNREALMCP_API FNiagaraModuleInputParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the emitter */
    FString EmitterName;

    /** Name of the module */
    FString ModuleName;

    /** Stage the module is in */
    FString Stage;

    /** Name of the input to set */
    FString InputName;

    /** Value to set (as JSON for flexibility) */
    TSharedPtr<FJsonValue> Value;

    /** Type hint for the value (auto-detected if empty) */
    FString ValueType;

    /** Default constructor */
    FNiagaraModuleInputParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (ModuleName.IsEmpty())
        {
            OutError = TEXT("Module name cannot be empty");
            return false;
        }
        if (InputName.IsEmpty())
        {
            OutError = TEXT("Input name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding a Niagara parameter
 */
struct UNREALMCP_API FNiagaraParameterAddParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the parameter */
    FString ParameterName;

    /** Type of the parameter: "Float", "Int", "Bool", "Vector", "LinearColor" */
    FString ParameterType;

    /** Optional default value (as JSON) */
    TSharedPtr<FJsonValue> DefaultValue;

    /** Scope of the parameter: "user", "system", "emitter" */
    FString Scope = TEXT("user");

    /** Default constructor */
    FNiagaraParameterAddParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
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
 * Parameters for adding a data interface
 */
struct UNREALMCP_API FNiagaraDataInterfaceParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the emitter */
    FString EmitterName;

    /** Type of data interface to add */
    FString InterfaceType;

    /** Optional name for the data interface */
    FString InterfaceName;

    /** Default constructor */
    FNiagaraDataInterfaceParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (InterfaceType.IsEmpty())
        {
            OutError = TEXT("Interface type cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * A single keyframe for a curve input
 */
struct UNREALMCP_API FNiagaraCurveKeyframe
{
    /** Time position (normalized 0-1 for lifetime curves) */
    float Time = 0.0f;

    /** Value at this time */
    float Value = 0.0f;

    /** Default constructor */
    FNiagaraCurveKeyframe() = default;

    FNiagaraCurveKeyframe(float InTime, float InValue)
        : Time(InTime), Value(InValue) {}
};

/**
 * Parameters for setting a curve input on a module
 */
struct UNREALMCP_API FNiagaraModuleCurveInputParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the emitter */
    FString EmitterName;

    /** Name of the module */
    FString ModuleName;

    /** Stage the module is in */
    FString Stage;

    /** Name of the input to set */
    FString InputName;

    /** Curve keyframes */
    TArray<FNiagaraCurveKeyframe> Keyframes;

    /** Default constructor */
    FNiagaraModuleCurveInputParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (ModuleName.IsEmpty())
        {
            OutError = TEXT("Module name cannot be empty");
            return false;
        }
        if (InputName.IsEmpty())
        {
            OutError = TEXT("Input name cannot be empty");
            return false;
        }
        if (Keyframes.Num() < 2)
        {
            OutError = TEXT("Curve must have at least 2 keyframes");
            return false;
        }
        return true;
    }
};

/**
 * A single color keyframe for a color curve input
 */
struct UNREALMCP_API FNiagaraColorCurveKeyframe
{
    /** Time position (normalized 0-1 for lifetime curves) */
    float Time = 0.0f;

    /** Red value */
    float R = 1.0f;

    /** Green value */
    float G = 1.0f;

    /** Blue value */
    float B = 1.0f;

    /** Alpha value */
    float A = 1.0f;

    /** Default constructor */
    FNiagaraColorCurveKeyframe() = default;

    FNiagaraColorCurveKeyframe(float InTime, float InR, float InG, float InB, float InA)
        : Time(InTime), R(InR), G(InG), B(InB), A(InA) {}
};

/**
 * Parameters for setting a color curve input on a module
 */
struct UNREALMCP_API FNiagaraModuleColorCurveInputParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the emitter */
    FString EmitterName;

    /** Name of the module */
    FString ModuleName;

    /** Stage the module is in */
    FString Stage;

    /** Name of the input to set */
    FString InputName;

    /** Color curve keyframes */
    TArray<FNiagaraColorCurveKeyframe> Keyframes;

    /** Default constructor */
    FNiagaraModuleColorCurveInputParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (ModuleName.IsEmpty())
        {
            OutError = TEXT("Module name cannot be empty");
            return false;
        }
        if (InputName.IsEmpty())
        {
            OutError = TEXT("Input name cannot be empty");
            return false;
        }
        if (Keyframes.Num() < 2)
        {
            OutError = TEXT("Color curve must have at least 2 keyframes");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for setting a random range input on a module (Uniform Random Float/Vector)
 */
struct UNREALMCP_API FNiagaraModuleRandomInputParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the emitter */
    FString EmitterName;

    /** Name of the module */
    FString ModuleName;

    /** Stage the module is in */
    FString Stage;

    /** Name of the input to set */
    FString InputName;

    /** Minimum value (as string - supports float "1.0" or vector "0,0,100") */
    FString MinValue;

    /** Maximum value (as string - supports float "5.0" or vector "100,100,500") */
    FString MaxValue;

    /** Default constructor */
    FNiagaraModuleRandomInputParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (ModuleName.IsEmpty())
        {
            OutError = TEXT("Module name cannot be empty");
            return false;
        }
        if (InputName.IsEmpty())
        {
            OutError = TEXT("Input name cannot be empty");
            return false;
        }
        if (MinValue.IsEmpty())
        {
            OutError = TEXT("Min value cannot be empty");
            return false;
        }
        if (MaxValue.IsEmpty())
        {
            OutError = TEXT("Max value cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for adding a renderer
 */
struct UNREALMCP_API FNiagaraRendererParams
{
    /** Path to the system */
    FString SystemPath;

    /** Name of the emitter */
    FString EmitterName;

    /** Type of renderer: "Sprite", "Mesh", "Ribbon", "Light", "Decal", "Component" */
    FString RendererType;

    /** Optional name for the renderer */
    FString RendererName;

    /** Default constructor */
    FNiagaraRendererParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (EmitterName.IsEmpty())
        {
            OutError = TEXT("Emitter name cannot be empty");
            return false;
        }
        if (RendererType.IsEmpty())
        {
            OutError = TEXT("Renderer type cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for spawning a Niagara actor
 */
struct UNREALMCP_API FNiagaraActorSpawnParams
{
    /** Path to the Niagara system asset */
    FString SystemPath;

    /** Name for the spawned actor */
    FString ActorName;

    /** Spawn location */
    FVector Location = FVector::ZeroVector;

    /** Spawn rotation */
    FRotator Rotation = FRotator::ZeroRotator;

    /** Whether to auto-activate on spawn */
    bool bAutoActivate = true;

    /** Default constructor */
    FNiagaraActorSpawnParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (SystemPath.IsEmpty())
        {
            OutError = TEXT("System path cannot be empty");
            return false;
        }
        if (ActorName.IsEmpty())
        {
            OutError = TEXT("Actor name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Interface for Niagara VFX service operations
 * Provides abstraction for Niagara System/Emitter creation, modification, and management
 */
class UNREALMCP_API INiagaraService
{
public:
    virtual ~INiagaraService() = default;

    // ========================================================================
    // Core Asset Management (Feature 1)
    // ========================================================================

    /**
     * Create a new Niagara System asset
     * @param Params - System creation parameters
     * @param OutSystemPath - Output path of the created system
     * @param OutError - Error message if creation fails
     * @return Created system or nullptr if failed
     */
    virtual UNiagaraSystem* CreateSystem(const FNiagaraSystemCreationParams& Params, FString& OutSystemPath, FString& OutError) = 0;

    /**
     * Create a new Niagara Emitter asset
     * @param Params - Emitter creation parameters
     * @param OutEmitterPath - Output path of the created emitter
     * @param OutError - Error message if creation fails
     * @return Created emitter or nullptr if failed
     */
    virtual UNiagaraEmitter* CreateEmitter(const FNiagaraEmitterCreationParams& Params, FString& OutEmitterPath, FString& OutError) = 0;

    /**
     * Add an emitter to an existing system
     * @param SystemPath - Path to the system
     * @param EmitterPath - Path to the emitter to add
     * @param EmitterName - Optional custom name for the emitter handle
     * @param OutEmitterHandleId - Output GUID of the created emitter handle
     * @param OutError - Error message if adding fails
     * @return true if emitter was added successfully
     */
    virtual bool AddEmitterToSystem(const FString& SystemPath, const FString& EmitterPath, const FString& EmitterName, FGuid& OutEmitterHandleId, FString& OutError) = 0;

    /**
     * Enable or disable an emitter within a system
     * @param SystemPath - Path to the system containing the emitter
     * @param EmitterName - Name of the emitter to enable/disable
     * @param bEnabled - Whether to enable (true) or disable (false) the emitter
     * @param OutError - Error message if operation fails
     * @return true if emitter enabled state was changed successfully
     */
    virtual bool SetEmitterEnabled(const FString& SystemPath, const FString& EmitterName, bool bEnabled, FString& OutError) = 0;

    /**
     * Remove an emitter from a system
     * @param SystemPath - Path to the system containing the emitter
     * @param EmitterName - Name of the emitter to remove
     * @param OutError - Error message if operation fails
     * @return true if emitter was removed successfully
     */
    virtual bool RemoveEmitterFromSystem(const FString& SystemPath, const FString& EmitterName, FString& OutError) = 0;

    /**
     * Get metadata about a Niagara System or Emitter
     * @param AssetPath - Path to the asset
     * @param Fields - Optional fields to include (nullptr = all)
     * @param OutMetadata - Output JSON object with metadata
     * @param EmitterName - Optional emitter name filter (required for "modules" field)
     * @param Stage - Optional stage filter for "modules" field ("Spawn"|"Update"|"Render")
     * @return true if metadata was retrieved successfully
     */
    virtual bool GetMetadata(const FString& AssetPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata, const FString& EmitterName = TEXT(""), const FString& Stage = TEXT("")) = 0;

    /**
     * Get input values for a specific module
     * @param SystemPath - Path to the Niagara System
     * @param EmitterName - Name of the emitter containing the module
     * @param ModuleName - Name of the module
     * @param Stage - Stage the module is in ("Spawn", "Update", "Event")
     * @param OutInputs - Output JSON object with module inputs
     * @return true if inputs were retrieved successfully
     */
    virtual bool GetModuleInputs(const FString& SystemPath, const FString& EmitterName, const FString& ModuleName, const FString& Stage, TSharedPtr<FJsonObject>& OutInputs) = 0;

    /**
     * Compile a Niagara System or Emitter
     * @param AssetPath - Path to the asset to compile
     * @param OutError - Error message if compilation fails
     * @return true if compilation succeeded
     */
    virtual bool CompileAsset(const FString& AssetPath, FString& OutError) = 0;

    /**
     * Duplicate a Niagara System
     * @param SourcePath - Path to the source system
     * @param NewName - Name for the duplicated system
     * @param FolderPath - Optional folder path for the new system
     * @param OutNewPath - Output path of the duplicated system
     * @param OutError - Error message if duplication fails
     * @return true if duplication succeeded
     */
    virtual bool DuplicateSystem(const FString& SourcePath, const FString& NewName, const FString& FolderPath, FString& OutNewPath, FString& OutError) = 0;

    // ========================================================================
    // Module System (Feature 2)
    // ========================================================================

    /**
     * Add a module to an emitter stage
     * @param Params - Module add parameters
     * @param OutModuleId - Output identifier for the added module
     * @param OutError - Error message if adding fails
     * @return true if module was added successfully
     */
    virtual bool AddModule(const FNiagaraModuleAddParams& Params, FString& OutModuleId, FString& OutError) = 0;

    /**
     * Search for available Niagara modules
     * @param SearchQuery - Search string (empty for all)
     * @param StageFilter - Optional stage filter
     * @param MaxResults - Maximum results to return
     * @param OutModules - Output array of module info JSON objects
     * @return true if search completed successfully
     */
    virtual bool SearchModules(const FString& SearchQuery, const FString& StageFilter, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutModules) = 0;

    /**
     * Set an input value on a module
     * @param Params - Module input parameters
     * @param OutError - Error message if setting fails
     * @return true if input was set successfully
     */
    virtual bool SetModuleInput(const FNiagaraModuleInputParams& Params, FString& OutError) = 0;

    /**
     * Move a module to a new position within its stage
     * @param Params - Module move parameters
     * @param OutError - Error message if moving fails
     * @return true if module was moved successfully
     */
    virtual bool MoveModule(const FNiagaraModuleMoveParams& Params, FString& OutError) = 0;

    /**
     * Set a curve input on a module (for float curves like scale over life)
     * @param Params - Curve input parameters with keyframes
     * @param OutError - Error message if setting fails
     * @return true if curve input was set successfully
     */
    virtual bool SetModuleCurveInput(const FNiagaraModuleCurveInputParams& Params, FString& OutError) = 0;

    /**
     * Set a color curve input on a module (for color gradients over life)
     * @param Params - Color curve input parameters with keyframes
     * @param OutError - Error message if setting fails
     * @return true if color curve input was set successfully
     */
    virtual bool SetModuleColorCurveInput(const FNiagaraModuleColorCurveInputParams& Params, FString& OutError) = 0;

    /**
     * Set a random range input on a module (uniform random between min and max)
     * @param Params - Random input parameters with min/max values
     * @param OutError - Error message if setting fails
     * @return true if random input was set successfully
     */
    virtual bool SetModuleRandomInput(const FNiagaraModuleRandomInputParams& Params, FString& OutError) = 0;

    // ========================================================================
    // Parameters (Feature 3)
    // ========================================================================

    /**
     * Add a parameter to a Niagara System
     * @param Params - Parameter add parameters
     * @param OutError - Error message if adding fails
     * @return true if parameter was added successfully
     */
    virtual bool AddParameter(const FNiagaraParameterAddParams& Params, FString& OutError) = 0;

    /**
     * Set a parameter value on a Niagara System
     * @param SystemPath - Path to the system
     * @param ParameterName - Name of the parameter
     * @param Value - Value to set (as JSON)
     * @param OutError - Error message if setting fails
     * @return true if parameter was set successfully
     */
    virtual bool SetParameter(const FString& SystemPath, const FString& ParameterName, const TSharedPtr<FJsonValue>& Value, FString& OutError) = 0;

    // ========================================================================
    // Data Interfaces (Feature 4)
    // ========================================================================

    /**
     * Add a Data Interface to an emitter
     * @param Params - Data interface parameters
     * @param OutInterfaceId - Output identifier for the added interface
     * @param OutError - Error message if adding fails
     * @return true if interface was added successfully
     */
    virtual bool AddDataInterface(const FNiagaraDataInterfaceParams& Params, FString& OutInterfaceId, FString& OutError) = 0;

    /**
     * Set a property on a Data Interface
     * @param SystemPath - Path to the system
     * @param EmitterName - Name of the emitter
     * @param InterfaceName - Name of the data interface
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value to set (as JSON)
     * @param OutError - Error message if setting fails
     * @return true if property was set successfully
     */
    virtual bool SetDataInterfaceProperty(const FString& SystemPath, const FString& EmitterName, const FString& InterfaceName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;

    // ========================================================================
    // Renderers (Feature 5)
    // ========================================================================

    /**
     * Add a renderer to an emitter
     * @param Params - Renderer parameters
     * @param OutRendererId - Output identifier for the added renderer
     * @param OutError - Error message if adding fails
     * @return true if renderer was added successfully
     */
    virtual bool AddRenderer(const FNiagaraRendererParams& Params, FString& OutRendererId, FString& OutError) = 0;

    /**
     * Set a property on a renderer
     * @param SystemPath - Path to the system
     * @param EmitterName - Name of the emitter
     * @param RendererName - Name of the renderer
     * @param PropertyName - Name of the property
     * @param PropertyValue - Value to set (as JSON)
     * @param OutError - Error message if setting fails
     * @return true if property was set successfully
     */
    virtual bool SetRendererProperty(const FString& SystemPath, const FString& EmitterName, const FString& RendererName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;

    // ========================================================================
    // Level Integration (Feature 6)
    // ========================================================================

    /**
     * Spawn a Niagara System actor in the level
     * @param Params - Spawn parameters
     * @param OutActorName - Output name of the spawned actor
     * @param OutError - Error message if spawning fails
     * @return Spawned actor or nullptr if failed
     */
    virtual ANiagaraActor* SpawnActor(const FNiagaraActorSpawnParams& Params, FString& OutActorName, FString& OutError) = 0;

    // ========================================================================
    // Utility Methods
    // ========================================================================

    /**
     * Find a Niagara System by path
     * @param SystemPath - Path to the system
     * @return System or nullptr if not found
     */
    virtual UNiagaraSystem* FindSystem(const FString& SystemPath) = 0;

    /**
     * Find a Niagara Emitter by path
     * @param EmitterPath - Path to the emitter
     * @return Emitter or nullptr if not found
     */
    virtual UNiagaraEmitter* FindEmitter(const FString& EmitterPath) = 0;

    /**
     * Refresh any open Niagara editors for an asset
     * @param Asset - The asset to refresh editors for
     */
    virtual void RefreshEditors(UObject* Asset) = 0;
};
