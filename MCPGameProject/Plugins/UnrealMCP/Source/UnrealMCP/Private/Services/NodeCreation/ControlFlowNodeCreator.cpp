#include "Services/NodeCreation/ControlFlowNodeCreator.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetMathLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"

FControlFlowNodeCreator& FControlFlowNodeCreator::Get()
{
	static FControlFlowNodeCreator Instance;
	return Instance;
}

bool FControlFlowNodeCreator::TryCreateLiteralNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
	UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 208-274
	// Check for literal/constant value creation
	if (FunctionName.Equals(TEXT("Float"), ESearchCase::IgnoreCase) ||
		FunctionName.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) ||
		FunctionName.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase) ||
		FunctionName.StartsWith(TEXT("Literal"), ESearchCase::IgnoreCase))
	{
		// For literal values, we create a simple variable get node with a default value
		// This is the most straightforward way to create constants in Blueprint
		UK2Node_VariableGet* LiteralNode = NewObject<UK2Node_VariableGet>(EventGraph);

		// Set up a temporary variable reference for the literal
		FString LiteralVarName = TEXT("LiteralValue");
		if (ParamsObject.IsValid())
		{
			FString ParamValue;
			if (ParamsObject->TryGetStringField(TEXT("value"), ParamValue) && !ParamValue.IsEmpty())
			{
				LiteralVarName = FString::Printf(TEXT("Literal_%s"), *ParamValue);
			}
		}

		// For now, create a simple math operation that returns the constant
		// This is a workaround since direct literal nodes are complex in UE
		UK2Node_CallFunction* MathNode = NewObject<UK2Node_CallFunction>(EventGraph);

		// Use SelectFloat from KismetMathLibrary to create a constant
		UFunction* SelectFloatFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("SelectFloat"));
		if (SelectFloatFunc)
		{
			MathNode->SetFromFunction(SelectFloatFunc);
			MathNode->NodePosX = PositionX;
			MathNode->NodePosY = PositionY;
			MathNode->CreateNewGuid();
			EventGraph->AddNode(MathNode, true, true);
			MathNode->PostPlacedNewNode();
			MathNode->AllocateDefaultPins();

			// Set default values on pins to create the constant
			if (ParamsObject.IsValid())
			{
				FString ParamValue;
				if (ParamsObject->TryGetStringField(TEXT("value"), ParamValue) && !ParamValue.IsEmpty())
				{
					// Find the A and B pins and set them to the same value
					for (UEdGraphPin* Pin : MathNode->Pins)
					{
						if (Pin && (Pin->PinName == TEXT("A") || Pin->PinName == TEXT("B")))
						{
							Pin->DefaultValue = ParamValue;
						}
						// Set the Index pin to false (0) so it always returns A
						else if (Pin && Pin->PinName == TEXT("Index"))
						{
							Pin->DefaultValue = TEXT("false");
						}
					}
				}
			}

			OutNode = MathNode;
			OutNodeTitle = FString::Printf(TEXT("Constant %s"), *FunctionName);
			OutNodeType = TEXT("K2Node_CallFunction");
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool FControlFlowNodeCreator::TryCreateBranchNode(const FString& FunctionName, UEdGraph* EventGraph,
	int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 276-290
	// Check if this is a control flow node request
	if (FunctionName.Equals(TEXT("Branch"), ESearchCase::IgnoreCase) ||
		FunctionName.Equals(TEXT("IfThenElse"), ESearchCase::IgnoreCase) ||
		FunctionName.Equals(TEXT("UK2Node_IfThenElse"), ESearchCase::IgnoreCase))
	{
		UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
		BranchNode->NodePosX = PositionX;
		BranchNode->NodePosY = PositionY;
		BranchNode->CreateNewGuid();
		EventGraph->AddNode(BranchNode, true, true);
		BranchNode->PostPlacedNewNode();
		BranchNode->AllocateDefaultPins();
		OutNode = BranchNode;
		OutNodeTitle = TEXT("Branch");
		OutNodeType = TEXT("UK2Node_IfThenElse");
		return true;
	}

	return false;
}

bool FControlFlowNodeCreator::TryCreateSequenceNode(const FString& FunctionName, UEdGraph* EventGraph,
	int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 291-305
	if (FunctionName.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("ExecutionSequence"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("UK2Node_ExecutionSequence"), ESearchCase::IgnoreCase))
	{
		UK2Node_ExecutionSequence* SequenceNode = NewObject<UK2Node_ExecutionSequence>(EventGraph);
		SequenceNode->NodePosX = PositionX;
		SequenceNode->NodePosY = PositionY;
		SequenceNode->CreateNewGuid();
		EventGraph->AddNode(SequenceNode, true, true);
		SequenceNode->PostPlacedNewNode();
		SequenceNode->AllocateDefaultPins();
		OutNode = SequenceNode;
		OutNodeTitle = TEXT("Sequence");
		OutNodeType = TEXT("UK2Node_ExecutionSequence");
		return true;
	}

	return false;
}

bool FControlFlowNodeCreator::TryCreateCustomEventNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
	UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 306-333
	if (FunctionName.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("Custom Event"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("UK2Node_CustomEvent"), ESearchCase::IgnoreCase))
	{
		UK2Node_CustomEvent* CustomEventNode = NewObject<UK2Node_CustomEvent>(EventGraph);

		// Set custom event name from parameters if provided
		FString EventName = TEXT("CustomEvent"); // Default name
		if (ParamsObject.IsValid())
		{
			FString ParamEventName;
			if (ParamsObject->TryGetStringField(TEXT("event_name"), ParamEventName) && !ParamEventName.IsEmpty())
			{
				EventName = ParamEventName;
			}
		}

		CustomEventNode->CustomFunctionName = FName(*EventName);
		CustomEventNode->NodePosX = PositionX;
		CustomEventNode->NodePosY = PositionY;
		CustomEventNode->CreateNewGuid();
		EventGraph->AddNode(CustomEventNode, true, true);
		CustomEventNode->PostPlacedNewNode();
		CustomEventNode->AllocateDefaultPins();
		OutNode = CustomEventNode;
		OutNodeTitle = EventName;
		OutNodeType = TEXT("UK2Node_CustomEvent");
		return true;
	}

	return false;
}

bool FControlFlowNodeCreator::TryCreateCastNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
	UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 446-581
	if (FunctionName.Equals(TEXT("Cast"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("UK2Node_DynamicCast"), ESearchCase::IgnoreCase))
	{
		UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(EventGraph);

		// Set target type if provided in parameters
		if (ParamsObject.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: ParamsObject is valid for Cast node"));
			FString TargetTypeName;

			// Check if target_type is in kwargs sub-object
			const TSharedPtr<FJsonObject>* KwargsObject;
			if (ParamsObject->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found kwargs object"));
				if ((*KwargsObject)->TryGetStringField(TEXT("target_type"), TargetTypeName) && !TargetTypeName.IsEmpty())
				{
					UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found target_type in kwargs: '%s'"), *TargetTypeName);
				}
			}
			// Also check at root level for backwards compatibility
			else if (ParamsObject->TryGetStringField(TEXT("target_type"), TargetTypeName) && !TargetTypeName.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found target_type parameter: '%s'"), *TargetTypeName);
			}

			if (!TargetTypeName.IsEmpty())
			{
				// Find the class to cast to
				UClass* CastTargetClass = nullptr;

				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Looking for target type '%s'"), *TargetTypeName);

				// Common class mappings
				if (TargetTypeName.Equals(TEXT("PlayerController"), ESearchCase::IgnoreCase))
				{
					CastTargetClass = APlayerController::StaticClass();
					UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found PlayerController class"));
				}
				else if (TargetTypeName.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase))
				{
					CastTargetClass = APawn::StaticClass();
					UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found Pawn class"));
				}
				else if (TargetTypeName.Equals(TEXT("Actor"), ESearchCase::IgnoreCase))
				{
					CastTargetClass = AActor::StaticClass();
					UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found Actor class"));
				}
				else
				{
					// Try to find the class by name
					CastTargetClass = UClass::TryFindTypeSlow<UClass>(TargetTypeName);
					if (!CastTargetClass)
					{
						// Try with /Script/Engine. prefix
						FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *TargetTypeName);
						CastTargetClass = LoadClass<UObject>(nullptr, *EnginePath);
					}

					// If still not found, try to find it as a Blueprint class
					if (!CastTargetClass)
					{
						FAssetRegistryModule& AssetReg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
						TArray<FAssetData> BPAssets;
						AssetReg.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BPAssets);

						for (const FAssetData& AssetData : BPAssets)
						{
							FString AssetName = AssetData.AssetName.ToString();

							// Try exact match first (most reliable)
							bool bIsMatch = AssetName.Equals(TargetTypeName, ESearchCase::IgnoreCase);

							// If no exact match, try matching against the generated class name
							if (!bIsMatch)
							{
								if (UBlueprint* TestBP = Cast<UBlueprint>(AssetData.GetAsset()))
								{
									if (TestBP->GeneratedClass)
									{
										FString GeneratedClassName = TestBP->GeneratedClass->GetName();
										// Remove common Blueprint prefixes for comparison
										if (GeneratedClassName.StartsWith(TEXT("BP_")))
										{
											GeneratedClassName = GeneratedClassName.Mid(3);
										}
										bIsMatch = GeneratedClassName.Equals(TargetTypeName, ESearchCase::IgnoreCase);
									}
								}
							}

							if (bIsMatch)
							{
								if (UBlueprint* TargetBP = Cast<UBlueprint>(AssetData.GetAsset()))
								{
									CastTargetClass = TargetBP->GeneratedClass;
									if (CastTargetClass)
									{
										UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found Blueprint class '%s' (matched asset '%s')"), *CastTargetClass->GetName(), *AssetName);
										break;
									}
									else
									{
										UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Blueprint '%s' has null GeneratedClass"), *AssetName);
									}
								}
							}
						}
					}
				}

				if (CastTargetClass)
				{
					CastNode->TargetType = CastTargetClass;
					UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Set cast target type to '%s'"), *CastTargetClass->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find target type '%s'"), *TargetTypeName);
				}
			}
		}

		CastNode->NodePosX = PositionX;
		CastNode->NodePosY = PositionY;
		CastNode->CreateNewGuid();
		EventGraph->AddNode(CastNode, true, true);
		CastNode->PostPlacedNewNode();
		CastNode->AllocateDefaultPins();
		OutNode = CastNode;
		OutNodeTitle = TEXT("Cast");
		OutNodeType = TEXT("UK2Node_DynamicCast");
		return true;
	}

	return false;
}
