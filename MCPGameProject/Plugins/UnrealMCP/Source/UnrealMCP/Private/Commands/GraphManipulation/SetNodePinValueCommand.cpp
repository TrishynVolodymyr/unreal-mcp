#include "Commands/GraphManipulation/SetNodePinValueCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"

FString FSetNodePinValueCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetNodePinValueCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Parse required parameters
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

    FString PinName;
    if (!JsonObject->TryGetStringField(TEXT("pin_name"), PinName))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: pin_name"));
    }

    FString Value;
    if (!JsonObject->TryGetStringField(TEXT("value"), Value))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: value"));
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
        return CreateErrorResponse(FString::Printf(TEXT("Graph '%s' not found in blueprint '%s'"), *TargetGraph, *BlueprintName));
    }

    // Find the node
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            FString CurrentNodeId = Node->NodeGuid.ToString();
            if (CurrentNodeId == NodeId)
            {
                TargetNode = Node;
                break;
            }
        }
    }

    if (!TargetNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Node not found with ID: %s"), *NodeId));
    }

    // Find the pin
    UEdGraphPin* TargetPin = nullptr;
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        if (Pin && Pin->GetName() == PinName)
        {
            TargetPin = Pin;
            break;
        }
    }

    if (!TargetPin)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Pin '%s' not found on node"), *PinName));
    }

    // Set the pin value based on its type
    const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
    if (!K2Schema)
    {
        return CreateErrorResponse(TEXT("Graph schema is not K2 (Blueprint) schema"));
    }

    // Handle different pin types
    if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
    {
        // Handle class reference pins
        UClass* ClassToSet = nullptr;
        
        // Try to find the class by name
        if (Value.StartsWith(TEXT("/Script/")))
        {
            // Full path provided
            ClassToSet = FindObject<UClass>(nullptr, *Value);
        }
        else
        {
            // Short name provided, try common engine classes
            FString FullPath = FString::Printf(TEXT("/Script/Engine.%s"), *Value);
            ClassToSet = FindObject<UClass>(nullptr, *FullPath);
            
            // If not found, try without Engine prefix
            if (!ClassToSet)
            {
                ClassToSet = FindFirstObject<UClass>(*Value, EFindFirstObjectOptions::NativeFirst);
            }
        }

        if (ClassToSet)
        {
            TargetPin->DefaultObject = ClassToSet;
            TargetPin->DefaultValue = ClassToSet->GetPathName();
        }
        else
        {
            return CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *Value));
        }
    }
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && TargetPin->PinType.PinSubCategoryObject.IsValid())
    {
        // Handle enum pins
        UEnum* EnumType = Cast<UEnum>(TargetPin->PinType.PinSubCategoryObject.Get());
        if (EnumType)
        {
            // Try to find the enum value by name
            int64 EnumValue = INDEX_NONE;
            FString EnumDisplayName;
            
            // First try exact match with full name (e.g., "ESplineCoordinateSpace::World")
            EnumValue = EnumType->GetValueByNameString(Value);
            
            if (EnumValue == INDEX_NONE)
            {
                // Try to find by short name (e.g., "World")
                for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to skip MAX value
                {
                    FString EnumName = EnumType->GetNameStringByIndex(i);
                    FString ShortName = EnumName;
                    
                    // Remove enum prefix (e.g., "ESplineCoordinateSpace::World" -> "World")
                    int32 ColonPos;
                    if (EnumName.FindLastChar(':', ColonPos))
                    {
                        ShortName = EnumName.RightChop(ColonPos + 1);
                    }
                    
                    if (ShortName.Equals(Value, ESearchCase::IgnoreCase) || EnumName.Equals(Value, ESearchCase::IgnoreCase))
                    {
                        EnumValue = i;
                        EnumDisplayName = EnumName;
                        break;
                    }
                }
            }
            else
            {
                // Get the display name for the found value
                EnumDisplayName = EnumType->GetNameStringByValue(EnumValue);
            }
            
            if (EnumValue != INDEX_NONE)
            {
                // Set the default value to the full enum name (not the index)
                // This makes it display properly in the Blueprint editor
                if (EnumDisplayName.IsEmpty())
                {
                    EnumDisplayName = EnumType->GetNameStringByValue(EnumValue);
                }
                TargetPin->DefaultValue = EnumDisplayName;
            }
            else
            {
                return CreateErrorResponse(FString::Printf(TEXT("Enum value '%s' not found in enum '%s'"), *Value, *EnumType->GetName()));
            }
        }
    }
    else
    {
        // For other types (int, float, bool, string), just set the default value directly
        TargetPin->DefaultValue = Value;
    }

    // Reconstruct the node to apply changes
    TargetNode->ReconstructNode();

    // Mark blueprint as modified
    Blueprint->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    ResponseObj->SetStringField(TEXT("pin_name"), PinName);
    ResponseObj->SetStringField(TEXT("value"), Value);
    ResponseObj->SetStringField(TEXT("pin_type"), TargetPin->PinType.PinCategory.ToString());
    ResponseObj->SetStringField(TEXT("message"), TEXT("Pin value set successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
