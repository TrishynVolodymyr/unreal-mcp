#include "Commands/GraphManipulation/AutoArrangeNodesCommand.h"
#include "Services/NodeLayout/NodeLayoutService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"

FString FAutoArrangeNodesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Parse blueprint_name (required)
    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: blueprint_name"));
    }

    // Parse graph_name (optional, default: EventGraph)
    FString GraphName = TEXT("EventGraph");
    JsonObject->TryGetStringField(TEXT("graph_name"), GraphName);

    // Find the Blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the graph
    UEdGraph* TargetGraph = nullptr;

    // Search in UbergraphPages (EventGraph, etc.)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetName() == GraphName)
        {
            TargetGraph = Graph;
            break;
        }
    }

    // Search in FunctionGraphs if not found
    if (!TargetGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetName() == GraphName)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }

    if (!TargetGraph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s in Blueprint %s"), *GraphName, *BlueprintName));
    }

    // Auto-arrange nodes
    int32 ArrangedCount = 0;
    if (!FNodeLayoutService::AutoArrangeNodes(TargetGraph, ArrangedCount))
    {
        return CreateErrorResponse(TEXT("Failed to auto-arrange nodes"));
    }

    return CreateSuccessResponse(BlueprintName, GraphName, ArrangedCount);
}

bool FAutoArrangeNodesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString BlueprintName;
    return JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
}

FString FAutoArrangeNodesCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& GraphName, int32 ArrangedCount) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("graph_name"), GraphName);
    ResponseObj->SetNumberField(TEXT("arranged_count"), ArrangedCount);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully arranged %d nodes"), ArrangedCount));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FAutoArrangeNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
