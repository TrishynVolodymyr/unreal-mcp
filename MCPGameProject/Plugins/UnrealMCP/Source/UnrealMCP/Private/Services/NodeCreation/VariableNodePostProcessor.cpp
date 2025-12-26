// Copyright Epic Games, Inc. All Rights Reserved.

#include "VariableNodePostProcessor.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "EdGraph/EdGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/UObjectIterator.h"

void FVariableNodePostProcessor::ProcessVariableGetNode(
    UK2Node_VariableGet* GetNode,
    const FString& ClassName,
    UEdGraph* EventGraph
)
{
    if (!GetNode || !EventGraph)
    {
        return;
    }

    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(EventGraph);
    if (!Blueprint)
    {
        return;
    }

    FName VarName = GetNode->GetVarName();

    // CASE 1: ClassName is specified - this should be an EXTERNAL member of that class
    // This handles the naming collision case where both WBP_DialogueWindow and BP_DialogueNPC
    // have a variable named "DialogueTable" - we want the one from the specified class
    if (!ClassName.IsEmpty())
    {
        UClass* TargetClass = nullptr;
        FProperty* Property = nullptr;

        if (SetupExternalMember(VarName, ClassName, TargetClass, Property))
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Setting variable getter '%s' as external member of class '%s'"),
                   *VarName.ToString(), *TargetClass->GetName());
            GetNode->VariableReference.SetExternalMember(VarName, TargetClass);
            GetNode->ReconstructNode(); // Rebuild pins with correct Target type
        }
        else if (TargetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Variable '%s' not found in class '%s', keeping original reference"),
                   *VarName.ToString(), *TargetClass->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Could not find class '%s' for external member '%s'"),
                   *ClassName, *VarName.ToString());
        }
    }
    // CASE 2: No ClassName specified - check if this is a self variable
    else
    {
        if (IsSelfVariable(Blueprint, VarName))
        {
            // This is a self variable! Fix the reference to use SetSelfMember
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Converting variable getter '%s' from external to self member"), *VarName.ToString());
            GetNode->VariableReference.SetSelfMember(VarName);
            GetNode->ReconstructNode(); // Rebuild pins to remove the "Target" pin
        }
    }
}

void FVariableNodePostProcessor::ProcessVariableSetNode(
    UK2Node_VariableSet* SetNode,
    const FString& ClassName,
    UEdGraph* EventGraph
)
{
    if (!SetNode || !EventGraph)
    {
        return;
    }

    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(EventGraph);
    if (!Blueprint)
    {
        return;
    }

    FName VarName = SetNode->GetVarName();

    // CASE 1: ClassName is specified - this should be an EXTERNAL member of that class
    if (!ClassName.IsEmpty())
    {
        UClass* TargetClass = nullptr;
        FProperty* Property = nullptr;

        if (SetupExternalMember(VarName, ClassName, TargetClass, Property))
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Setting variable setter '%s' as external member of class '%s'"),
                   *VarName.ToString(), *TargetClass->GetName());
            SetNode->VariableReference.SetExternalMember(VarName, TargetClass);
            SetNode->ReconstructNode();
        }
        else if (TargetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Variable '%s' not found in class '%s', keeping original reference"),
                   *VarName.ToString(), *TargetClass->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Could not find class '%s' for external member '%s'"),
                   *ClassName, *VarName.ToString());
        }
    }
    // CASE 2: No ClassName specified - check if this is a self variable
    else
    {
        if (IsSelfVariable(Blueprint, VarName))
        {
            UE_LOG(LogTemp, Warning, TEXT("POST-FIX: Converting variable setter '%s' from external to self member"), *VarName.ToString());
            SetNode->VariableReference.SetSelfMember(VarName);
            SetNode->ReconstructNode(); // Rebuild pins to remove the "Target" pin
        }
    }
}

UClass* FVariableNodePostProcessor::FindClassByName(const FString& ClassName)
{
    FString ClassNameWithSuffix = ClassName;
    if (!ClassNameWithSuffix.EndsWith(TEXT("_C")))
    {
        ClassNameWithSuffix += TEXT("_C");
    }

    // Search for Blueprint-generated class
    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* TestClass = *It;
        if (TestClass && (TestClass->GetName().Equals(ClassName, ESearchCase::IgnoreCase) ||
                          TestClass->GetName().Equals(ClassNameWithSuffix, ESearchCase::IgnoreCase)))
        {
            return TestClass;
        }
    }

    return nullptr;
}

bool FVariableNodePostProcessor::IsSelfVariable(UBlueprint* Blueprint, FName VarName)
{
    if (!Blueprint)
    {
        return false;
    }

    // Check Blueprint variables
    for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
    {
        if (VarDesc.VarName == VarName)
        {
            return true;
        }
    }

    // Also check component variables (SCS nodes)
    if (Blueprint->SimpleConstructionScript)
    {
        const TArray<USCS_Node*>& AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
        for (USCS_Node* Node : AllNodes)
        {
            if (Node && Node->GetVariableName() == VarName)
            {
                return true;
            }
        }
    }

    return false;
}

bool FVariableNodePostProcessor::SetupExternalMember(
    FName VarName,
    const FString& ClassName,
    UClass*& OutTargetClass,
    FProperty*& OutProperty
)
{
    OutTargetClass = FindClassByName(ClassName);
    OutProperty = nullptr;

    if (OutTargetClass)
    {
        // Verify the variable exists in the target class
        OutProperty = FindFProperty<FProperty>(OutTargetClass, VarName);
        return OutProperty != nullptr;
    }

    return false;
}
