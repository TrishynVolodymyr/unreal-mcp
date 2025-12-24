// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArithmeticNodeCreator.h"
#include "K2Node_PromotableOperator.h"
#include "K2Node_CallFunction.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlueprintNodeBinder.h"
#include "BlueprintTypePromotion.h"

bool FArithmeticNodeCreator::TryCreateArithmeticOrComparisonNode(
    const FString& OperationName,
    UEdGraph* EventGraph,
    int32 PositionX,
    int32 PositionY,
    UEdGraphNode*& OutNode,
    FString& OutTitle,
    FString& OutNodeType
)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Attempting to create '%s' using TypePromotion system"), *OperationName);
    
    // Map common operation names to TypePromotion operator names
    TMap<FString, FName> OperationMappings;
    
    // Arithmetic operations
    OperationMappings.Add(TEXT("Add"), TEXT("Add"));
    OperationMappings.Add(TEXT("Subtract"), TEXT("Subtract"));
    OperationMappings.Add(TEXT("Multiply"), TEXT("Multiply"));
    OperationMappings.Add(TEXT("Divide"), TEXT("Divide"));
    OperationMappings.Add(TEXT("Modulo"), TEXT("Percent"));
    OperationMappings.Add(TEXT("Power"), TEXT("MultiplyMultiply"));
    
    // Comparison operations
    OperationMappings.Add(TEXT("Equal"), TEXT("EqualEqual"));
    OperationMappings.Add(TEXT("NotEqual"), TEXT("NotEqual"));
    OperationMappings.Add(TEXT("Greater"), TEXT("Greater"));
    OperationMappings.Add(TEXT("GreaterEqual"), TEXT("GreaterEqual"));
    OperationMappings.Add(TEXT("Less"), TEXT("Less"));
    OperationMappings.Add(TEXT("LessEqual"), TEXT("LessEqual"));
    
    // Logical operations
    OperationMappings.Add(TEXT("And"), TEXT("BooleanAND"));
    OperationMappings.Add(TEXT("Or"), TEXT("BooleanOR"));
    OperationMappings.Add(TEXT("Not"), TEXT("BooleanNOT"));
    
    // Symbol-based operations
    OperationMappings.Add(TEXT("+"), TEXT("Add"));
    OperationMappings.Add(TEXT("-"), TEXT("Subtract"));
    OperationMappings.Add(TEXT("*"), TEXT("Multiply"));
    OperationMappings.Add(TEXT("/"), TEXT("Divide"));
    OperationMappings.Add(TEXT("=="), TEXT("EqualEqual"));
    OperationMappings.Add(TEXT("!="), TEXT("NotEqual"));
    OperationMappings.Add(TEXT(">"), TEXT("Greater"));
    OperationMappings.Add(TEXT(">="), TEXT("GreaterEqual"));
    OperationMappings.Add(TEXT("<"), TEXT("Less"));
    OperationMappings.Add(TEXT("<="), TEXT("LessEqual"));
    
    // Check if we have a mapping for this operation
    const FName* OpName = OperationMappings.Find(OperationName);
    if (!OpName)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: No TypePromotion mapping found for '%s'"), *OperationName);
        return false;
    }
    
    // Check if this is a promotable operator (arithmetic/comparison) vs legacy boolean operation
    static const TSet<FName> PromotableOpNames = {
        TEXT("Add"), TEXT("Subtract"), TEXT("Multiply"), TEXT("Divide"),
        TEXT("Greater"), TEXT("GreaterEqual"), TEXT("Less"), TEXT("LessEqual"),
        TEXT("NotEqual"), TEXT("EqualEqual")
    };

    bool bIsPromotableOp = PromotableOpNames.Contains(*OpName);

    // First try the TypePromotion spawner (may not be registered if editor context menu hasn't been built yet)
    UBlueprintFunctionNodeSpawner* OperatorSpawner = FTypePromotion::GetOperatorSpawner(*OpName);
    if (OperatorSpawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Found TypePromotion spawner for operation '%s' -> '%s'"), *OperationName, *OpName->ToString());

        // Create the node using the TypePromotion spawner
        OutNode = OperatorSpawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D(PositionX, PositionY));
        if (OutNode)
        {
            FText UserFacingName = FTypePromotion::GetUserFacingOperatorName(*OpName);
            OutTitle = UserFacingName.IsEmpty() ? OperationName : UserFacingName.ToString();
            OutNodeType = OutNode->GetClass()->GetName();

            // Cast to PromotableOperator to call ResetNodeToWildcard for proper initialization
            if (UK2Node_PromotableOperator* PromotableOp = Cast<UK2Node_PromotableOperator>(OutNode))
            {
                PromotableOp->ResetNodeToWildcard();
                PromotableOp->ReconstructNode();

                if (const UEdGraphSchema* Schema = EventGraph->GetSchema())
                {
                    Schema->ForceVisualizationCacheClear();
                }
                EventGraph->NotifyGraphChanged();

                UE_LOG(LogTemp, Warning, TEXT("Applied PromotableOperator wildcard pin fix for node: %s"), *OutTitle);
            }

            UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Successfully created '%s' operator node using TypePromotion spawner"), *OutTitle);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TryCreateArithmeticOrComparisonNode: TypePromotion spawner failed to create node for '%s'"), *OpName->ToString());
        }
    }

    // Fallback: Manually create UK2Node_PromotableOperator for promotable operations
    // This is needed when the TypePromotion spawner isn't registered (e.g., editor context menu hasn't been built)
    if (bIsPromotableOp)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Creating PromotableOperator manually for '%s'"), *OpName->ToString());

        // Create the PromotableOperator node directly
        UK2Node_PromotableOperator* PromotableOp = NewObject<UK2Node_PromotableOperator>(EventGraph);
        PromotableOp->NodePosX = PositionX;
        PromotableOp->NodePosY = PositionY;
        PromotableOp->CreateNewGuid();
        EventGraph->AddNode(PromotableOp, true, true);

        // Find a PRIMITIVE numeric function for this operation to initialize the node
        // The key is calling ResetNodeToWildcard() AFTER to make it a proper wildcard
        // IMPORTANT: GetAllFuncsForOp returns ALL functions including specialized types like
        // FrameNumber, Timespan, etc. We need to prioritize primitive numeric types (int, float, double)
        // so the node title shows correctly as "Divide" instead of "FrameNumber / FrameNumber"
        TArray<UFunction*> OpFunctions;
        FTypePromotion::GetAllFuncsForOp(*OpName, OpFunctions);

        if (OpFunctions.Num() > 0)
        {
            // Find a primitive numeric function to initialize with (int, float, or double)
            // This ensures the node displays correctly as a generic operator
            UFunction* PrimitiveFunc = nullptr;
            for (UFunction* Func : OpFunctions)
            {
                FString FuncName = Func->GetName();
                // Prefer functions operating on primitive numeric types
                // Examples: Add_IntInt, Divide_IntInt, Multiply_FloatFloat, Add_DoubleDouble
                if (FuncName.Contains(TEXT("_Int")) ||
                    FuncName.Contains(TEXT("_Float")) ||
                    FuncName.Contains(TEXT("_Double")))
                {
                    PrimitiveFunc = Func;
                    UE_LOG(LogTemp, Display, TEXT("TryCreateArithmeticOrComparisonNode: Selected primitive function '%s' for operation '%s'"), *FuncName, *OpName->ToString());
                    break;
                }
            }

            // Fall back to first function if no primitive found (shouldn't happen for basic math)
            UFunction* InitFunc = PrimitiveFunc ? PrimitiveFunc : OpFunctions[0];
            if (!PrimitiveFunc)
            {
                UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: No primitive function found for '%s', using '%s'"), *OpName->ToString(), *OpFunctions[0]->GetName());
            }

            // Use SetFromFunction to initialize the node structure (sets OperationName internally)
            PromotableOp->SetFromFunction(InitFunc);
            PromotableOp->PostPlacedNewNode();
            PromotableOp->AllocateDefaultPins();

            // CRITICAL: Reset to wildcard after initialization to remove any pre-typed pins
            PromotableOp->ResetNodeToWildcard();

            if (const UEdGraphSchema* Schema = EventGraph->GetSchema())
            {
                Schema->ForceVisualizationCacheClear();
            }
            EventGraph->NotifyGraphChanged();

            OutNode = PromotableOp;
            FText UserFacingName = FTypePromotion::GetUserFacingOperatorName(*OpName);
            OutTitle = UserFacingName.IsEmpty() ? OperationName : UserFacingName.ToString();
            OutNodeType = TEXT("UK2Node_PromotableOperator");

            UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Successfully created wildcard PromotableOperator '%s' manually"), *OutTitle);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TryCreateArithmeticOrComparisonNode: No functions found for operation '%s'"), *OpName->ToString());
            // Clean up the node we created
            EventGraph->RemoveNode(PromotableOp);
        }
    }

    // Fallback to legacy function nodes for boolean operations (And, Or, Not)
    TMap<FString, TArray<FString>> LegacyMappings;
    LegacyMappings.Add(TEXT("And"), {TEXT("BooleanAND")});
    LegacyMappings.Add(TEXT("Or"), {TEXT("BooleanOR")});
    LegacyMappings.Add(TEXT("Not"), {TEXT("BooleanNOT")});

    const TArray<FString>* FunctionNames = LegacyMappings.Find(OperationName);
    if (FunctionNames)
    {
        for (const FString& FunctionName : *FunctionNames)
        {
            UFunction* TargetFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(*FunctionName);
            if (TargetFunction)
            {
                UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
                FunctionNode->FunctionReference.SetExternalMember(TargetFunction->GetFName(), UKismetMathLibrary::StaticClass());
                FunctionNode->NodePosX = PositionX;
                FunctionNode->NodePosY = PositionY;
                FunctionNode->CreateNewGuid();
                EventGraph->AddNode(FunctionNode, true, true);
                FunctionNode->PostPlacedNewNode();
                FunctionNode->AllocateDefaultPins();

                OutNode = FunctionNode;
                OutTitle = OperationName;
                OutNodeType = TEXT("UK2Node_CallFunction");

                UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Created legacy function node '%s'"), *FunctionName);
                return true;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Failed to create node for operation '%s'"), *OperationName);
    return false;
}
