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

    /** Stage to add the module to: "Spawn", "Update", "Event", "EmitterSpawn", or "EmitterUpdate" */
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
        if (Stage != TEXT("Spawn") && Stage != TEXT("Update") && Stage != TEXT("Event") &&
            Stage != TEXT("EmitterSpawn") && Stage != TEXT("EmitterUpdate"))
        {
            OutError = FString::Printf(TEXT("Invalid stage '%s'. Valid stages: 'Spawn', 'Update', 'Event', 'EmitterSpawn', 'EmitterUpdate'"), *Stage);
            return false;
        }
        return true;
    }
};

/**
 * Parameters for removing a module from an emitter
 */
struct UNREALMCP_API FNiagaraModuleRemoveParams
{
    /** Path to the system containing the emitter */
    FString SystemPath;

    /** Name of the emitter containing the module */
    FString EmitterName;

    /** Name of the module to remove */
    FString ModuleName;

    /** Stage the module is in: "Spawn", "Update", or "Event" */
    FString Stage;

    /** Default constructor */
    FNiagaraModuleRemoveParams() = default;

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
        if (Stage != TEXT("Spawn") && Stage != TEXT("Update") && Stage != TEXT("Event") &&
            Stage != TEXT("EmitterSpawn") && Stage != TEXT("EmitterUpdate"))
        {
            OutError = FString::Printf(TEXT("Invalid stage '%s'. Valid stages: 'Spawn', 'Update', 'Event', 'EmitterSpawn', 'EmitterUpdate'"), *Stage);
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
        if (Stage != TEXT("Spawn") && Stage != TEXT("Update") && Stage != TEXT("Event") &&
            Stage != TEXT("EmitterSpawn") && Stage != TEXT("EmitterUpdate"))
        {
            OutError = FString::Printf(TEXT("Invalid stage '%s'. Valid stages: 'Spawn', 'Update', 'Event', 'EmitterSpawn', 'EmitterUpdate'"), *Stage);
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

    /** Optional: Set the module's enabled state (if set, takes precedence - can be combined with input setting) */
    TOptional<bool> Enabled;

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
        // InputName is only required if we're not just setting enabled state
        if (InputName.IsEmpty() && !Enabled.IsSet())
        {
            OutError = TEXT("Either input_name or enabled must be provided");
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
 * Parameters for setting a linked input on a module (binding to a particle attribute)
 */
struct UNREALMCP_API FNiagaraModuleLinkedInputParams
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

    /** Value to link to (e.g., "Particles.NormalizedAge", "Particles.Velocity") */
    FString LinkedValue;

    /** Default constructor */
    FNiagaraModuleLinkedInputParams() = default;

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
        if (InputName.IsEmpty())
        {
            OutError = TEXT("Input name cannot be empty");
            return false;
        }
        if (LinkedValue.IsEmpty())
        {
            OutError = TEXT("Linked value cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for setting an emitter property
 */
struct UNREALMCP_API FNiagaraEmitterPropertyParams
{
    /** Path to the system containing the emitter */
    FString SystemPath;

    /** Name of the emitter to modify */
    FString EmitterName;

    /** Name of the property to set */
    FString PropertyName;

    /** Value to set (as string, will be parsed based on property type) */
    FString PropertyValue;

    /** Default constructor */
    FNiagaraEmitterPropertyParams() = default;

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
        if (PropertyName.IsEmpty())
        {
            OutError = TEXT("Property name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for setting a static switch on a module
 */
struct UNREALMCP_API FNiagaraModuleStaticSwitchParams
{
    /** Path to the system containing the emitter */
    FString SystemPath;

    /** Name of the emitter containing the module */
    FString EmitterName;

    /** Name of the module */
    FString ModuleName;

    /** Stage the module is in */
    FString Stage;

    /** Name of the static switch (e.g., "Scale Color Mode") */
    FString SwitchName;

    /** Value to set - can be display name, internal name, or index */
    FString Value;

    /** Default constructor */
    FNiagaraModuleStaticSwitchParams() = default;

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
        if (SwitchName.IsEmpty())
        {
            OutError = TEXT("Switch name cannot be empty");
            return false;
        }
        if (Value.IsEmpty())
        {
            OutError = TEXT("Value cannot be empty");
            return false;
        }
        return true;
    }
};
