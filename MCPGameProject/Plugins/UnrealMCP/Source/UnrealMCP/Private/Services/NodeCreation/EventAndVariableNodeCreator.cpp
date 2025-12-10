#include "Services/NodeCreation/EventAndVariableNodeCreator.h"
#include "Services/MacroDiscoveryService.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// Include refactored node creation helpers
#include "Services/NodeCreation/BlueprintActionDatabaseNodeCreator.h"
#include "Services/NodeCreation/NativePropertyNodeCreator.h"

FEventAndVariableNodeCreator& FEventAndVariableNodeCreator::Get()
{
	static FEventAndVariableNodeCreator Instance;
	return Instance;
}

bool FEventAndVariableNodeCreator::TryCreateComponentBoundEventNode(TSharedPtr<FJsonObject> ParamsObject, UBlueprint* Blueprint,
	const FString& BlueprintName, UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 334-445
	if (ParamsObject.IsValid() && ParamsObject->HasField(TEXT("component_name")) && ParamsObject->HasField(TEXT("event_name")))
	{
		// Component Bound Event - triggered by presence of component_name and event_name in kwargs
		FString ComponentName = ParamsObject->GetStringField(TEXT("component_name"));
		FString DelegateEventName = ParamsObject->GetStringField(TEXT("event_name"));

		UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Creating component bound event for component '%s', event '%s'"), *ComponentName, *DelegateEventName);

		// Find the component property in the Blueprint
		FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(Blueprint->GeneratedClass, FName(*ComponentName));
		if (!ComponentProperty)
		{
			OutErrorMessage = FString::Printf(TEXT("Component '%s' not found in Blueprint '%s'"), *ComponentName, *BlueprintName);
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: %s"), *OutErrorMessage);
			return true; // We handled it (even though it failed)
		}

		// Get the component class
		UClass* ComponentClass = ComponentProperty->PropertyClass;
		if (!ComponentClass)
		{
			OutErrorMessage = FString::Printf(TEXT("Could not get class for component '%s'"), *ComponentName);
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: %s"), *OutErrorMessage);
			return true; // We handled it (even though it failed)
		}

		// Find the delegate property on the component class
		FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(ComponentClass, FName(*DelegateEventName));
		if (!DelegateProperty)
		{
			OutErrorMessage = FString::Printf(TEXT("Event delegate '%s' not found on component class '%s'"), *DelegateEventName, *ComponentClass->GetName());
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: %s"), *OutErrorMessage);
			return true; // We handled it (even though it failed)
		}

		// Check if this event is already bound
		TArray<UK2Node_ComponentBoundEvent*> AllBoundEvents;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_ComponentBoundEvent>(Blueprint, AllBoundEvents);

		for (UK2Node_ComponentBoundEvent* ExistingNode : AllBoundEvents)
		{
			if (ExistingNode->GetComponentPropertyName() == FName(*ComponentName) &&
				ExistingNode->DelegatePropertyName == FName(*DelegateEventName))
			{
				OutErrorMessage = FString::Printf(TEXT("Event '%s' is already bound to component '%s'"), *DelegateEventName, *ComponentName);
				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: %s"), *OutErrorMessage);
				return true; // We handled it (even though it failed)
			}
		}

		// Create the UK2Node_ComponentBoundEvent
		UK2Node_ComponentBoundEvent* BoundEventNode = NewObject<UK2Node_ComponentBoundEvent>(EventGraph);
		if (!BoundEventNode)
		{
			OutErrorMessage = TEXT("Failed to create UK2Node_ComponentBoundEvent");
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: %s"), *OutErrorMessage);
			return true; // We handled it (even though it failed)
		}

		// Initialize the event node
		BoundEventNode->InitializeComponentBoundEventParams(ComponentProperty, DelegateProperty);
		BoundEventNode->NodePosX = PositionX;
		BoundEventNode->NodePosY = PositionY;

		// Add node to graph
		EventGraph->AddNode(BoundEventNode, true, false);
		BoundEventNode->CreateNewGuid();
		BoundEventNode->PostPlacedNewNode();
		BoundEventNode->AllocateDefaultPins();
		BoundEventNode->ReconstructNode();

		OutNode = BoundEventNode;
		OutNodeTitle = FString::Printf(TEXT("%s (%s)"), *DelegateEventName, *ComponentName);
		OutNodeType = TEXT("UK2Node_ComponentBoundEvent");

		UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created component bound event '%s' for component '%s'"), *DelegateEventName, *ComponentName);
		return true;
	}

	return false;
}

bool FEventAndVariableNodeCreator::TryCreateStandardEventNode(const FString& FunctionName, UEdGraph* EventGraph,
	int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 582-657
	// Handle standard event nodes (BeginPlay, Tick, etc.)
	if (FunctionName.StartsWith(TEXT("Receive")) ||
		 FunctionName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("EndPlay"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("BeginDestroy"), ESearchCase::IgnoreCase))
	{
		// Create standard event node
		UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);

		// Determine the correct event name and parent class
		FString EventName = FunctionName;
		FString ParentClassName = TEXT("/Script/Engine.Actor");

		// Map common event names to their proper "Receive" format
		if (FunctionName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveBeginPlay");
		}
		else if (FunctionName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveTick");
		}
		else if (FunctionName.Equals(TEXT("EndPlay"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveEndPlay");
		}
		else if (FunctionName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveActorBeginOverlap");
		}
		else if (FunctionName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveActorEndOverlap");
		}
		else if (FunctionName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveHit");
		}
		else if (FunctionName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveDestroyed");
		}
		else if (FunctionName.Equals(TEXT("BeginDestroy"), ESearchCase::IgnoreCase))
		{
			EventName = TEXT("ReceiveBeginDestroy");
		}

		// Set up the EventReference structure
		EventNode->EventReference.SetExternalMember(*EventName, UClass::TryFindTypeSlow<UClass>(*ParentClassName));
		if (!EventNode->EventReference.GetMemberParentClass())
		{
			// Fallback to Actor class if the specific class wasn't found
			EventNode->EventReference.SetExternalMember(*EventName, AActor::StaticClass());
		}

		// Override function - this makes it a Blueprint implementable event
		EventNode->bOverrideFunction = true;

		EventNode->NodePosX = PositionX;
		EventNode->NodePosY = PositionY;
		EventNode->CreateNewGuid();
		EventGraph->AddNode(EventNode, true, true);
		EventNode->PostPlacedNewNode();
		EventNode->AllocateDefaultPins();

		OutNode = EventNode;
		OutNodeTitle = EventName;
		OutNodeType = TEXT("UK2Node_Event");

		UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Created event node '%s'"), *EventName);
		return true;
	}

	return false;
}

bool FEventAndVariableNodeCreator::TryCreateMacroNode(const FString& FunctionName, UEdGraph* EventGraph,
	int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 658-703
	// Handle macro functions using the Macro Discovery Service
	if (FMacroDiscoveryService::IsMacroFunction(FunctionName))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Processing macro function '%s' using MacroDiscoveryService"), *FunctionName);

		// Use the macro discovery service to find the macro blueprint dynamically
		FString MacroGraphName = FMacroDiscoveryService::MapFunctionNameToMacroGraphName(FunctionName);
		UBlueprint* MacroBlueprint = FMacroDiscoveryService::FindMacroBlueprint(FunctionName);

		if (MacroBlueprint)
		{
			UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found macro blueprint for '%s' via discovery service"), *FunctionName);

			// Find the specific macro graph
			UEdGraph* TargetMacroGraph = FMacroDiscoveryService::FindMacroGraph(MacroBlueprint, MacroGraphName);

			if (TargetMacroGraph)
			{
				// Create macro instance
				UK2Node_MacroInstance* MacroInstance = NewObject<UK2Node_MacroInstance>(EventGraph);
				MacroInstance->SetMacroGraph(TargetMacroGraph);
				MacroInstance->NodePosX = PositionX;
				MacroInstance->NodePosY = PositionY;
				MacroInstance->CreateNewGuid();
				EventGraph->AddNode(MacroInstance, true, true);
				MacroInstance->PostPlacedNewNode();
				MacroInstance->AllocateDefaultPins();

				OutNode = MacroInstance;
				OutNodeTitle = FunctionName;
				OutNodeType = TEXT("UK2Node_MacroInstance");

				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created macro instance for '%s' using discovery service"), *FunctionName);
				return true;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName);
				OutErrorMessage = FString::Printf(TEXT("Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName);
				return true; // We handled it (even though it failed)
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not discover macro blueprint for '%s'"), *FunctionName);
			OutErrorMessage = FString::Printf(TEXT("Could not discover macro blueprint for '%s'. Macro may not be available."), *FunctionName);
			return true; // We handled it (even though it failed)
		}
	}

	return false;
}

bool FEventAndVariableNodeCreator::TryCreateVariableNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
	UBlueprint* Blueprint, const FString& BlueprintName, UEdGraph* EventGraph,
	int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 704-863
	// Variable getter/setter node creation
	if (FunctionName.StartsWith(TEXT("Get ")) || FunctionName.StartsWith(TEXT("Set ")) ||
		FunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase) ||
		FunctionName.Equals(TEXT("UK2Node_VariableSet"), ESearchCase::IgnoreCase))
	{
		FString VarName = FunctionName;
		bool bIsGetter = false;
		if (VarName.StartsWith(TEXT("Get ")))
		{
			VarName = VarName.RightChop(4);
			bIsGetter = true;
		}
		else if (VarName.StartsWith(TEXT("Set ")))
		{
			VarName = VarName.RightChop(4);
		}
		// Handle explicit node class names without "Get " or "Set " prefix
		else if (FunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase) ||
				 FunctionName.Equals(TEXT("UK2Node_VariableSet"), ESearchCase::IgnoreCase))
		{
			// Determine getter vs setter based on node class requested
			bIsGetter = FunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase);

			// Attempt to pull the actual variable name from JSON parameters ("variable_name") if provided
			if (ParamsObject.IsValid())
			{
				FString ParamVarName;
				// First check at root level
				if (ParamsObject->TryGetStringField(TEXT("variable_name"), ParamVarName) && !ParamVarName.IsEmpty())
				{
					VarName = ParamVarName;
				}
				else
				{
					// Then check nested under a "kwargs" object (for backward compatibility with Python tool)
					const TSharedPtr<FJsonObject>* KwargsObject;
					if (ParamsObject->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
					{
						if ((*KwargsObject)->TryGetStringField(TEXT("variable_name"), ParamVarName) && !ParamVarName.IsEmpty())
						{
							VarName = ParamVarName;
						}
					}
				}
			}
		}

		// Log getter/setter detection and variable name
		UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: EffectiveFunctionName='%s', bIsGetter=%d, bIsSetter=%d, VarName='%s'"), *FunctionName, (int)bIsGetter, (int)bIsGetter, *VarName);

		// Try to find the variable or component in the Blueprint
		bool bFound = false;

		// Diagnostic logging: List all user variables and what we're searching for
		UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Blueprint '%s' has %d user variables. Looking for '%s'."), *Blueprint->GetName(), Blueprint->NewVariables.Num(), *VarName);
		for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
		{
			UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found variable '%s' (type: %s)"), *VarDesc.VarName.ToString(), *VarDesc.VarType.PinCategory.ToString());
			if (VarDesc.VarName.ToString().Equals(VarName, ESearchCase::IgnoreCase))
			{
				if (bIsGetter)
				{
					UK2Node_VariableGet* GetterNode = NewObject<UK2Node_VariableGet>(EventGraph);
					GetterNode->VariableReference.SetSelfMember(*VarName);
					GetterNode->NodePosX = PositionX;
					GetterNode->NodePosY = PositionY;
					GetterNode->CreateNewGuid();
					EventGraph->AddNode(GetterNode, true, true);
					GetterNode->PostPlacedNewNode();
					GetterNode->AllocateDefaultPins();
					OutNode = GetterNode;
					OutNodeTitle = FString::Printf(TEXT("Get %s"), *VarName);
					OutNodeType = TEXT("UK2Node_VariableGet");
				}
				else
				{
					UK2Node_VariableSet* SetterNode = NewObject<UK2Node_VariableSet>(EventGraph);
					SetterNode->VariableReference.SetSelfMember(*VarName);
					SetterNode->NodePosX = PositionX;
					SetterNode->NodePosY = PositionY;
					SetterNode->CreateNewGuid();
					EventGraph->AddNode(SetterNode, true, true);
					SetterNode->PostPlacedNewNode();
					SetterNode->AllocateDefaultPins();
					OutNode = SetterNode;
					OutNodeTitle = FString::Printf(TEXT("Set %s"), *VarName);
					OutNodeType = TEXT("UK2Node_VariableSet");
				}
				bFound = true;
				break;
			}
		}

		// If not found in variables, check components
		if (!bFound && bIsGetter && Blueprint->SimpleConstructionScript)
		{
			TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
			for (USCS_Node* Node : AllNodes)
			{
				if (Node && Node->GetVariableName().ToString().Equals(VarName, ESearchCase::IgnoreCase))
				{
					// Create component reference node using variable get approach
					UK2Node_VariableGet* ComponentGetterNode = NewObject<UK2Node_VariableGet>(EventGraph);
					ComponentGetterNode->VariableReference.SetSelfMember(Node->GetVariableName());
					ComponentGetterNode->NodePosX = PositionX;
					ComponentGetterNode->NodePosY = PositionY;
					ComponentGetterNode->CreateNewGuid();
					EventGraph->AddNode(ComponentGetterNode, true, true);
					ComponentGetterNode->PostPlacedNewNode();
					ComponentGetterNode->AllocateDefaultPins();
					OutNode = ComponentGetterNode;
					OutNodeTitle = FString::Printf(TEXT("Get %s"), *VarName);
					OutNodeType = TEXT("UK2Node_VariableGet");
					bFound = true;

					UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Created component reference for '%s'"), *VarName);
					break;
				}
			}
		}

		if (!bFound)
		{
			// Variable not found directly in the Blueprint – it might be a native property on another class.
			// Attempt to spawn it via the Blueprint Action Database using multiple name variants so users can still
			// create property nodes like "Get Show Mouse Cursor" on a PlayerController reference.

			bool bSpawned = false;
			bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(FunctionName, TEXT(""), EventGraph, PositionX, PositionY, OutNode, OutNodeTitle, OutNodeType);

			if (!bSpawned)
			{
				// Try trimmed variable name (without Get/Set prefix)
				bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(VarName, TEXT(""), EventGraph, PositionX, PositionY, OutNode, OutNodeTitle, OutNodeType);
			}

			if (!bSpawned && bIsGetter)
			{
				// Prepend "Get " to the node name in case the BAD entry includes it
				FString GetterName = FString::Printf(TEXT("Get %s"), *VarName);
				bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(GetterName, TEXT(""), EventGraph, PositionX, PositionY, OutNode, OutNodeTitle, OutNodeType);
			}
			else if (!bSpawned && !bIsGetter)
			{
				FString SetterName = FString::Printf(TEXT("Set %s"), *VarName);
				bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(SetterName, TEXT(""), EventGraph, PositionX, PositionY, OutNode, OutNodeTitle, OutNodeType);
			}

			if (!bSpawned)
			{
				// Final attempt: directly construct native property node by class search
				bSpawned = FNativePropertyNodeCreator::TryCreateNativePropertyNode(VarName, bIsGetter, EventGraph, PositionX, PositionY, OutNode, OutNodeTitle, OutNodeType);
			}

			if (!bSpawned)
			{
				OutErrorMessage = FString::Printf(TEXT("Variable or component '%s' not found in Blueprint '%s' and no matching Blueprint Action Database entry"), *VarName, *BlueprintName);
				return true; // We handled it (even though it failed)
			}
		}

		return true;
	}

	return false;
}

bool FEventAndVariableNodeCreator::TryCreateStructNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
	UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
	UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage)
{
	// EXACT COPY from BlueprintNodeCreationService.cpp lines 864-980
	if (FunctionName.Equals(TEXT("BreakStruct"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("Break Struct"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("MakeStruct"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("Make Struct"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("UK2Node_BreakStruct"), ESearchCase::IgnoreCase) ||
		 FunctionName.Equals(TEXT("UK2Node_MakeStruct"), ESearchCase::IgnoreCase))
	{
		bool bIsBreakStruct = FunctionName.Contains(TEXT("Break"), ESearchCase::IgnoreCase);

		// Extract struct type from parameters
		FString StructTypeName;
		bool bFoundStructType = false;

		if (ParamsObject.IsValid())
		{
			// First check at root level
			if (ParamsObject->TryGetStringField(TEXT("struct_type"), StructTypeName) && !StructTypeName.IsEmpty())
			{
				bFoundStructType = true;
				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found struct_type '%s' at root level"), *StructTypeName);
			}
			else
			{
				// Then check nested under kwargs object
				const TSharedPtr<FJsonObject>* KwargsObject;
				if (ParamsObject->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
				{
					if ((*KwargsObject)->TryGetStringField(TEXT("struct_type"), StructTypeName) && !StructTypeName.IsEmpty())
					{
						bFoundStructType = true;
						UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found struct_type '%s' in kwargs"), *StructTypeName);
					}
				}
			}
		}

		if (!bFoundStructType)
		{
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: struct_type parameter is required for %s operations"), bIsBreakStruct ? TEXT("BreakStruct") : TEXT("MakeStruct"));
			OutErrorMessage = FString::Printf(TEXT("struct_type parameter is required for %s operations"), bIsBreakStruct ? TEXT("BreakStruct") : TEXT("MakeStruct"));
			return true; // We handled it (even though it failed)
		}

		// Find the struct type
		UScriptStruct* StructType = nullptr;

		// Try multiple variations of struct name resolution
		TArray<FString> StructNameVariations = {
			StructTypeName,
			FString::Printf(TEXT("F%s"), *StructTypeName),
			FString::Printf(TEXT("/Script/Engine.%s"), *StructTypeName),
			FString::Printf(TEXT("/Script/Engine.F%s"), *StructTypeName),
			FString::Printf(TEXT("/Script/CoreUObject.%s"), *StructTypeName),
			FString::Printf(TEXT("/Script/CoreUObject.F%s"), *StructTypeName)
		};

		for (const FString& StructName : StructNameVariations)
		{
			StructType = FindObject<UScriptStruct>(nullptr, *StructName);
			if (StructType)
			{
				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found struct type '%s' using name '%s'"), *StructType->GetName(), *StructName);
				break;
			}
		}

		// If not found by FindObject, try using LoadObject for asset paths
		if (!StructType && StructTypeName.StartsWith(TEXT("/Game/")))
		{
			StructType = LoadObject<UScriptStruct>(nullptr, *StructTypeName);
			if (StructType)
			{
				UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Loaded struct type '%s' using LoadObject"), *StructType->GetName());
			}
		}

		if (!StructType)
		{
			UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find struct type '%s'"), *StructTypeName);
			OutErrorMessage = FString::Printf(TEXT("Struct type not found: %s"), *StructTypeName);
			return true; // We handled it (even though it failed)
		}

		// Create the appropriate struct node
		if (bIsBreakStruct)
		{
			UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(EventGraph);
			BreakNode->StructType = StructType;
			BreakNode->NodePosX = PositionX;
			BreakNode->NodePosY = PositionY;
			BreakNode->CreateNewGuid();
			EventGraph->AddNode(BreakNode, true, true);
			BreakNode->PostPlacedNewNode();
			BreakNode->AllocateDefaultPins();

			OutNode = BreakNode;
			OutNodeTitle = FString::Printf(TEXT("Break %s"), *StructType->GetDisplayNameText().ToString());
			OutNodeType = TEXT("UK2Node_BreakStruct");

			UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created BreakStruct node for '%s'"), *StructType->GetName());
		}
		else
		{
			UK2Node_MakeStruct* MakeNode = NewObject<UK2Node_MakeStruct>(EventGraph);
			MakeNode->StructType = StructType;
			MakeNode->NodePosX = PositionX;
			MakeNode->NodePosY = PositionY;
			MakeNode->CreateNewGuid();
			EventGraph->AddNode(MakeNode, true, true);
			MakeNode->PostPlacedNewNode();
			MakeNode->AllocateDefaultPins();

			OutNode = MakeNode;
			OutNodeTitle = FString::Printf(TEXT("Make %s"), *StructType->GetDisplayNameText().ToString());
			OutNodeType = TEXT("UK2Node_MakeStruct");

			UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created MakeStruct node for '%s'"), *StructType->GetName());
		}

		return true;
	}

	return false;
}
