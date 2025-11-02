// Copyright Epic Games, Inc. All Rights Reserved.

#include "NativePropertyNodeCreator.h"
#include "NodeCreationHelpers.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

bool FNativePropertyNodeCreator::TryCreateNativePropertyNode(
    const FString& VarName,
    bool bIsGetter,
    UEdGraph* EventGraph,
    int32 PositionX,
    int32 PositionY,
    UEdGraphNode*& OutNode,
    FString& OutTitle,
    FString& OutNodeType
)
{
    // Helper lambdas -------------------------------------------------------
    auto StripBoolPrefix = [](const FString& InName) -> FString
    {
        // Strip a leading 'b' only when it is followed by an uppercase character (UE bool convention)
        if (InName.StartsWith(TEXT("b")) && InName.Len() > 1 && FChar::IsUpper(InName[1]))
        {
            return InName.Mid(1);
        }
        return InName;
    };

    auto IsPropertyWritable = [bIsGetter](FProperty* Property) -> bool
    {
        const bool bConstParm = Property->HasAnyPropertyFlags(CPF_ConstParm);
        const bool bReadOnlyMeta = Property->HasMetaData(TEXT("BlueprintReadOnly"));
        // For setter requests we allow write when the property is NOT const and NOT marked BlueprintReadOnly.
        if (bIsGetter)
        {
            return true; // Getter can always be created for BlueprintVisible properties
        }

        return !bConstParm && !bReadOnlyMeta;
    };

    // ---------------------------------------------------------------------
    // Build search candidates (space-stripped variants with/without bool prefix)
    TArray<FString> Candidates;
    const FString NoSpace = VarName.Replace(TEXT(" "), TEXT(""));
    Candidates.Add(NoSpace);
    Candidates.Add(FString(TEXT("b")) + NoSpace); // bool prefix variant

    // Structure to hold all matching properties so we can choose deterministically
    struct FPropMatch
    {
        UClass*  Class;
        FProperty* Property;
    };
    TArray<FPropMatch> Matches;

    // Iterate over all loaded classes once and gather potential matches
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass* TargetClass = *ClassIt;
        if (!TargetClass || TargetClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }

        for (TFieldIterator<FProperty> PropIt(TargetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
            {
                continue; // Not visible to Blueprints – skip
            }

            // For setter requests ensure the property is writable
            if (!bIsGetter && !IsPropertyWritable(Property))
            {
                continue; // Not writable – skip for setters
            }

            const FString PropName    = Property->GetName();
            const FString StrippedProp = StripBoolPrefix(PropName);

            // Build list of potential property names for matching
            TArray<FString> NameOptions;
            NameOptions.Add(PropName);
            if (StrippedProp != PropName)
            {
                NameOptions.Add(StrippedProp);
            }

            FString DisplayNameMeta = Property->GetMetaData(TEXT("DisplayName"));
            if (DisplayNameMeta.IsEmpty())
            {
                DisplayNameMeta = NodeCreationHelpers::ConvertPropertyNameToDisplay(PropName);
            }
            NameOptions.Add(DisplayNameMeta.Replace(TEXT(" "), TEXT("")));

            // Perform case-insensitive comparison against all candidates
            bool bMatch = false;
            for (const FString& Candidate : Candidates)
            {
                for (const FString& Option : NameOptions)
                {
                    if (Option.Equals(Candidate, ESearchCase::IgnoreCase))
                    {
                        bMatch = true;
                        break;
                    }
                }
                if (bMatch) break;
            }

            if (bMatch)
            {
                Matches.Add({ TargetClass, Property });
            }
        }
    }

    if (Matches.Num() == 0)
    {
        return false; // Nothing matched
    }

    // ---------------------------------------------------------------------
    // Choose a deterministic match: sort by class name, then property name for stability
    Matches.Sort([](const FPropMatch& A, const FPropMatch& B)
    {
        const int32 ClassCmp = A.Class->GetName().Compare(B.Class->GetName());
        if (ClassCmp == 0)
        {
            return A.Property->GetName().Compare(B.Property->GetName()) < 0;
        }
        return ClassCmp < 0;
    });

    const FPropMatch& Chosen = Matches[0];
    UClass*   TargetClass = Chosen.Class;
    FProperty* Property   = Chosen.Property;
    const FString PropName = Property->GetName();

    // Create the appropriate node ------------------------------------------------
    if (bIsGetter)
    {
        UK2Node_VariableGet* GetterNode = NewObject<UK2Node_VariableGet>(EventGraph);
        GetterNode->VariableReference.SetExternalMember(*PropName, TargetClass);
        GetterNode->NodePosX = PositionX;
        GetterNode->NodePosY = PositionY;
        GetterNode->CreateNewGuid();
        EventGraph->AddNode(GetterNode, true, true);
        GetterNode->PostPlacedNewNode();
        GetterNode->AllocateDefaultPins();
        OutNode = GetterNode;
        OutTitle = FString::Printf(TEXT("Get %s"), *VarName);
        OutNodeType = TEXT("UK2Node_VariableGet");
    }
    else
    {
        UK2Node_VariableSet* SetterNode = NewObject<UK2Node_VariableSet>(EventGraph);
        SetterNode->VariableReference.SetExternalMember(*PropName, TargetClass);
        SetterNode->NodePosX = PositionX;
        SetterNode->NodePosY = PositionY;
        SetterNode->CreateNewGuid();
        EventGraph->AddNode(SetterNode, true, true);
        SetterNode->PostPlacedNewNode();
        SetterNode->AllocateDefaultPins();
        OutNode = SetterNode;
        OutTitle = FString::Printf(TEXT("Set %s"), *VarName);
        OutNodeType = TEXT("UK2Node_VariableSet");
    }

    return true;
}
