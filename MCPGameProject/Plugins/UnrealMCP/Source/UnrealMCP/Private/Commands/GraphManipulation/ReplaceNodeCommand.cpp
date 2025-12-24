#include "Commands/GraphManipulation/ReplaceNodeCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Utils/GraphUtils.h"
#include "Services/BlueprintNodeCreationService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node.h"
#include "Kismet2/BlueprintEditorUtils.h"

FString FReplaceNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FReplaceNodeCommand::Execute(const FString& Parameters)
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

    FString OldNodeId;
    if (!JsonObject->TryGetStringField(TEXT("old_node_id"), OldNodeId))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: old_node_id"));
    }

    FString NewNodeType;
    if (!JsonObject->TryGetStringField(TEXT("new_node_type"), NewNodeType))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: new_node_type"));
    }

    FString TargetGraph = TEXT("EventGraph");
    JsonObject->TryGetStringField(TEXT("target_graph"), TargetGraph);

    // Optional: new node configuration
    TSharedPtr<FJsonObject> NewNodeConfig;
    const TSharedPtr<FJsonObject>* ConfigPtr;
    if (JsonObject->TryGetObjectField(TEXT("new_node_config"), ConfigPtr))
    {
        NewNodeConfig = *ConfigPtr;
    }

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

    // Also check function graphs
    if (!Graph)
    {
        for (UEdGraph* CurrentGraph : Blueprint->FunctionGraphs)
        {
            if (CurrentGraph && CurrentGraph->GetName() == TargetGraph)
            {
                Graph = CurrentGraph;
                break;
            }
        }
    }

    if (!Graph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *TargetGraph));
    }

    // Find the old node
    UEdGraphNode* OldNode = nullptr;
    for (UEdGraphNode* CurrentNode : Graph->Nodes)
    {
        if (CurrentNode && FGraphUtils::GetReliableNodeId(CurrentNode) == OldNodeId)
        {
            OldNode = CurrentNode;
            break;
        }
    }

    if (!OldNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Old node not found with ID: %s"), *OldNodeId));
    }

    // Step 1: Store all connections from the old node
    TArray<FPinConnection> StoredConnections;
    FVector2D OldNodePosition(OldNode->NodePosX, OldNode->NodePosY);
    
    for (UEdGraphPin* Pin : OldNode->Pins)
    {
        if (!Pin || Pin->LinkedTo.Num() == 0)
        {
            continue;
        }

        FPinConnection Connection;
        Connection.PinName = Pin->GetName();
        Connection.bIsInput = (Pin->Direction == EGPD_Input);

        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            if (LinkedPin && LinkedPin->GetOwningNode())
            {
                Connection.ConnectedNodeIds.Add(FGraphUtils::GetReliableNodeId(LinkedPin->GetOwningNode()));
                Connection.ConnectedPinNames.Add(LinkedPin->GetName());
            }
        }

        if (Connection.ConnectedNodeIds.Num() > 0)
        {
            StoredConnections.Add(Connection);
        }
    }

    // Step 2: Disconnect and delete old node
    for (UEdGraphPin* Pin : OldNode->Pins)
    {
        if (Pin)
        {
            Pin->BreakAllPinLinks();
        }
    }

    FString OldNodeTitle = OldNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    Graph->RemoveNode(OldNode);

    // Step 3: Create new node at the same position
    FBlueprintNodeCreationService NodeCreationService;
    
    // Build node position string
    FString NodePosStr = FString::Printf(TEXT("[%d, %d]"), (int32)OldNodePosition.X, (int32)OldNodePosition.Y);
    
    // Build JSON params with target_graph
    TSharedPtr<FJsonObject> ParamsObj = MakeShareable(new FJsonObject());
    ParamsObj->SetStringField(TEXT("target_graph"), TargetGraph);
    
    // Add any additional config from NewNodeConfig
    if (NewNodeConfig.IsValid())
    {
        for (auto& Pair : NewNodeConfig->Values)
        {
            ParamsObj->SetField(Pair.Key, Pair.Value);
        }
    }
    
    FString ParamsJsonStr;
    TSharedRef<TJsonWriter<>> ParamsWriter = TJsonWriterFactory<>::Create(&ParamsJsonStr);
    FJsonSerializer::Serialize(ParamsObj.ToSharedRef(), ParamsWriter);
    
    // Create the new node
    FString CreateNodeResult = NodeCreationService.CreateNodeByActionName(
        BlueprintName,
        NewNodeType,
        TEXT(""), // ClassName - empty to let it auto-discover
        NodePosStr,
        ParamsJsonStr
    );
    
    // Parse the result to get new node ID
    TSharedPtr<FJsonObject> CreateNodeResultObj;
    TSharedRef<TJsonReader<>> CreateNodeReader = TJsonReaderFactory<>::Create(CreateNodeResult);
    if (!FJsonSerializer::Deserialize(CreateNodeReader, CreateNodeResultObj) || !CreateNodeResultObj.IsValid())
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to parse create node result: %s"), *CreateNodeResult));
    }
    
    bool bCreateSuccess = false;
    CreateNodeResultObj->TryGetBoolField(TEXT("success"), bCreateSuccess);
    if (!bCreateSuccess)
    {
        FString ErrorMsg;
        CreateNodeResultObj->TryGetStringField(TEXT("message"), ErrorMsg);
        return CreateErrorResponse(FString::Printf(TEXT("Failed to create new node: %s"), *ErrorMsg));
    }
    
    FString NewNodeId;
    CreateNodeResultObj->TryGetStringField(TEXT("node_id"), NewNodeId);
    if (NewNodeId.IsEmpty())
    {
        return CreateErrorResponse(TEXT("New node was created but node_id was not returned"));
    }
    
    // Find the new node
    UEdGraphNode* NewNode = nullptr;
    for (UEdGraphNode* CurrentNode : Graph->Nodes)
    {
        if (CurrentNode && FGraphUtils::GetReliableNodeId(CurrentNode) == NewNodeId)
        {
            NewNode = CurrentNode;
            break;
        }
    }
    
    if (!NewNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Could not find newly created node with ID: %s"), *NewNodeId));
    }
    
    // Step 4: Restore connections
    int32 RestoredConnections = 0;
    for (const FPinConnection& Connection : StoredConnections)
    {
        // Find the pin on the new node
        // First try exact name match
        UEdGraphPin* NewNodePin = nullptr;
        for (UEdGraphPin* Pin : NewNode->Pins)
        {
            if (Pin && Pin->GetName() == Connection.PinName && 
                (Pin->Direction == EGPD_Input) == Connection.bIsInput)
            {
                NewNodePin = Pin;
                break;
            }
        }
        
        // If exact name not found, try to find first pin with same direction
        // This handles cases like replacing one variable getter with another
        if (!NewNodePin)
        {
            for (UEdGraphPin* Pin : NewNode->Pins)
            {
                if (Pin && (Pin->Direction == EGPD_Input) == Connection.bIsInput)
                {
                    NewNodePin = Pin;
                    UE_LOG(LogTemp, Warning, TEXT("Pin '%s' not found by name, using compatible pin '%s' instead"), 
                        *Connection.PinName, *Pin->GetName());
                    break;
                }
            }
        }
        
        if (!NewNodePin)
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find compatible pin for '%s' on new node"), *Connection.PinName);
            continue;
        }
        
        // Reconnect to all previously connected nodes
        for (int32 i = 0; i < Connection.ConnectedNodeIds.Num(); i++)
        {
            const FString& ConnectedNodeId = Connection.ConnectedNodeIds[i];
            const FString& ConnectedPinName = Connection.ConnectedPinNames[i];
            
            // Find the connected node
            UEdGraphNode* ConnectedNode = nullptr;
            for (UEdGraphNode* CurrentNode : Graph->Nodes)
            {
                if (CurrentNode && FGraphUtils::GetReliableNodeId(CurrentNode) == ConnectedNodeId)
                {
                    ConnectedNode = CurrentNode;
                    break;
                }
            }
            
            if (!ConnectedNode)
            {
                UE_LOG(LogTemp, Warning, TEXT("Could not find connected node with ID: %s"), *ConnectedNodeId);
                continue;
            }
            
            // Find the connected pin
            UEdGraphPin* ConnectedPin = nullptr;
            for (UEdGraphPin* Pin : ConnectedNode->Pins)
            {
                if (Pin && Pin->GetName() == ConnectedPinName)
                {
                    ConnectedPin = Pin;
                    break;
                }
            }
            
            if (!ConnectedPin)
            {
                UE_LOG(LogTemp, Warning, TEXT("Could not find pin '%s' on connected node"), *ConnectedPinName);
                continue;
            }
            
            // Make the connection using auto-cast logic
            bool bConnected = false;
            if (Connection.bIsInput)
            {
                // NewNodePin is input, ConnectedPin is output
                // Source: ConnectedNode/ConnectedPin -> Target: NewNode/NewNodePin
                bConnected = Service.ConnectNodesWithAutoCast(
                    Graph, 
                    ConnectedNode, 
                    ConnectedPinName, 
                    NewNode, 
                    NewNodePin->GetName()
                );
            }
            else
            {
                // NewNodePin is output, ConnectedPin is input
                // Source: NewNode/NewNodePin -> Target: ConnectedNode/ConnectedPin
                bConnected = Service.ConnectNodesWithAutoCast(
                    Graph, 
                    NewNode, 
                    NewNodePin->GetName(), 
                    ConnectedNode, 
                    ConnectedPinName
                );
            }
            
            if (bConnected)
            {
                RestoredConnections++;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to connect pins '%s' -> '%s'"), 
                    Connection.bIsInput ? *ConnectedPinName : *NewNodePin->GetName(),
                    Connection.bIsInput ? *NewNodePin->GetName() : *ConnectedPinName);
            }
        }
    }

    // Mark as modified
    Blueprint->Modify();
    Graph->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("old_node_id"), OldNodeId);
    ResponseObj->SetStringField(TEXT("old_node_title"), OldNodeTitle);
    ResponseObj->SetStringField(TEXT("new_node_id"), NewNodeId);
    ResponseObj->SetStringField(TEXT("new_node_type"), NewNodeType);
    ResponseObj->SetStringField(TEXT("target_graph"), TargetGraph);
    ResponseObj->SetNumberField(TEXT("position_x"), OldNodePosition.X);
    ResponseObj->SetNumberField(TEXT("position_y"), OldNodePosition.Y);
    ResponseObj->SetNumberField(TEXT("restored_connections"), RestoredConnections);
    ResponseObj->SetStringField(TEXT("message"), 
        FString::Printf(TEXT("Successfully replaced node '%s' with '%s'. Restored %d connections."), 
            *OldNodeTitle, *NewNodeType, RestoredConnections));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
