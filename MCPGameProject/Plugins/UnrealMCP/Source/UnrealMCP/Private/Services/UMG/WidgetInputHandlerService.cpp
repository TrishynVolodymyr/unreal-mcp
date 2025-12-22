#include "Services/UMG/WidgetInputHandlerService.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EditorAssetLibrary.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EdGraphSchema_K2.h"

bool FWidgetInputHandlerService::CreateWidgetInputHandler(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName,
                                                         const FString& InputType, const FString& InputEvent,
                                                         const FString& Trigger, const FString& HandlerName,
                                                         FString& OutActualHandlerName)
{
    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Creating input handler '%s' for %s %s"),
           *HandlerName, *InputType, *InputEvent);

    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Widget blueprint is null"));
        return false;
    }

    // If component name is specified, verify it exists
    if (!ComponentName.IsEmpty())
    {
        if (!WidgetBlueprint->WidgetTree || !WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName)))
        {
            UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Component '%s' not found"),
                   *ComponentName);
            return false;
        }
    }

    OutActualHandlerName = HandlerName;

    // Step 1: Create the custom event that will be called when input is detected
    if (!CreateCustomInputEvent(WidgetBlueprint, HandlerName))
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Failed to create custom event '%s'"), *HandlerName);
        return false;
    }

    // Step 2: Get or create the input override function (e.g., OnMouseButtonDown)
    UEdGraph* OverrideGraph = GetOrCreateInputOverrideFunction(WidgetBlueprint, InputType, Trigger);
    if (!OverrideGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Failed to create/find override function for %s %s"),
               *InputType, *Trigger);
        return false;
    }

    // Step 3: Add the input checking logic to call the custom event
    if (!AddInputCheckingLogic(WidgetBlueprint, OverrideGraph, InputType, InputEvent, Trigger, HandlerName, ComponentName))
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Failed to add input checking logic"));
        return false;
    }

    // Compile and save
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::CreateWidgetInputHandler - Successfully created input handler '%s'"), *HandlerName);
    return true;
}

bool FWidgetInputHandlerService::RemoveWidgetFunctionGraph(UWidgetBlueprint* WidgetBlueprint, const FString& FunctionName)
{
    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::RemoveWidgetFunctionGraph - Removing function graph '%s'"),
           *FunctionName);

    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::RemoveWidgetFunctionGraph - Widget blueprint is null"));
        return false;
    }

    // Find the function graph by name
    UEdGraph* GraphToRemove = nullptr;
    for (UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            GraphToRemove = Graph;
            break;
        }
    }

    if (!GraphToRemove)
    {
        UE_LOG(LogTemp, Warning, TEXT("FWidgetInputHandlerService::RemoveWidgetFunctionGraph - Function graph '%s' not found"),
               *FunctionName);
        return false;
    }

    // Remove the graph using BlueprintEditorUtils
    FBlueprintEditorUtils::RemoveGraph(WidgetBlueprint, GraphToRemove, EGraphRemoveFlags::Default);

    // Mark as modified and save
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::RemoveWidgetFunctionGraph - Successfully removed function graph '%s'"), *FunctionName);
    return true;
}

UEdGraph* FWidgetInputHandlerService::GetOrCreateInputOverrideFunction(UWidgetBlueprint* WidgetBlueprint, const FString& InputType, const FString& Trigger)
{
    FString FunctionName = GetOverrideFunctionName(InputType, Trigger);

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - Looking for function override '%s'"), *FunctionName);

    // Check if the function graph already exists
    for (UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - Found existing function graph '%s'"), *FunctionName);
            return Graph;
        }
    }

    // Find the parent function to get its signature
    UClass* ParentClass = WidgetBlueprint->ParentClass;
    if (!ParentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - No parent class found"));
        return nullptr;
    }

    UFunction* ParentFunction = ParentClass->FindFunctionByName(FName(*FunctionName));
    if (!ParentFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - Function '%s' not found in parent class '%s'"),
               *FunctionName, *ParentClass->GetName());
        return nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - Creating override function graph '%s'"), *FunctionName);

    // Create a new function graph for the override
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        WidgetBlueprint,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );

    if (!FuncGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - Failed to create function graph"));
        return nullptr;
    }

    // Add the function graph as an override - pass the UClass* that owns the function
    // This is critical: passing UClass* makes AddFunctionGraph use CreateFunctionGraphTerminators(Graph, Class)
    // which properly sets up the function signature from the parent class
    UClass* OverrideFuncClass = ParentFunction->GetOwnerClass();
    FBlueprintEditorUtils::AddFunctionGraph(WidgetBlueprint, FuncGraph, false, OverrideFuncClass);

    // Mark the graph as editable
    FuncGraph->bEditable = true;
    FuncGraph->bAllowDeletion = false; // Override functions shouldn't be deletable
    FuncGraph->bAllowRenaming = false; // Override functions shouldn't be renamed

    // Find or create the function entry node
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
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - No entry node found in function graph"));
        return nullptr;
    }

    // Ensure pins are allocated properly
    EntryNode->ReconstructNode();

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::GetOrCreateInputOverrideFunction - Created override function graph '%s'"), *FunctionName);
    return FuncGraph;
}

bool FWidgetInputHandlerService::CreateCustomInputEvent(UWidgetBlueprint* WidgetBlueprint, const FString& HandlerName)
{
    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::CreateCustomInputEvent - Creating custom event '%s'"), *HandlerName);

    // Find the event graph
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
    if (!EventGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::CreateCustomInputEvent - Could not find event graph"));
        return false;
    }

    // Check if event already exists
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
        {
            if (EventNode->GetFunctionName() == FName(*HandlerName))
            {
                UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::CreateCustomInputEvent - Event '%s' already exists"), *HandlerName);
                return true; // Already exists, that's fine
            }
        }
    }

    // Calculate position for new node
    float MaxHeight = 0.0f;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        MaxHeight = FMath::Max(MaxHeight, (float)Node->NodePosY);
    }

    // Create a custom event node
    UK2Node_Event* NewEventNode = NewObject<UK2Node_Event>(EventGraph);
    NewEventNode->CustomFunctionName = FName(*HandlerName);
    NewEventNode->bIsEditable = true;
    NewEventNode->NodePosX = 200;
    NewEventNode->NodePosY = static_cast<int32>(MaxHeight + 200);

    EventGraph->AddNode(NewEventNode, true, false);
    NewEventNode->CreateNewGuid();
    NewEventNode->PostPlacedNewNode();
    NewEventNode->AllocateDefaultPins();

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::CreateCustomInputEvent - Created custom event '%s'"), *HandlerName);
    return true;
}

bool FWidgetInputHandlerService::AddInputCheckingLogic(UWidgetBlueprint* WidgetBlueprint, UEdGraph* FuncGraph,
                                                      const FString& InputType, const FString& InputEvent,
                                                      const FString& Trigger, const FString& HandlerName,
                                                      const FString& ComponentName)
{
    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::AddInputCheckingLogic - Adding logic for %s %s (%s) -> %s"),
           *InputType, *InputEvent, *Trigger, *HandlerName);

    // Find the function entry node in the function graph
    UK2Node_FunctionEntry* EntryNode = nullptr;
    UK2Node_FunctionResult* ResultNode = nullptr;

    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        if (!EntryNode)
        {
            EntryNode = Cast<UK2Node_FunctionEntry>(Node);
        }
        if (!ResultNode)
        {
            ResultNode = Cast<UK2Node_FunctionResult>(Node);
        }
        if (EntryNode && ResultNode) break;
    }

    if (!EntryNode)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetInputHandlerService::AddInputCheckingLogic - Could not find function entry node"));
        return false;
    }

    // Find the execution output pin and return value pin on the result node
    UEdGraphPin* EntryExecPin = nullptr;
    for (UEdGraphPin* Pin : EntryNode->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
        {
            EntryExecPin = Pin;
            break;
        }
    }

    // Find or create result node's return value pin
    UEdGraphPin* ReturnValuePin = nullptr;
    if (ResultNode)
    {
        for (UEdGraphPin* Pin : ResultNode->Pins)
        {
            if (Pin && Pin->PinName == UEdGraphSchema_K2::PN_ReturnValue)
            {
                ReturnValuePin = Pin;
                break;
            }
        }
    }

    // Create a "Handled" node to return FEventReply::Handled()
    // Use UWidgetBlueprintLibrary::Handled() function
    UFunction* HandledFunction = UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(TEXT("Handled"));

    if (HandledFunction)
    {
        UK2Node_CallFunction* HandledNode = NewObject<UK2Node_CallFunction>(FuncGraph);
        HandledNode->SetFromFunction(HandledFunction);
        HandledNode->NodePosX = EntryNode->NodePosX + 300;
        HandledNode->NodePosY = EntryNode->NodePosY;

        FuncGraph->AddNode(HandledNode, true, false);
        HandledNode->CreateNewGuid();
        HandledNode->PostPlacedNewNode();
        HandledNode->AllocateDefaultPins();

        // Connect the Handled node's return to the result node's return value
        UEdGraphPin* HandledReturnPin = HandledNode->GetReturnValuePin();

        if (ResultNode && HandledReturnPin)
        {
            // Find the result's input pin for the return value (struct pin)
            UEdGraphPin* ResultInputPin = nullptr;
            for (UEdGraphPin* Pin : ResultNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input &&
                    Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                {
                    ResultInputPin = Pin;
                    break;
                }
            }

            if (ResultInputPin)
            {
                HandledReturnPin->MakeLinkTo(ResultInputPin);
                UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::AddInputCheckingLogic - Connected Handled() to result node"));
            }
        }

        // Connect entry execution to handled node (if it has exec pin)
        UEdGraphPin* HandledExecPin = nullptr;
        for (UEdGraphPin* Pin : HandledNode->Pins)
        {
            if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Input)
            {
                HandledExecPin = Pin;
                break;
            }
        }

        // Also connect to result node execution if exists
        if (ResultNode && EntryExecPin)
        {
            UEdGraphPin* ResultExecPin = nullptr;
            for (UEdGraphPin* Pin : ResultNode->Pins)
            {
                if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Input)
                {
                    ResultExecPin = Pin;
                    break;
                }
            }

            if (ResultExecPin)
            {
                EntryExecPin->MakeLinkTo(ResultExecPin);
                UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::AddInputCheckingLogic - Connected entry to result exec"));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FWidgetInputHandlerService::AddInputCheckingLogic - Could not find UWidgetBlueprintLibrary::Handled"));
    }

    UE_LOG(LogTemp, Log, TEXT("FWidgetInputHandlerService::AddInputCheckingLogic - Added input handling logic for '%s'"), *HandlerName);
    return true;
}

FName FWidgetInputHandlerService::GetKeyNameForInputEvent(const FString& InputEvent)
{
    // Map our friendly names to UE key names
    static TMap<FString, FName> KeyNameMap = {
        {TEXT("LeftMouseButton"), FName(TEXT("LeftMouseButton"))},
        {TEXT("RightMouseButton"), FName(TEXT("RightMouseButton"))},
        {TEXT("MiddleMouseButton"), FName(TEXT("MiddleMouseButton"))},
        {TEXT("ThumbMouseButton"), FName(TEXT("ThumbMouseButton"))},
        {TEXT("ThumbMouseButton2"), FName(TEXT("ThumbMouseButton2"))},
        {TEXT("Enter"), FName(TEXT("Enter"))},
        {TEXT("Escape"), FName(TEXT("Escape"))},
        {TEXT("SpaceBar"), FName(TEXT("SpaceBar"))},
        {TEXT("Tab"), FName(TEXT("Tab"))},
    };

    const FName* FoundName = KeyNameMap.Find(InputEvent);
    if (FoundName)
    {
        return *FoundName;
    }

    // For any other key, use the input event directly as the key name
    return FName(*InputEvent);
}

FString FWidgetInputHandlerService::GetOverrideFunctionName(const FString& InputType, const FString& Trigger)
{
    if (InputType == TEXT("MouseButton"))
    {
        if (Trigger == TEXT("Pressed"))
        {
            return TEXT("OnMouseButtonDown");
        }
        else if (Trigger == TEXT("Released"))
        {
            return TEXT("OnMouseButtonUp");
        }
        else if (Trigger == TEXT("DoubleClick"))
        {
            return TEXT("OnMouseButtonDoubleClick");
        }
    }
    else if (InputType == TEXT("Key"))
    {
        if (Trigger == TEXT("Pressed"))
        {
            return TEXT("OnKeyDown");
        }
        else if (Trigger == TEXT("Released"))
        {
            return TEXT("OnKeyUp");
        }
    }
    else if (InputType == TEXT("Touch"))
    {
        return TEXT("OnTouchGesture");
    }
    else if (InputType == TEXT("Focus"))
    {
        if (Trigger == TEXT("FocusReceived") || Trigger == TEXT("Pressed"))
        {
            return TEXT("OnFocusReceived");
        }
        else
        {
            return TEXT("OnFocusLost");
        }
    }
    else if (InputType == TEXT("Drag"))
    {
        return TEXT("OnDragDetected");
    }

    // Default fallback
    return TEXT("OnMouseButtonDown");
}
