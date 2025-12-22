#include "Services/UMG/WidgetBindingService.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "EditorAssetLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_VariableGet.h"
#include "K2Node_ComponentBoundEvent.h"

bool FWidgetBindingService::CreateEventBinding(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget,
                                               const FString& WidgetVarName, const FString& EventName,
                                               const FString& FunctionName)
{
    if (!WidgetBlueprint || !Widget)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetBindingService: Invalid widget blueprint or widget"));
        return false;
    }

    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
    if (!EventGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetBindingService: Failed to find event graph"));
        return false;
    }

    FName EventFName(*EventName);
    FName WidgetVarFName(*WidgetVarName);

    // Find the widget's variable property in the generated class
    // Widget Blueprints expose widgets as FObjectProperty pointing to the widget class
    FObjectProperty* WidgetProperty = nullptr;
    if (WidgetBlueprint->GeneratedClass)
    {
        WidgetProperty = FindFProperty<FObjectProperty>(WidgetBlueprint->GeneratedClass, WidgetVarFName);
    }

    if (!WidgetProperty)
    {
        // Widget variable not found - this can happen if the widget was just exposed
        // Need to compile the blueprint first
        UE_LOG(LogTemp, Warning, TEXT("WidgetBindingService: Widget property '%s' not found in GeneratedClass. Compiling blueprint first."), *WidgetVarName);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

        // Try again after compilation
        if (WidgetBlueprint->GeneratedClass)
        {
            WidgetProperty = FindFProperty<FObjectProperty>(WidgetBlueprint->GeneratedClass, WidgetVarFName);
        }

        if (!WidgetProperty)
        {
            UE_LOG(LogTemp, Error, TEXT("WidgetBindingService: Widget property '%s' still not found after compilation"), *WidgetVarName);
            return false;
        }
    }

    // Find the delegate property on the widget's class
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(Widget->GetClass(), EventFName);
    if (!DelegateProperty)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetBindingService: Could not find delegate property '%s' on class '%s'"), *EventName, *Widget->GetClass()->GetName());
        return false;
    }

    // Check if this event binding already exists
    TArray<UK2Node_ComponentBoundEvent*> AllBoundEvents;
    FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_ComponentBoundEvent>(WidgetBlueprint, AllBoundEvents);

    for (UK2Node_ComponentBoundEvent* ExistingNode : AllBoundEvents)
    {
        if (ExistingNode->GetComponentPropertyName() == WidgetVarFName &&
            ExistingNode->DelegatePropertyName == EventFName)
        {
            UE_LOG(LogTemp, Warning, TEXT("WidgetBindingService: Event '%s' is already bound to widget '%s'"), *EventName, *WidgetVarName);
            return true; // Already bound, consider it success
        }
    }

    // Calculate position for new node
    float MaxHeight = 0.0f;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        MaxHeight = FMath::Max(MaxHeight, (float)Node->NodePosY);
    }
    const int32 NodePosX = 200;
    const int32 NodePosY = static_cast<int32>(MaxHeight + 200);

    // Create UK2Node_ComponentBoundEvent - this is the correct node type for widget event bindings
    UK2Node_ComponentBoundEvent* BoundEventNode = NewObject<UK2Node_ComponentBoundEvent>(EventGraph);
    if (!BoundEventNode)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetBindingService: Failed to create UK2Node_ComponentBoundEvent"));
        return false;
    }

    // Initialize the component bound event with the widget property and delegate
    BoundEventNode->InitializeComponentBoundEventParams(WidgetProperty, DelegateProperty);
    BoundEventNode->NodePosX = NodePosX;
    BoundEventNode->NodePosY = NodePosY;

    // Add node to graph and set it up
    EventGraph->AddNode(BoundEventNode, true, false);
    BoundEventNode->CreateNewGuid();
    BoundEventNode->PostPlacedNewNode();
    BoundEventNode->AllocateDefaultPins();
    BoundEventNode->ReconstructNode();

    UE_LOG(LogTemp, Log, TEXT("WidgetBindingService: Successfully created event binding '%s' for widget '%s'"), *EventName, *WidgetVarName);

    // Save the blueprint
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    return true;
}

bool FWidgetBindingService::CreateTextBlockBindingFunction(UWidgetBlueprint* WidgetBlueprint,
                                                          const FString& TextBlockName,
                                                          const FString& BindingName,
                                                          const FString& VariableType)
{
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetBindingService: Widget blueprint is null"));
        return false;
    }

    const FString FunctionName = FString::Printf(TEXT("Get%s"), *BindingName);

    // Check if function already exists
    bool bFunctionExists = false;
    for (UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            bFunctionExists = true;
            break;
        }
    }

    // Check if binding already exists
    bool bBindingExists = false;
    for (const FDelegateEditorBinding& ExistingBinding : WidgetBlueprint->Bindings)
    {
        if (ExistingBinding.ObjectName == TextBlockName && ExistingBinding.PropertyName == TEXT("Text"))
        {
            bBindingExists = true;
            break;
        }
    }

    // If both function and binding exist, we're done
    if (bFunctionExists && bBindingExists)
    {
        return true;
    }

    // If only function exists but binding doesn't, we still need to create the binding
    if (!bFunctionExists)
    {
        // Create binding function
        UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
            WidgetBlueprint,
            FName(*FunctionName),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass()
        );

        if (!FuncGraph)
        {
            return false;
        }

        FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, FuncGraph, false, nullptr);

        // Find or create entry node
        UK2Node_FunctionEntry* EntryNode = nullptr;
        for (UEdGraphNode* Node : FuncGraph->Nodes)
        {
            EntryNode = Cast<UK2Node_FunctionEntry>(Node);
            if (EntryNode)
            {
                break;
            }
        }

        if (!EntryNode)
        {
            EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
            FuncGraph->AddNode(EntryNode, false, false);
            EntryNode->NodePosX = 0;
            EntryNode->NodePosY = 0;
            EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBlueprint->GeneratedClass);
            EntryNode->AllocateDefaultPins();
        }

        // Create get variable node
        UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
        GetVarNode->VariableReference.SetSelfMember(FName(*BindingName));
        FuncGraph->AddNode(GetVarNode, false, false);
        GetVarNode->NodePosX = 200;
        GetVarNode->NodePosY = 0;
        GetVarNode->AllocateDefaultPins();

        // Create function result node
        UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
        FuncGraph->AddNode(ResultNode, false, false);
        ResultNode->NodePosX = 400;
        ResultNode->NodePosY = 0;
        ResultNode->UserDefinedPins.Empty();

        // Set up return pin type
        FEdGraphPinType ReturnPinType;
        if (VariableType == TEXT("Text"))
        {
            ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        }
        else if (VariableType == TEXT("String"))
        {
            ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (VariableType == TEXT("Int") || VariableType == TEXT("Integer"))
        {
            ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (VariableType == TEXT("Float"))
        {
            ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
            ReturnPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (VariableType == TEXT("Boolean") || VariableType == TEXT("Bool"))
        {
            ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else
        {
            ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        }

        // Add return value pin
        TSharedPtr<FUserPinInfo> ReturnPin = MakeShared<FUserPinInfo>();
        ReturnPin->PinName = TEXT("ReturnValue");
        ReturnPin->PinType = ReturnPinType;
        ReturnPin->DesiredPinDirection = EGPD_Output;
        ResultNode->UserDefinedPins.Add(ReturnPin);
        ResultNode->ReconstructNode();

        // Connect the nodes
        UEdGraphPin* GetVarOutputPin = GetVarNode->FindPin(FName(*BindingName), EGPD_Output);
        UEdGraphPin* ResultInputPin = ResultNode->FindPin(TEXT("ReturnValue"), EGPD_Input);

        if (GetVarOutputPin && ResultInputPin)
        {
            GetVarOutputPin->MakeLinkTo(ResultInputPin);
        }
    } // End of if (!bFunctionExists)

    // CRITICAL FIX: Add the binding entry to WidgetBlueprint->Bindings array
    // This is what makes the binding visible in the UI and connects it at runtime
    if (!bBindingExists)
    {
        FDelegateEditorBinding NewBinding;
        NewBinding.ObjectName = TextBlockName;      // The widget component name (e.g., "TextBlock_1")
        NewBinding.PropertyName = TEXT("Text");     // The property being bound (always "Text" for text blocks)
        NewBinding.FunctionName = FName(*FunctionName);  // The getter function name (e.g., "GetMyVariable")
        NewBinding.SourceProperty = FName(*BindingName); // The source variable name (e.g., "MyVariable")
        NewBinding.Kind = EBindingKind::Function;   // Binding to a function, not a property

        // Get the function's GUID for rename tracking
        for (UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetName() == FunctionName)
            {
                NewBinding.MemberGuid = Graph->GraphGuid;
                break;
            }
        }

        WidgetBlueprint->Bindings.Add(NewBinding);

        UE_LOG(LogTemp, Log, TEXT("WidgetBindingService: Added binding entry for '%s.Text' -> '%s()' (source: '%s')"),
               *TextBlockName, *FunctionName, *BindingName);
    }

    // Save the blueprint
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    return true;
}
