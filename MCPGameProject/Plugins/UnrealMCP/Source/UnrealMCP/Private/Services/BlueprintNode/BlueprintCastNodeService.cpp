#include "Services/BlueprintNode/BlueprintCastNodeService.h"
#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Utils/GraphUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"

FBlueprintCastNodeService& FBlueprintCastNodeService::Get()
{
    static FBlueprintCastNodeService Instance;
    return Instance;
}

bool FBlueprintCastNodeService::ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType)
{
    // Execution pins are always compatible with execution pins
    if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Exec && TargetPinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        return true;
    }

    // Exact type match
    if (SourcePinType.PinCategory == TargetPinType.PinCategory)
    {
        // For basic types, category match is sufficient
        if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Int ||
            SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
            SourcePinType.PinCategory == UEdGraphSchema_K2::PC_String ||
            SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
        {
            return true;
        }

        // For object types, check subcategory
        if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Object)
        {
            return SourcePinType.PinSubCategoryObject == TargetPinType.PinSubCategoryObject;
        }

        // For struct types, check subcategory
        if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
        {
            return SourcePinType.PinSubCategoryObject == TargetPinType.PinSubCategoryObject;
        }
    }

    // Some implicit conversions that don't need cast nodes
    // Int to Float
    if (SourcePinType.PinCategory == UEdGraphSchema_K2::PC_Int && TargetPinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        return true;
    }

    return false;
}

bool FBlueprintCastNodeService::DoesCastNeed(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    if (!SourcePin || !TargetPin)
    {
        return false;
    }

    // Check for primitive type mismatches that need casting
    // Int/Float/Bool -> String conversions
    if ((SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int ||
         SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
         SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean) &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return true;
    }

    // String -> Int/Float conversions
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String &&
        (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int ||
         TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real))
    {
        return true;
    }

    // For object types, check if types are compatible without casting
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
    {
        UClass* SourceClass = Cast<UClass>(SourcePin->PinType.PinSubCategoryObject.Get());
        UClass* TargetClass = Cast<UClass>(TargetPin->PinType.PinSubCategoryObject.Get());

        if (SourceClass && TargetClass)
        {
            bool bSourceIsChildOfTarget = SourceClass->IsChildOf(TargetClass);
            bool bTargetIsChildOfSource = TargetClass->IsChildOf(SourceClass);
            bool bSameClass = (SourceClass == TargetClass);

            // Only need cast if target is MORE SPECIFIC than source (downcasting)
            return bTargetIsChildOfSource && !bSameClass && !bSourceIsChildOfTarget;
        }
    }

    return false;
}

bool FBlueprintCastNodeService::CreateCastNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    if (!Graph || !SourcePin || !TargetPin)
    {
        return false;
    }

    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return false;
    }

    // Handle common cast scenarios

    // Integer to String cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return CreateIntToStringCast(Graph, SourcePin, TargetPin);
    }

    // Float to String cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return CreateFloatToStringCast(Graph, SourcePin, TargetPin);
    }

    // Boolean to String cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return CreateBoolToStringCast(Graph, SourcePin, TargetPin);
    }

    // String to Int cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
    {
        return CreateStringToIntCast(Graph, SourcePin, TargetPin);
    }

    // String to Float cast
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        return CreateStringToFloatCast(Graph, SourcePin, TargetPin);
    }

    // Object to Object cast (e.g., ActorComponent -> SplineComponent)
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
    {
        return CreateObjectCast(Graph, SourcePin, TargetPin);
    }

    UE_LOG(LogTemp, Warning, TEXT("CreateCastNode: No cast implementation for %s to %s"),
           *SourcePin->PinType.PinCategory.ToString(),
           *TargetPin->PinType.PinCategory.ToString());

    return false;
}

bool FBlueprintCastNodeService::CreateConversionNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin,
                                                      const TCHAR* LibraryClassName, const TCHAR* FunctionName, const TCHAR* InputPinName)
{
    // Find the library class
    FString LibraryPath = FString::Printf(TEXT("/Script/Engine.%s"), LibraryClassName);
    UClass* Library = FindObject<UClass>(nullptr, *LibraryPath);
    if (!Library)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateConversionNode: Could not find %s"), LibraryClassName);
        return false;
    }

    UFunction* ConvFunction = Library->FindFunctionByName(FunctionName);
    if (!ConvFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateConversionNode: Could not find %s function"), FunctionName);
        return false;
    }

    // Create the conversion node
    UK2Node_CallFunction* ConvNode = NewObject<UK2Node_CallFunction>(Graph);
    ConvNode->SetFromFunction(ConvFunction);

    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;

    ConvNode->NodePosX = CastPos.X;
    ConvNode->NodePosY = CastPos.Y;

    Graph->AddNode(ConvNode, true);
    ConvNode->PostPlacedNewNode();
    ConvNode->AllocateDefaultPins();

    // Find the input and output pins on the conversion node
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(InputPinName, EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);

    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateConversionNode: Successfully created %s node"), FunctionName);
    return true;
}

bool FBlueprintCastNodeService::CreateIntToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return CreateConversionNode(Graph, SourcePin, TargetPin,
        TEXT("KismetStringLibrary"), TEXT("Conv_IntToString"), TEXT("InInt"));
}

bool FBlueprintCastNodeService::CreateFloatToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return CreateConversionNode(Graph, SourcePin, TargetPin,
        TEXT("KismetStringLibrary"), TEXT("Conv_FloatToString"), TEXT("InFloat"));
}

bool FBlueprintCastNodeService::CreateBoolToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return CreateConversionNode(Graph, SourcePin, TargetPin,
        TEXT("KismetStringLibrary"), TEXT("Conv_BoolToString"), TEXT("InBool"));
}

bool FBlueprintCastNodeService::CreateStringToIntCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return CreateConversionNode(Graph, SourcePin, TargetPin,
        TEXT("KismetStringLibrary"), TEXT("Conv_StringToInt"), TEXT("InString"));
}

bool FBlueprintCastNodeService::CreateStringToFloatCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    return CreateConversionNode(Graph, SourcePin, TargetPin,
        TEXT("KismetStringLibrary"), TEXT("Conv_StringToFloat"), TEXT("InString"));
}

bool FBlueprintCastNodeService::CreateObjectCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin,
                                                  FAutoInsertedNodeInfo* OutNodeInfo)
{
    if (!Graph || !SourcePin || !TargetPin)
    {
        return false;
    }

    // Get the target class from the target pin
    UClass* TargetClass = Cast<UClass>(TargetPin->PinType.PinSubCategoryObject.Get());
    if (!TargetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateObjectCast: Could not determine target class from pin"));
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("CreateObjectCast: Creating cast to %s"), *TargetClass->GetName());

    // Create a dynamic cast node
    UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(Graph);
    CastNode->TargetType = TargetClass;

    // Position the cast node between source and target
    FVector2D SourcePos(SourcePin->GetOwningNode()->NodePosX, SourcePin->GetOwningNode()->NodePosY);
    FVector2D TargetPos(TargetPin->GetOwningNode()->NodePosX, TargetPin->GetOwningNode()->NodePosY);
    FVector2D CastPos = (SourcePos + TargetPos) * 0.5f;

    CastNode->NodePosX = CastPos.X;
    CastNode->NodePosY = CastPos.Y;

    Graph->AddNode(CastNode, true);
    CastNode->PostPlacedNewNode();
    CastNode->AllocateDefaultPins();

    // Find the input and output pins on the cast node
    UEdGraphPin* CastInputPin = nullptr;
    UEdGraphPin* CastOutputPin = nullptr;

    for (UEdGraphPin* Pin : CastNode->Pins)
    {
        // The Cast node input pin is named "Object" and has PC_Wildcard category
        if (Pin->Direction == EGPD_Input &&
            (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard ||
             Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object) &&
            Pin->PinName.ToString() == TEXT("Object"))
        {
            CastInputPin = Pin;
        }
        // The Cast node output pin starts with "As" and has PC_Object or PC_Interface category
        else if (Pin->Direction == EGPD_Output &&
                 (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
                  Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Interface) &&
                 Pin->PinName.ToString().StartsWith(TEXT("As")))
        {
            CastOutputPin = Pin;
        }
    }

    if (!CastInputPin || !CastOutputPin)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateObjectCast: Could not find input/output pins on cast node to %s"),
            *TargetClass->GetName());
        UE_LOG(LogTemp, Error, TEXT("  Available pins on cast node:"));
        for (UEdGraphPin* Pin : CastNode->Pins)
        {
            UE_LOG(LogTemp, Error, TEXT("    - '%s': Category=%s, Direction=%s"),
                *Pin->PinName.ToString(),
                *Pin->PinType.PinCategory.ToString(),
                Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
        }
        Graph->RemoveNode(CastNode);
        return false;
    }

    // Connect: Source -> Cast Input -> Cast Output -> Target
    SourcePin->MakeLinkTo(CastInputPin);
    CastOutputPin->MakeLinkTo(TargetPin);

    // Populate auto-inserted node info if requested
    if (OutNodeInfo)
    {
        OutNodeInfo->NodeId = FGraphUtils::GetReliableNodeId(CastNode);
        OutNodeInfo->NodeTitle = CastNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
        OutNodeInfo->NodeType = CastNode->GetClass()->GetName();
        OutNodeInfo->bRequiresExecConnection = true;  // Dynamic casts always need exec pins

        // Check if exec pins are connected
        bool bHasExecInput = false;
        bool bHasExecOutput = false;
        for (UEdGraphPin* Pin : CastNode->Pins)
        {
            if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
                {
                    bHasExecInput = true;
                }
                else if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                {
                    bHasExecOutput = true;
                }
            }
        }
        OutNodeInfo->bExecConnected = bHasExecInput && bHasExecOutput;
    }

    UE_LOG(LogTemp, Display, TEXT("CreateObjectCast: Successfully created object cast node to %s"), *TargetClass->GetName());
    return true;
}
