#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Dom/JsonObject.h"

/**
 * Parameters for Animation Blueprint creation
 */
struct UNREALMCP_API FAnimBlueprintCreationParams
{
    /** Name of the Animation Blueprint to create */
    FString Name;

    /** Folder path where the Animation Blueprint should be created */
    FString FolderPath;

    /** Parent AnimInstance class name (C++ or Blueprint) */
    FString ParentClassName;

    /** Path to the target skeleton asset */
    FString SkeletonPath;

    /** Whether to compile after creation */
    bool bCompileOnCreation = true;

    /** Default constructor */
    FAnimBlueprintCreationParams()
    {
    }

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for linking an animation layer
 */
struct UNREALMCP_API FAnimLayerLinkParams
{
    /** Name of the layer interface to link */
    FString LayerInterfaceName;

    /** Name or path of the animation layer class implementing the interface */
    FString LayerClassName;

    /** Default constructor */
    FAnimLayerLinkParams()
    {
    }
};

/**
 * Parameters for creating an animation state
 */
struct UNREALMCP_API FAnimStateParams
{
    /** Name of the state */
    FString StateName;

    /** Path to the animation asset (sequence, blend space, etc.) */
    FString AnimationAssetPath;

    /** Whether this is the default state */
    bool bIsDefaultState = false;

    /** Node position in graph */
    FVector2D NodePosition = FVector2D::ZeroVector;

    /** Default constructor */
    FAnimStateParams()
    {
    }
};

/**
 * Parameters for creating a state transition
 */
struct UNREALMCP_API FAnimTransitionParams
{
    /** Name of the source state */
    FString FromStateName;

    /** Name of the destination state */
    FString ToStateName;

    /** Transition rule type: "TimeRemaining", "BoolVariable", "CrossfadeBlend", "Custom" */
    FString TransitionRuleType;

    /** Duration for blend transitions */
    float BlendDuration = 0.2f;

    /** Variable name for bool-based transitions */
    FString ConditionVariableName;

    /** Default constructor */
    FAnimTransitionParams()
    {
    }
};

/**
 * Interface for Animation Blueprint service operations
 */
class UNREALMCP_API IAnimationBlueprintService
{
public:
    virtual ~IAnimationBlueprintService() = default;

    // ============================================================================
    // Animation Blueprint Creation
    // ============================================================================

    /**
     * Create a new Animation Blueprint
     * @param Params - Animation Blueprint creation parameters
     * @return Created Animation Blueprint or nullptr if failed
     */
    virtual UAnimBlueprint* CreateAnimBlueprint(const FAnimBlueprintCreationParams& Params) = 0;

    /**
     * Find an Animation Blueprint by name
     * @param AnimBlueprintName - Name of the Animation Blueprint to find
     * @return Found Animation Blueprint or nullptr
     */
    virtual UAnimBlueprint* FindAnimBlueprint(const FString& AnimBlueprintName) = 0;

    /**
     * Compile an Animation Blueprint
     * @param AnimBlueprint - Animation Blueprint to compile
     * @param OutError - Error message if compilation fails
     * @return true if compilation succeeded
     */
    virtual bool CompileAnimBlueprint(UAnimBlueprint* AnimBlueprint, FString& OutError) = 0;

    // ============================================================================
    // Animation Layers
    // ============================================================================

    /**
     * Link an animation layer to the Animation Blueprint
     * @param AnimBlueprint - Target Animation Blueprint
     * @param Params - Layer linking parameters
     * @param OutError - Error message if linking fails
     * @return true if layer was linked successfully
     */
    virtual bool LinkAnimationLayer(UAnimBlueprint* AnimBlueprint, const FAnimLayerLinkParams& Params, FString& OutError) = 0;

    /**
     * Get list of linked animation layers
     * @param AnimBlueprint - Target Animation Blueprint
     * @param OutLayers - Array of linked layer names
     * @return true if layers were retrieved successfully
     */
    virtual bool GetLinkedAnimationLayers(UAnimBlueprint* AnimBlueprint, TArray<FString>& OutLayers) = 0;

    // ============================================================================
    // State Machines
    // ============================================================================

    /**
     * Create a state machine in the AnimGraph
     * @param AnimBlueprint - Target Animation Blueprint
     * @param StateMachineName - Name for the state machine
     * @param OutError - Error message if creation fails
     * @return true if state machine was created successfully
     */
    virtual bool CreateStateMachine(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, FString& OutError) = 0;

    /**
     * Add a state to a state machine
     * @param AnimBlueprint - Target Animation Blueprint
     * @param StateMachineName - Name of the state machine
     * @param Params - State creation parameters
     * @param OutError - Error message if creation fails
     * @return true if state was added successfully
     */
    virtual bool AddStateToStateMachine(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, const FAnimStateParams& Params, FString& OutError) = 0;

    /**
     * Add a transition between states
     * @param AnimBlueprint - Target Animation Blueprint
     * @param StateMachineName - Name of the state machine
     * @param Params - Transition creation parameters
     * @param OutError - Error message if creation fails
     * @return true if transition was added successfully
     */
    virtual bool AddStateTransition(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, const FAnimTransitionParams& Params, FString& OutError) = 0;

    /**
     * Get list of states in a state machine
     * @param AnimBlueprint - Target Animation Blueprint
     * @param StateMachineName - Name of the state machine
     * @param OutStates - Array of state names
     * @return true if states were retrieved successfully
     */
    virtual bool GetStateMachineStates(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, TArray<FString>& OutStates) = 0;

    // ============================================================================
    // Animation Variables
    // ============================================================================

    /**
     * Add a variable to the Animation Blueprint
     * @param AnimBlueprint - Target Animation Blueprint
     * @param VariableName - Name of the variable
     * @param VariableType - Type of the variable (Bool, Float, Int, etc.)
     * @param DefaultValue - Optional default value as string
     * @param OutError - Error message if addition fails
     * @return true if variable was added successfully
     */
    virtual bool AddAnimVariable(UAnimBlueprint* AnimBlueprint, const FString& VariableName, const FString& VariableType, const FString& DefaultValue, FString& OutError) = 0;

    /**
     * Get list of animation variables
     * @param AnimBlueprint - Target Animation Blueprint
     * @param OutVariables - Array of variable name/type pairs
     * @return true if variables were retrieved successfully
     */
    virtual bool GetAnimVariables(UAnimBlueprint* AnimBlueprint, TArray<TPair<FString, FString>>& OutVariables) = 0;

    // ============================================================================
    // Animation Slots
    // ============================================================================

    /**
     * Configure an animation slot
     * @param AnimBlueprint - Target Animation Blueprint
     * @param SlotName - Name of the slot
     * @param SlotGroupName - Name of the slot group
     * @param OutError - Error message if configuration fails
     * @return true if slot was configured successfully
     */
    virtual bool ConfigureAnimSlot(UAnimBlueprint* AnimBlueprint, const FString& SlotName, const FString& SlotGroupName, FString& OutError) = 0;

    // ============================================================================
    // Metadata
    // ============================================================================

    /**
     * Get Animation Blueprint metadata
     * @param AnimBlueprint - Target Animation Blueprint
     * @param OutMetadata - JSON object containing metadata
     * @return true if metadata was retrieved successfully
     */
    virtual bool GetAnimBlueprintMetadata(UAnimBlueprint* AnimBlueprint, TSharedPtr<FJsonObject>& OutMetadata) = 0;

    // ============================================================================
    // AnimGraph Node Connections
    // ============================================================================

    /**
     * Connect nodes in the AnimGraph (e.g., state machine to output pose)
     * @param AnimBlueprint - Target Animation Blueprint
     * @param SourceNodeName - Name of the source node (e.g., state machine name)
     * @param TargetNodeName - Name of the target node (empty string means OutputPose/Root)
     * @param SourcePinName - Name of the source pin (default: "Pose")
     * @param TargetPinName - Name of the target pin (default: "Result")
     * @param OutError - Error message if connection fails
     * @return true if connection was successful
     */
    virtual bool ConnectAnimGraphNodes(UAnimBlueprint* AnimBlueprint, const FString& SourceNodeName, const FString& TargetNodeName, const FString& SourcePinName, const FString& TargetPinName, FString& OutError) = 0;
};
