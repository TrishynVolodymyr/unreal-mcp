#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Services/IBlueprintNodeService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_PromotableOperator.h"

// Helper function to generate safe Node IDs (copy from BlueprintNodeService.cpp)
static FString GetSafeNodeId(UEdGraphNode* Node, const FString& NodeTitle)
{
    if (!Node)
    {
        return TEXT("InvalidNode");
    }

    FString NodeId = Node->NodeGuid.ToString();
    if (NodeId.IsEmpty() || NodeId == TEXT("00000000-0000-0000-0000-000000000000") || NodeId == TEXT("00000000000000000000000000000000"))
    {
        // Generate fallback ID using node pointer and title
        FString SafeTitle = NodeTitle.Replace(TEXT(" "), TEXT("_")).Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT(""));
        NodeId = FString::Printf(TEXT("Node_%p_%s"), Node, *SafeTitle);
    }

    return NodeId;
}

FBlueprintNodeConnectionService& FBlueprintNodeConnectionService::Get()
{
    static FBlueprintNodeConnectionService Instance;
    return Instance;
}

bool FBlueprintNodeConnectionService::ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& TargetGraph, TArray<bool>& OutResults)
{
    if (!Blueprint)
    {
        return false;
    }

    OutResults.Empty();
    OutResults.Reserve(Connections.Num());

    bool bAllSucceeded = true;

    // Find the target graph dynamically
    UEdGraph* SearchGraph = nullptr;

    // Try to find the specific graph by name in UbergraphPages
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetFName() == FName(*TargetGraph))
        {
            SearchGraph = Graph;
            break;
        }
    }

    // Also check function graphs
    if (!SearchGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetFName() == FName(*TargetGraph))
            {
                SearchGraph = Graph;
                break;
            }
        }
    }

    // Fallback to EventGraph if not found and TargetGraph is default
    if (!SearchGraph && TargetGraph == TEXT("EventGraph"))
    {
        SearchGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!SearchGraph)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target graph '%s' not found in Blueprint '%s'"), *TargetGraph, *Blueprint->GetName());
        return false;
    }

    for (const FBlueprintNodeConnectionParams& Connection : Connections)
    {
        FString ValidationError;
        if (!Connection.IsValid(ValidationError))
        {
            OutResults.Add(false);
            bAllSucceeded = false;
            continue;
        }

        // Find the nodes by GUID or special identifiers for Entry/Exit nodes
        UEdGraphNode* SourceNode = FindNodeByIdOrType(SearchGraph, Connection.SourceNodeId);
        UEdGraphNode* TargetNode = FindNodeByIdOrType(SearchGraph, Connection.TargetNodeId);

        UE_LOG(LogTemp, Warning, TEXT("Connecting nodes: SourceID='%s' -> SourceNode=%s, TargetID='%s' -> TargetNode=%s"),
            *Connection.SourceNodeId,
            SourceNode ? *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"),
            *Connection.TargetNodeId,
            TargetNode ? *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"));

        if (!SourceNode || !TargetNode)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find nodes for connection: Source=%s, Target=%s"),
                SourceNode ? TEXT("Found") : TEXT("NOT FOUND"),
                TargetNode ? TEXT("Found") : TEXT("NOT FOUND"));
            OutResults.Add(false);
            bAllSucceeded = false;
            continue;
        }

        // Try to connect the nodes with automatic cast node creation if needed
        UE_LOG(LogTemp, Warning, TEXT("Attempting connection: '%s'.'%s' -> '%s'.'%s'"),
            *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
            *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);

        bool bConnectionSucceeded = ConnectNodesWithAutoCast(SearchGraph, SourceNode, Connection.SourcePin, TargetNode, Connection.TargetPin);
        OutResults.Add(bConnectionSucceeded);

        if (!bConnectionSucceeded)
        {
            UE_LOG(LogTemp, Error, TEXT("FINAL RESULT: Connection failed for '%s'.'%s' -> '%s'.'%s'"),
                *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
                *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);
            bAllSucceeded = false;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FINAL RESULT: Connection succeeded for '%s'.'%s' -> '%s'.'%s'"),
                *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.SourcePin,
                *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Connection.TargetPin);
        }
    }

    if (bAllSucceeded)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    return bAllSucceeded;
}

bool FBlueprintNodeConnectionService::ConnectPins(UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!SourceNode || !TargetNode)
    {
        return false;
    }

    // Find the source pin
    UEdGraphPin* SourcePin = nullptr;
    for (UEdGraphPin* Pin : SourceNode->Pins)
    {
        if (Pin && Pin->PinName.ToString() == SourcePinName)
        {
            SourcePin = Pin;
            break;
        }
    }

    // Find the target pin
    UEdGraphPin* TargetPin = nullptr;
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        if (Pin && Pin->PinName.ToString() == TargetPinName)
        {
            TargetPin = Pin;
            break;
        }
    }

    // CRITICAL FIX: Handle Return Node pin naming variations
    if (!TargetPin && TargetNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(TEXT("Return")))
    {
        // Try alternative pin names for Return Node
        TArray<FString> ReturnPinVariations = {
            TEXT("ReturnValue"),
            TEXT("Return Value"),
            TEXT("OutputDelegate"),
            TEXT("Value"),
            TEXT("Result")
        };

        for (const FString& Variation : ReturnPinVariations)
        {
            for (UEdGraphPin* Pin : TargetNode->Pins)
            {
                if (Pin && Pin->PinName.ToString() == Variation)
                {
                    TargetPin = Pin;
                    UE_LOG(LogTemp, Warning, TEXT("Found Return Node pin using variation: %s"), *Variation);
                    break;
                }
            }
            if (TargetPin) break;
        }
    }

    if (!SourcePin || !TargetPin)
    {
        return false;
    }

    // Make the connection
    SourcePin->MakeLinkTo(TargetPin);

    return true;
}

bool FBlueprintNodeConnectionService::ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    UE_LOG(LogTemp, Warning, TEXT("=== ConnectNodesWithAutoCast START ==="));
    UE_LOG(LogTemp, Warning, TEXT("Connecting: '%s'.%s -> '%s'.%s"),
        SourceNode ? *SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"),
        *SourcePinName,
        TargetNode ? *TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("NULL"),
        *TargetPinName);

    if (!Graph || !SourceNode || !TargetNode)
    {
        UE_LOG(LogTemp, Error, TEXT("NULL parameters"));
        return false;
    }

    // Find the pins
    UEdGraphPin* SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);

    if (!SourcePin || !TargetPin)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find pins - Source '%s': %s, Target '%s': %s"),
               *SourcePinName, SourcePin ? TEXT("FOUND") : TEXT("NOT FOUND"),
               *TargetPinName, TargetPin ? TEXT("FOUND") : TEXT("NOT FOUND"));
        return false;
    }

    // LOG DETAILED PIN INFORMATION
    UE_LOG(LogTemp, Warning, TEXT("=== PIN TYPE ANALYSIS ==="));
    UE_LOG(LogTemp, Warning, TEXT("Source Pin '%s': Category=%s, SubCategory=%s, SubCategoryObject=%s"),
        *SourcePin->PinName.ToString(),
        *SourcePin->PinType.PinCategory.ToString(),
        *SourcePin->PinType.PinSubCategory.ToString(),
        SourcePin->PinType.PinSubCategoryObject.IsValid() ? *SourcePin->PinType.PinSubCategoryObject->GetName() : TEXT("None"));
    UE_LOG(LogTemp, Warning, TEXT("Target Pin '%s': Category=%s, SubCategory=%s, SubCategoryObject=%s"),
        *TargetPin->PinName.ToString(),
        *TargetPin->PinType.PinCategory.ToString(),
        *TargetPin->PinType.PinSubCategory.ToString(),
        TargetPin->PinType.PinSubCategoryObject.IsValid() ? *TargetPin->PinType.PinSubCategoryObject->GetName() : TEXT("None"));

    // CHECK IF NODES ARE PROMOTABLE OPERATORS BEFORE CONNECTION
    bool bSourceIsPromotable = SourceNode->GetClass()->GetName().Contains(TEXT("PromotableOperator"));
    bool bTargetIsPromotable = TargetNode->GetClass()->GetName().Contains(TEXT("PromotableOperator"));
    UE_LOG(LogTemp, Warning, TEXT("Node types - Source: %s (Promotable: %s), Target: %s (Promotable: %s)"),
        *SourceNode->GetClass()->GetName(),
        bSourceIsPromotable ? TEXT("YES") : TEXT("NO"),
        *TargetNode->GetClass()->GetName(),
        bTargetIsPromotable ? TEXT("YES") : TEXT("NO"));

    // Check if we need a cast node BEFORE attempting connection
    // For object types, check if target requires more specific type than source provides
    bool bNeedsCast = false;
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object &&
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
    {
        UClass* SourceClass = Cast<UClass>(SourcePin->PinType.PinSubCategoryObject.Get());
        UClass* TargetClass = Cast<UClass>(TargetPin->PinType.PinSubCategoryObject.Get());

        if (SourceClass && TargetClass)
        {
            // Need cast if target is more specific than source (not a parent class)
            // e.g., Source=ActorComponent, Target=SplineComponent -> need cast
            // e.g., Source=AActor, Target=PlayerController -> need cast
            bNeedsCast = !TargetClass->IsChildOf(SourceClass);

            UE_LOG(LogTemp, Warning, TEXT("Object type check: Source=%s, Target=%s, NeedsCast=%s"),
                *SourceClass->GetName(), *TargetClass->GetName(), bNeedsCast ? TEXT("YES") : TEXT("NO"));
        }
    }

    // If we need a cast, create it immediately
    if (bNeedsCast)
    {
        UE_LOG(LogTemp, Warning, TEXT("Creating cast node before connection attempt..."));
        bool bCastCreated = CreateCastNode(Graph, SourcePin, TargetPin);

        if (bCastCreated)
        {
            UE_LOG(LogTemp, Display, TEXT("Auto-cast successful - created conversion node"));
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Auto-cast failed - no conversion available"));
            // Continue with direct connection attempt as fallback
        }
    }

    // Just try to connect - let Unreal handle all the validation and type conversion
    UE_LOG(LogTemp, Warning, TEXT("Attempting connection..."));

    // For execution pins, break existing connections first to avoid multiple outputs
    // Execution pins should only have ONE outgoing connection
    if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        if (SourcePin->LinkedTo.Num() > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Breaking %d existing exec connections from source pin"), SourcePin->LinkedTo.Num());
            SourcePin->BreakAllPinLinks();
        }
    }

    SourcePin->MakeLinkTo(TargetPin);

    // CRITICAL: Explicitly notify nodes about pin connection changes for wildcard adaptation
    // This ensures PromotableOperators properly evaluate their pins and convert wildcards to specific types
    SourceNode->PinConnectionListChanged(SourcePin);
    TargetNode->PinConnectionListChanged(TargetPin);
    UE_LOG(LogTemp, Warning, TEXT("Notified nodes of pin connection changes"));

    // Check if connection was made
    bool bConnectionExists = SourcePin->LinkedTo.Contains(TargetPin);
    UE_LOG(LogTemp, Warning, TEXT("Connection result: %s"), bConnectionExists ? TEXT("SUCCESS") : TEXT("FAILED"));

    // If direct connection failed and we didn't try cast yet, try now
    if (!bConnectionExists && !bNeedsCast)
    {
        UE_LOG(LogTemp, Warning, TEXT("Direct connection failed - attempting auto-cast..."));

        // Check if this is a case where we can insert a cast node
        bool bCastCreated = CreateCastNode(Graph, SourcePin, TargetPin);

        if (bCastCreated)
        {
            UE_LOG(LogTemp, Display, TEXT("Auto-cast successful - created conversion node"));
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Auto-cast failed - no conversion available"));
            return false;
        }
    }

    if (bConnectionExists)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== POST-CONNECTION PIN TYPE ANALYSIS ==="));
        UE_LOG(LogTemp, Warning, TEXT("AFTER Connection - Source Pin '%s': Category=%s, SubCategory=%s"),
            *SourcePin->PinName.ToString(),
            *SourcePin->PinType.PinCategory.ToString(),
            *SourcePin->PinType.PinSubCategory.ToString());
        UE_LOG(LogTemp, Warning, TEXT("AFTER Connection - Target Pin '%s': Category=%s, SubCategory=%s"),
            *TargetPin->PinName.ToString(),
            *TargetPin->PinType.PinCategory.ToString(),
            *TargetPin->PinType.PinSubCategory.ToString());

        // Force wildcard pin type resolution for mathematical operators
        if (UK2Node_PromotableOperator* PromotableOpSource = Cast<UK2Node_PromotableOperator>(SourceNode))
        {
            UE_LOG(LogTemp, Warning, TEXT("=== PROCESSING SOURCE PROMOTABLE OPERATOR ==="));
            UE_LOG(LogTemp, Warning, TEXT("Source PromotableOperator before fixes:"));
            for (UEdGraphPin* Pin : PromotableOpSource->Pins)
            {
                UE_LOG(LogTemp, Warning, TEXT("  Pin '%s': %s.%s"),
                    *Pin->PinName.ToString(),
                    *Pin->PinType.PinCategory.ToString(),
                    *Pin->PinType.PinSubCategory.ToString());
            }

            // Apply comprehensive PromotableOperator fix (from Habr tutorial)
            if (UEdGraph* Graph = PromotableOpSource->GetGraph())
            {
                // Force visualization cache clear and notify graph changed
                if (const UEdGraphSchema* Schema = Graph->GetSchema())
                {
                    Schema->ForceVisualizationCacheClear();
                }
                Graph->NotifyGraphChanged();
            }

            // DON'T ReconstructNode() - it breaks wildcard pins by converting them to specific types!
            // PromotableOpSource->ReconstructNode();

            UE_LOG(LogTemp, Warning, TEXT("Source PromotableOperator AFTER fixes:"));
            for (UEdGraphPin* Pin : PromotableOpSource->Pins)
            {
                UE_LOG(LogTemp, Warning, TEXT("  Pin '%s': %s.%s"),
                    *Pin->PinName.ToString(),
                    *Pin->PinType.PinCategory.ToString(),
                    *Pin->PinType.PinSubCategory.ToString());
            }
            UE_LOG(LogTemp, Warning, TEXT("Applied comprehensive PromotableOperator fix to source node"));
        }

        if (UK2Node_PromotableOperator* PromotableOpTarget = Cast<UK2Node_PromotableOperator>(TargetNode))
        {
            UE_LOG(LogTemp, Warning, TEXT("=== PROCESSING TARGET PROMOTABLE OPERATOR ==="));
            UE_LOG(LogTemp, Warning, TEXT("Target PromotableOperator before fixes:"));
            for (UEdGraphPin* Pin : PromotableOpTarget->Pins)
            {
                UE_LOG(LogTemp, Warning, TEXT("  Pin '%s': %s.%s"),
                    *Pin->PinName.ToString(),
                    *Pin->PinType.PinCategory.ToString(),
                    *Pin->PinType.PinSubCategory.ToString());
            }

            // Apply comprehensive PromotableOperator fix (from Habr tutorial)
            if (UEdGraph* Graph = PromotableOpTarget->GetGraph())
            {
                // Force visualization cache clear and notify graph changed
                if (const UEdGraphSchema* Schema = Graph->GetSchema())
                {
                    Schema->ForceVisualizationCacheClear();
                }
                Graph->NotifyGraphChanged();
            }

            // DON'T ReconstructNode() - it breaks wildcard pins by converting them to specific types!
            // PromotableOpTarget->ReconstructNode();

            UE_LOG(LogTemp, Warning, TEXT("Target PromotableOperator AFTER fixes:"));
            for (UEdGraphPin* Pin : PromotableOpTarget->Pins)
            {
                UE_LOG(LogTemp, Warning, TEXT("  Pin '%s': %s.%s"),
                    *Pin->PinName.ToString(),
                    *Pin->PinType.PinCategory.ToString(),
                    *Pin->PinType.PinSubCategory.ToString());
            }
            UE_LOG(LogTemp, Warning, TEXT("Applied comprehensive PromotableOperator fix to target node"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Connection rejected by Unreal Engine - incompatible pin types or other validation failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("=== ConnectNodesWithAutoCast END ==="));
    return bConnectionExists;
}

bool FBlueprintNodeConnectionService::ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType)
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

bool FBlueprintNodeConnectionService::CreateCastNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
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

bool FBlueprintNodeConnectionService::CreateIntToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_IntToString function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateIntToStringCast: Could not find KismetStringLibrary"));
        return false;
    }

    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_IntToString"));
    if (!ConvFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateIntToStringCast: Could not find Conv_IntToString function"));
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
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InInt"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);

    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateIntToStringCast: Successfully created Int to String cast node"));
    return true;
}

bool FBlueprintNodeConnectionService::CreateFloatToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_FloatToString function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }

    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_FloatToString"));
    if (!ConvFunction)
    {
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
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InFloat"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);

    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateFloatToStringCast: Successfully created Float to String cast node"));
    return true;
}

bool FBlueprintNodeConnectionService::CreateBoolToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_BoolToString function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }

    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_BoolToString"));
    if (!ConvFunction)
    {
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
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InBool"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);

    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateBoolToStringCast: Successfully created Bool to String cast node"));
    return true;
}

bool FBlueprintNodeConnectionService::CreateStringToIntCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_StringToInt function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }

    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_StringToInt"));
    if (!ConvFunction)
    {
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
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InString"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);

    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateStringToIntCast: Successfully created String to Int cast node"));
    return true;
}

bool FBlueprintNodeConnectionService::CreateStringToFloatCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
    // Find the Conv_StringToFloat function
    UClass* KismetStringLibrary = FindObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
    if (!KismetStringLibrary)
    {
        return false;
    }

    UFunction* ConvFunction = KismetStringLibrary->FindFunctionByName(TEXT("Conv_StringToFloat"));
    if (!ConvFunction)
    {
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
    UEdGraphPin* ConvInputPin = ConvNode->FindPinChecked(TEXT("InString"), EGPD_Input);
    UEdGraphPin* ConvOutputPin = ConvNode->FindPinChecked(TEXT("ReturnValue"), EGPD_Output);

    // Connect: Source -> Conv Input -> Conv Output -> Target
    SourcePin->MakeLinkTo(ConvInputPin);
    ConvOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateStringToFloatCast: Successfully created String to Float cast node"));
    return true;
}

bool FBlueprintNodeConnectionService::CreateObjectCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
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
    // Cast nodes have "Object" input pin and "As [ClassName]" output pin
    UEdGraphPin* CastInputPin = nullptr;
    UEdGraphPin* CastOutputPin = nullptr;

    for (UEdGraphPin* Pin : CastNode->Pins)
    {
        if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
        {
            CastInputPin = Pin;
        }
        else if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object &&
                 Pin->PinName.ToString().StartsWith(TEXT("As")))
        {
            CastOutputPin = Pin;
        }
    }

    if (!CastInputPin || !CastOutputPin)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateObjectCast: Could not find input/output pins on cast node"));
        Graph->RemoveNode(CastNode);
        return false;
    }

    // Connect: Source -> Cast Input -> Cast Output -> Target
    SourcePin->MakeLinkTo(CastInputPin);
    CastOutputPin->MakeLinkTo(TargetPin);

    UE_LOG(LogTemp, Display, TEXT("CreateObjectCast: Successfully created object cast node to %s"), *TargetClass->GetName());
    return true;
}

UEdGraphNode* FBlueprintNodeConnectionService::FindNodeByIdOrType(UEdGraph* Graph, const FString& NodeIdOrType)
{
    if (!Graph)
    {
        return nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("FindNodeByIdOrType: Looking for '%s' in graph '%s' with %d nodes"),
        *NodeIdOrType, *Graph->GetFName().ToString(), Graph->Nodes.Num());

    // First try to find by exact GUID or safe node ID
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            // Check both raw GUID and safe node ID
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            FString SafeNodeId = GetSafeNodeId(Node, NodeTitle);

            if (Node->NodeGuid.ToString() == NodeIdOrType || SafeNodeId == NodeIdOrType)
            {
                return Node;
            }
        }
    }

    // If not found by GUID, try to find by node title (for Entry/Exit nodes)
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();

            // Handle special cases for Entry/Exit nodes
            if (NodeIdOrType == TEXT("FunctionEntry") || NodeIdOrType == TEXT("CanInteract"))
            {
                // Look for function entry nodes
                if (NodeTitle.Contains(TEXT("CanInteract")) && !NodeTitle.Contains(TEXT("Return")))
                {
                    return Node;
                }
            }
            else if (NodeIdOrType == TEXT("FunctionResult") || NodeIdOrType == TEXT("Return Node"))
            {
                // Look for function result nodes
                if (NodeTitle.Contains(TEXT("Return")) && NodeTitle.Contains(TEXT("Node")))
                {
                    return Node;
                }
            }
            else if (NodeTitle == NodeIdOrType)
            {
                // Direct title match
                return Node;
            }
        }
    }

    return nullptr;
}
