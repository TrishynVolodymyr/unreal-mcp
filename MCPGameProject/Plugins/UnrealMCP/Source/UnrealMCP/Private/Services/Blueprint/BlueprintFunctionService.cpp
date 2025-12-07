#include "Services/Blueprint/BlueprintFunctionService.h"
#include "Services/Blueprint/BlueprintCacheService.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool FBlueprintFunctionService::CreateCustomBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TSharedPtr<FJsonObject>& FunctionParams, FBlueprintCache& Cache, TFunction<bool(const FString&, FEdGraphPinType&)> ConvertStringToPinTypeFunc)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Creating function '%s' in blueprint '%s'"),
        *FunctionName, *Blueprint->GetName());

    // Get optional parameters
    bool bIsPure = false;
    if (FunctionParams.IsValid())
    {
        FunctionParams->TryGetBoolField(TEXT("is_pure"), bIsPure);
    }

    FString Category = TEXT("Default");
    if (FunctionParams.IsValid())
    {
        FunctionParams->TryGetStringField(TEXT("category"), Category);
    }

    // Check if a function graph with this name already exists
    UEdGraph* ExistingGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            ExistingGraph = Graph;
            break;
        }
    }

    if (ExistingGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Function '%s' already exists in Blueprint '%s'"), *FunctionName, *Blueprint->GetName());
        return false;
    }

    // Create the function graph using the working UMG pattern
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());

    if (!FuncGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Failed to create function graph"));
        return false;
    }

    // Add the function graph to the blueprint as a user-defined function
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, bIsPure, nullptr);

    // Mark this as a user-defined function to make it editable
    FuncGraph->Schema = UEdGraphSchema_K2::StaticClass();
    FuncGraph->bAllowDeletion = true;
    FuncGraph->bAllowRenaming = true;

    // Find the automatically created function entry node instead of creating a new one
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        if (UK2Node_FunctionEntry* ExistingEntry = Cast<UK2Node_FunctionEntry>(Node))
        {
            EntryNode = ExistingEntry;
            break;
        }
    }

    if (!EntryNode)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Failed to find function entry node"));
        return false;
    }

    // Position the entry node
    EntryNode->NodePosX = 0;
    EntryNode->NodePosY = 0;

    // Make sure the function is marked as BlueprintCallable and user-defined
    EntryNode->SetExtraFlags(FUNC_BlueprintCallable);

    // Remove any flags that would make it non-editable
    EntryNode->ClearExtraFlags(FUNC_BlueprintEvent);

    // Set metadata to ensure it's treated as a user function
    EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_CallInEditor, FString(TEXT("true")));
    EntryNode->MetaData.RemoveMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly);

    // Ensure the function signature is properly set up for user editing
    EntryNode->bCanRenameNode = true;

    // Clear any existing user defined pins to avoid duplicates
    EntryNode->UserDefinedPins.Empty();

    // Process input parameters
    if (FunctionParams.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* InputsArray = nullptr;
        if (FunctionParams->TryGetArrayField(TEXT("inputs"), InputsArray))
        {
            for (const auto& InputValue : *InputsArray)
            {
                const TSharedPtr<FJsonObject>& InputObj = InputValue->AsObject();
                if (InputObj.IsValid())
                {
                    FString ParamName;
                    FString ParamType;
                    if (InputObj->TryGetStringField(TEXT("name"), ParamName) && InputObj->TryGetStringField(TEXT("type"), ParamType))
                    {
                        // Convert string to pin type
                        FEdGraphPinType PinType;
                        if (ConvertStringToPinTypeFunc(ParamType, PinType))
                        {
                            // Add input parameter to function entry
                            EntryNode->UserDefinedPins.Add(MakeShared<FUserPinInfo>());
                            FUserPinInfo& NewPin = *EntryNode->UserDefinedPins.Last();
                            NewPin.PinName = FName(*ParamName);
                            NewPin.PinType = PinType;
                            NewPin.DesiredPinDirection = EGPD_Output; // Entry node outputs are function inputs

                            UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Added input parameter '%s' of type '%s'"),
                                *ParamName, *ParamType);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Unknown parameter type '%s' for input '%s'"),
                                *ParamType, *ParamName);
                        }
                    }
                }
            }
        }

        // Process output parameters
        const TArray<TSharedPtr<FJsonValue>>* OutputsArray = nullptr;
        if (FunctionParams->TryGetArrayField(TEXT("outputs"), OutputsArray) && OutputsArray->Num() > 0)
        {
            // Create function result node for outputs
            UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
            FuncGraph->AddNode(ResultNode, false, false);
            ResultNode->NodePosX = 400;
            ResultNode->NodePosY = 0;

            // Clear any existing user defined pins to avoid duplicates
            ResultNode->UserDefinedPins.Empty();

            for (const auto& OutputValue : *OutputsArray)
            {
                const TSharedPtr<FJsonObject>& OutputObj = OutputValue->AsObject();
                if (OutputObj.IsValid())
                {
                    FString ParamName;
                    FString ParamType;
                    if (OutputObj->TryGetStringField(TEXT("name"), ParamName) && OutputObj->TryGetStringField(TEXT("type"), ParamType))
                    {
                        // Convert string to pin type for output
                        FEdGraphPinType PinType;
                        if (ConvertStringToPinTypeFunc(ParamType, PinType))
                        {
                            // Add output parameter to function result
                            ResultNode->UserDefinedPins.Add(MakeShared<FUserPinInfo>());
                            FUserPinInfo& NewPin = *ResultNode->UserDefinedPins.Last();
                            NewPin.PinName = FName(*ParamName);
                            NewPin.PinType = PinType;
                            NewPin.DesiredPinDirection = EGPD_Input; // Result node inputs are function outputs

                            UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Added output parameter '%s' of type '%s'"),
                                *ParamName, *ParamType);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Unknown parameter type '%s' for output '%s'"),
                                *ParamType, *ParamName);
                        }
                    }
                }
            }

            // Allocate pins for result node after adding all outputs
            ResultNode->AllocateDefaultPins();
            // Reconstruct the result node to immediately update the visual representation
            ResultNode->ReconstructNode();
        }
        else
        {
            // CRITICAL: Even if there are no outputs, we MUST create a result node
            // for the execution flow connection. Without it, the function will never execute!
            UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
            FuncGraph->AddNode(ResultNode, false, false);
            ResultNode->NodePosX = 400;
            ResultNode->NodePosY = 0;

            // Allocate pins for result node
            ResultNode->AllocateDefaultPins();
            // Reconstruct the result node
            ResultNode->ReconstructNode();

            UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Created result node for execution flow (no outputs)"));
        }
    }

    // Allocate pins for entry node AFTER setting up user defined pins
    EntryNode->AllocateDefaultPins();
    // Reconstruct the entry node to immediately update the visual representation
    EntryNode->ReconstructNode();

    // CRITICAL FIX: Connect execution flow between entry and return nodes
    // Without this, the function will NEVER execute - it becomes a dead method!
    // This applies to BOTH pure and impure functions.
    {
        UE_LOG(LogTemp, Warning, TEXT("=== EXECUTION FLOW CONNECTION DEBUG START ==="));
        UE_LOG(LogTemp, Warning, TEXT("Total nodes in graph: %d"), FuncGraph->Nodes.Num());

        // Find the return/result node in the function graph
        UK2Node_FunctionResult* ResultNode = nullptr;
        for (UEdGraphNode* Node : FuncGraph->Nodes)
        {
            UE_LOG(LogTemp, Warning, TEXT("Node found: %s (Type: %s)"),
                *Node->GetName(),
                *Node->GetClass()->GetName());

            if (UK2Node_FunctionResult* ExistingResult = Cast<UK2Node_FunctionResult>(Node))
            {
                ResultNode = ExistingResult;
                UE_LOG(LogTemp, Warning, TEXT(">>> Found ResultNode: %s"), *ResultNode->GetName());
                break;
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("EntryNode: %s"), EntryNode ? *EntryNode->GetName() : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("ResultNode: %s"), ResultNode ? *ResultNode->GetName() : TEXT("NULL"));

        // If we have both entry and result nodes, connect their execution pins
        if (EntryNode && ResultNode)
        {
            UE_LOG(LogTemp, Warning, TEXT("Both nodes exist! Searching for execution pins..."));

            // Find execution output pin on entry node (named "then" or similar)
            UEdGraphPin* EntryExecPin = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("EntryNode pins count: %d"), EntryNode->Pins.Num());
            for (UEdGraphPin* Pin : EntryNode->Pins)
            {
                if (Pin)
                {
                    UE_LOG(LogTemp, Warning, TEXT("  EntryNode Pin: '%s' | Category: %s | Direction: %d | IsExec: %s"),
                        *Pin->PinName.ToString(),
                        *Pin->PinType.PinCategory.ToString(),
                        (int32)Pin->Direction,
                        (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) ? TEXT("YES") : TEXT("NO"));

                    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
                    {
                        EntryExecPin = Pin;
                        UE_LOG(LogTemp, Warning, TEXT("  >>> FOUND ENTRY EXEC PIN: %s"), *Pin->PinName.ToString());
                    }
                }
            }

            // Find execution input pin on result node
            UEdGraphPin* ResultExecPin = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("ResultNode pins count: %d"), ResultNode->Pins.Num());
            for (UEdGraphPin* Pin : ResultNode->Pins)
            {
                if (Pin)
                {
                    UE_LOG(LogTemp, Warning, TEXT("  ResultNode Pin: '%s' | Category: %s | Direction: %d | IsExec: %s"),
                        *Pin->PinName.ToString(),
                        *Pin->PinType.PinCategory.ToString(),
                        (int32)Pin->Direction,
                        (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) ? TEXT("YES") : TEXT("NO"));

                    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Input)
                    {
                        ResultExecPin = Pin;
                        UE_LOG(LogTemp, Warning, TEXT("  >>> FOUND RESULT EXEC PIN: %s"), *Pin->PinName.ToString());
                    }
                }
            }

            // Make the connection
            if (EntryExecPin && ResultExecPin)
            {
                UE_LOG(LogTemp, Warning, TEXT("Attempting to connect: %s -> %s"),
                    *EntryExecPin->PinName.ToString(),
                    *ResultExecPin->PinName.ToString());

                EntryExecPin->MakeLinkTo(ResultExecPin);

                UE_LOG(LogTemp, Warning, TEXT("Connection made! Links on EntryExecPin: %d"), EntryExecPin->LinkedTo.Num());
                UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Connected execution flow between entry and return nodes"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Could not find execution pins to connect (Entry: %s, Result: %s)"),
                    EntryExecPin ? TEXT("Found") : TEXT("Missing"),
                    ResultExecPin ? TEXT("Found") : TEXT("Missing"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Could not find result node to connect execution flow (EntryNode: %s, ResultNode: %s)"),
                EntryNode ? TEXT("Found") : TEXT("Missing"),
                ResultNode ? TEXT("Found") : TEXT("Missing"));
        }

        UE_LOG(LogTemp, Warning, TEXT("=== EXECUTION FLOW CONNECTION DEBUG END ==="));
    }

    // Mark blueprint as structurally modified
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    // Force refresh of blueprint nodes and UI to ensure proper pin connectivity
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

    // Invalidate cache since blueprint was modified
    Cache.InvalidateBlueprint(Blueprint->GetName());

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CreateCustomBlueprintFunction: Successfully created function '%s' with parameters"), *FunctionName);
    return true;
}

bool FBlueprintFunctionService::SpawnBlueprintActor(UBlueprint* Blueprint, const FString& ActorName, const FVector& Location, const FRotator& Rotation)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SpawnBlueprintActor: Spawning actor '%s' from blueprint '%s'"),
        *ActorName, *Blueprint->GetName());

    // Get the world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: No valid world found"));
        return false;
    }

    // Get the blueprint's generated class
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    if (!BlueprintClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: Blueprint has no generated class"));
        return false;
    }

    // Spawn the actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* SpawnedActor = World->SpawnActor<AActor>(BlueprintClass, Location, Rotation, SpawnParams);
    if (!SpawnedActor)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::SpawnBlueprintActor: Failed to spawn actor"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::SpawnBlueprintActor: Successfully spawned actor '%s'"), *ActorName);
    return true;
}

bool FBlueprintFunctionService::CallBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TArray<FString>& Parameters)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: Invalid blueprint"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CallBlueprintFunction: Calling function '%s' on blueprint '%s'"),
        *FunctionName, *Blueprint->GetName());

    // Get the blueprint's generated class
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    if (!BlueprintClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: Blueprint has no generated class"));
        return false;
    }

    // Find the function
    UFunction* Function = BlueprintClass->FindFunctionByName(*FunctionName);
    if (!Function)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: Function '%s' not found"), *FunctionName);
        return false;
    }

    // Get the default object to call the function on
    UObject* DefaultObject = BlueprintClass->GetDefaultObject();
    if (!DefaultObject)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CallBlueprintFunction: No default object available"));
        return false;
    }

    // Call the function (simplified - would need proper parameter handling for real implementation)
    DefaultObject->ProcessEvent(Function, nullptr);

    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CallBlueprintFunction: Successfully called function '%s'"), *FunctionName);
    return true;
}
