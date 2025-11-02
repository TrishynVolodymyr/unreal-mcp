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
    
    // APPROACH 1: Try to create K2Node_PromotableOperator directly
    UE_LOG(LogTemp, Warning, TEXT("Attempting DIRECT K2Node_PromotableOperator creation for operator '%s'"), *OpName->ToString());
    
    // Check if this operator is supported by TypePromotion system
    const TSet<FName>& AllOperators = FTypePromotion::GetAllOpNames();
    if (AllOperators.Contains(*OpName))
    {
        UE_LOG(LogTemp, Warning, TEXT("Operator '%s' is registered in TypePromotion system"), *OpName->ToString());
        
        // Create K2Node_PromotableOperator directly and set its function
        UK2Node_PromotableOperator* PromotableOpNode = NewObject<UK2Node_PromotableOperator>(EventGraph);
        if (PromotableOpNode)
        {
            // Find the appropriate function for this operator
            TArray<UFunction*> OpFunctions;
            FTypePromotion::GetAllFuncsForOp(*OpName, OpFunctions);
            
            if (OpFunctions.Num() > 0)
            {
                // Use the first available function (wildcard operators will adapt)
                UFunction* BaseFunction = OpFunctions[0];
                PromotableOpNode->SetFromFunction(BaseFunction);
                
                // Set position and add to graph
                PromotableOpNode->NodePosX = PositionX;
                PromotableOpNode->NodePosY = PositionY;
                PromotableOpNode->CreateNewGuid();
                EventGraph->AddNode(PromotableOpNode, true, true);
                PromotableOpNode->PostPlacedNewNode();
                PromotableOpNode->AllocateDefaultPins();
                
                // CRITICAL FIX: Don't break natural PromotableOperator behavior!
                // SetFromFunction already sets up proper wildcard adaptation
                // Just finalize with reconstruction and visualization
                PromotableOpNode->ReconstructNode();
                
                if (const UEdGraphSchema* Schema = EventGraph->GetSchema())
                {
                    Schema->ForceVisualizationCacheClear();
                }
                EventGraph->NotifyGraphChanged();
                
                OutNode = PromotableOpNode;
                FText UserFacingName = FTypePromotion::GetUserFacingOperatorName(*OpName);
                OutTitle = UserFacingName.IsEmpty() ? OperationName : UserFacingName.ToString();
                OutNodeType = TEXT("K2Node_PromotableOperator");
                
                UE_LOG(LogTemp, Warning, TEXT("Successfully created DIRECT K2Node_PromotableOperator for '%s'"), *OutTitle);
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No functions found for operator '%s'"), *OpName->ToString());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Operator '%s' not found in TypePromotion system"), *OpName->ToString());
    }
    
    // APPROACH 2: Try to get the operator spawner from TypePromotion system (original approach)
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
            
            // CRITICAL FIX: Properly initialize PromotableOperator wildcard pins
            if (OutNodeType.Contains(TEXT("PromotableOperator")))
            {
                // Cast to PromotableOperator to access ResetNodeToWildcard
                if (UK2Node_PromotableOperator* PromotableOp = Cast<UK2Node_PromotableOperator>(OutNode))
                {
                    // Don't interfere with natural PromotableOperator adaptation
                    // TypePromotion spawner already sets up proper behavior
                    PromotableOp->ReconstructNode();
                    
                    if (const UEdGraphSchema* Schema = EventGraph->GetSchema())
                    {
                        Schema->ForceVisualizationCacheClear();
                    }
                    EventGraph->NotifyGraphChanged();
                    
                    UE_LOG(LogTemp, Warning, TEXT("Applied PromotableOperator wildcard pin fix with ResetNodeToWildcard for node: %s"), *OutTitle);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to cast PromotableOperator node for ResetNodeToWildcard"));
                }
            }
            
            UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Successfully created dynamic '%s' operator node using TypePromotion"), *OutTitle);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TryCreateArithmeticOrComparisonNode: TypePromotion spawner failed to create node for '%s'"), *OpName->ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: No TypePromotion spawner found for operator '%s'"), *OpName->ToString());
        
        // Fallback to old system for non-promotable operations
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
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Failed to create node for operation '%s'"), *OperationName);
    return false;
}
