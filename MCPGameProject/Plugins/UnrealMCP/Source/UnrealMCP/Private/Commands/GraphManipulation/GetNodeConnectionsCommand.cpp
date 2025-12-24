#include "Commands/GraphManipulation/GetNodeConnectionsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Utils/GraphUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"

FString FGetNodeConnectionsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetNodeConnectionsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Parse parameters
    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: blueprint_name"));
    }

    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: node_id"));
    }

    FString TargetGraph = TEXT("EventGraph");
    JsonObject->TryGetStringField(TEXT("target_graph"), TargetGraph);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the graph
    UEdGraph* Graph = nullptr;
    for (UEdGraph* CurrentGraph : Blueprint->UbergraphPages)
    {
        if (CurrentGraph && CurrentGraph->GetName() == TargetGraph)
        {
            Graph = CurrentGraph;
            break;
        }
    }

    if (!Graph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *TargetGraph));
    }

    // Find the node
    UEdGraphNode* Node = nullptr;
    for (UEdGraphNode* CurrentNode : Graph->Nodes)
    {
        if (CurrentNode && FGraphUtils::GetReliableNodeId(CurrentNode) == NodeId)
        {
            Node = CurrentNode;
            break;
        }
    }

    if (!Node)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Node not found with ID: %s"), *NodeId));
    }

    // Collect pin information
    TArray<TSharedPtr<FJsonValue>> PinsArray;
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject());
        PinObj->SetStringField(TEXT("name"), Pin->GetName());
        PinObj->SetStringField(TEXT("display_name"), Pin->GetDisplayName().ToString());
        PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
        PinObj->SetStringField(TEXT("sub_category"), Pin->PinType.PinSubCategory.ToString());
        PinObj->SetBoolField(TEXT("is_array"), Pin->PinType.IsArray());
        PinObj->SetBoolField(TEXT("is_reference"), Pin->PinType.bIsReference);
        PinObj->SetBoolField(TEXT("is_input"), Pin->Direction == EGPD_Input);
        PinObj->SetBoolField(TEXT("is_output"), Pin->Direction == EGPD_Output);
        PinObj->SetBoolField(TEXT("is_hidden"), Pin->bHidden);
        
        // Check if it's an execution pin
        const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
        bool bIsExecPin = K2Schema && K2Schema->IsExecPin(*Pin);
        PinObj->SetBoolField(TEXT("is_execution"), bIsExecPin);
        
        // Default value if any
        if (!Pin->DefaultValue.IsEmpty())
        {
            PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
        }
        
        // Connection information
        TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            if (LinkedPin && LinkedPin->GetOwningNode())
            {
                TSharedPtr<FJsonObject> ConnObj = MakeShareable(new FJsonObject());
                ConnObj->SetStringField(TEXT("connected_node_id"), FGraphUtils::GetReliableNodeId(LinkedPin->GetOwningNode()));
                ConnObj->SetStringField(TEXT("connected_node_title"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString());
                ConnObj->SetStringField(TEXT("connected_pin_name"), LinkedPin->GetName());
                ConnObj->SetStringField(TEXT("connected_pin_display_name"), LinkedPin->GetDisplayName().ToString());
                ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnObj)));
            }
        }
        PinObj->SetArrayField(TEXT("connections"), ConnectionsArray);
        PinObj->SetNumberField(TEXT("connection_count"), Pin->LinkedTo.Num());
        
        PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
    }

    // Create response
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    ResponseObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
    ResponseObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
    ResponseObj->SetStringField(TEXT("target_graph"), TargetGraph);
    ResponseObj->SetArrayField(TEXT("pins"), PinsArray);
    ResponseObj->SetNumberField(TEXT("pin_count"), Node->Pins.Num());
    ResponseObj->SetNumberField(TEXT("node_pos_x"), Node->NodePosX);
    ResponseObj->SetNumberField(TEXT("node_pos_y"), Node->NodePosY);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
