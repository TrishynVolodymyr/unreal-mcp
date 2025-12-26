// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;
class UK2Node_VariableGet;
class UK2Node_VariableSet;
class UBlueprint;

/**
 * Post-processor for variable getter/setter nodes.
 *
 * Handles fixing variable references after node creation:
 * - Blueprint Action Database creates variable getters/setters with SetExternalMember()
 *   which adds an unnecessary "Target" pin. For self variables, we need SetSelfMember() instead.
 * - When ClassName is specified for an external class variable (e.g., "BP_DialogueNPC"),
 *   we must ensure the node is set as external member of THAT class, not self.
 *   This prevents naming collision when both current Blueprint and target class have same variable name.
 */
class FVariableNodePostProcessor
{
public:
    /**
     * Apply post-creation fixes to a variable getter node.
     *
     * @param GetNode - The variable getter node to process
     * @param ClassName - Optional class name for external member reference
     * @param EventGraph - The graph containing the node
     */
    static void ProcessVariableGetNode(
        UK2Node_VariableGet* GetNode,
        const FString& ClassName,
        UEdGraph* EventGraph
    );

    /**
     * Apply post-creation fixes to a variable setter node.
     *
     * @param SetNode - The variable setter node to process
     * @param ClassName - Optional class name for external member reference
     * @param EventGraph - The graph containing the node
     */
    static void ProcessVariableSetNode(
        UK2Node_VariableSet* SetNode,
        const FString& ClassName,
        UEdGraph* EventGraph
    );

private:
    /**
     * Find a class by name, handling Blueprint-generated class naming conventions.
     *
     * @param ClassName - Class name to find (with or without _C suffix)
     * @return The found class, or nullptr if not found
     */
    static UClass* FindClassByName(const FString& ClassName);

    /**
     * Check if a variable is a self variable (owned by the Blueprint).
     *
     * @param Blueprint - Blueprint to check
     * @param VarName - Variable name to look for
     * @return True if the variable is a self variable
     */
    static bool IsSelfVariable(UBlueprint* Blueprint, FName VarName);

    /**
     * Set up an external member reference for a variable.
     *
     * @param VarName - Name of the variable
     * @param ClassName - Class name to set as external member
     * @param OutTargetClass - Output: the found target class
     * @param OutProperty - Output: the found property
     * @return True if setup was successful
     */
    static bool SetupExternalMember(
        FName VarName,
        const FString& ClassName,
        UClass*& OutTargetClass,
        FProperty*& OutProperty
    );
};
