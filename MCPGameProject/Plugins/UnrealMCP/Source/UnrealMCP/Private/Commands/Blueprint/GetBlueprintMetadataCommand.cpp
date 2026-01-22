#include "Commands/Blueprint/GetBlueprintMetadataCommand.h"
#include "Engine/Blueprint.h"
#include "Services/AssetDiscoveryService.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetBlueprintMetadataCommand::FGetBlueprintMetadataCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
    , MetadataBuilder(InBlueprintService)
{
}

FString FGetBlueprintMetadataCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    TArray<FString> Fields;
    FGraphNodesFilter Filter;
    FString ParseError;

    if (!ParseParameters(Parameters, BlueprintName, Fields, Filter, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    UBlueprint* Blueprint = FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }

    TSharedPtr<FJsonObject> Metadata = BuildMetadata(Blueprint, Fields, Filter);
    return CreateSuccessResponse(Metadata);
}

FString FGetBlueprintMetadataCommand::GetCommandName() const
{
    return TEXT("get_blueprint_metadata");
}

bool FGetBlueprintMetadataCommand::ValidateParams(const FString& Parameters) const
{
    // Only validate basic JSON structure - detailed validation happens in Execute
    // so we can return meaningful error messages
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // Just check blueprint_name exists
    FString BlueprintName;
    return JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
}

bool FGetBlueprintMetadataCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FString>& OutFields, FGraphNodesFilter& OutFilter, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }

    // Parse required fields array - at least one field must be specified
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
    if (JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray) && FieldsArray->Num() > 0)
    {
        for (const TSharedPtr<FJsonValue>& Value : *FieldsArray)
        {
            OutFields.Add(Value->AsString());
        }
    }
    else
    {
        OutError = TEXT("Missing required 'fields' parameter. Specify at least one field (e.g., [\"components\"], [\"variables\"], [\"graph_nodes\"])");
        return false;
    }

    // Parse optional filters for graph_nodes field
    JsonObject->TryGetStringField(TEXT("graph_name"), OutFilter.GraphName);
    JsonObject->TryGetStringField(TEXT("node_type"), OutFilter.NodeType);
    JsonObject->TryGetStringField(TEXT("event_type"), OutFilter.EventType);
    JsonObject->TryGetStringField(TEXT("component_name"), OutFilter.ComponentName);

    // Parse detail_level (default: flow)
    FString DetailLevelStr;
    if (JsonObject->TryGetStringField(TEXT("detail_level"), DetailLevelStr))
    {
        DetailLevelStr = DetailLevelStr.ToLower();
        if (DetailLevelStr == TEXT("summary"))
        {
            OutFilter.DetailLevel = EGraphNodesDetailLevel::Summary;
        }
        else if (DetailLevelStr == TEXT("full"))
        {
            OutFilter.DetailLevel = EGraphNodesDetailLevel::Full;
        }
        else
        {
            // Default to Flow for any unrecognized value or "flow"
            OutFilter.DetailLevel = EGraphNodesDetailLevel::Flow;
        }
    }
    // else keep default (Flow)

    // Validate: graph_nodes requires graph_name to be specified
    if (OutFields.Contains(TEXT("graph_nodes")) && OutFilter.GraphName.IsEmpty())
    {
        OutError = TEXT("When requesting 'graph_nodes' field, 'graph_name' parameter is required to limit response size");
        return false;
    }

    // Validate: component_properties requires component_name to be specified
    if (OutFields.Contains(TEXT("component_properties")) && OutFilter.ComponentName.IsEmpty())
    {
        OutError = TEXT("When requesting 'component_properties' field, 'component_name' parameter is required");
        return false;
    }

    return true;
}

UBlueprint* FGetBlueprintMetadataCommand::FindBlueprint(const FString& BlueprintName) const
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Handle full path
    if (BlueprintName.StartsWith(TEXT("/Game/")) || BlueprintName.StartsWith(TEXT("/Script/")))
    {
        FString ObjectPath = BlueprintName;

        // If path doesn't contain '.', append the asset name (e.g., /Game/Foo/Bar -> /Game/Foo/Bar.Bar)
        if (!ObjectPath.Contains(TEXT(".")))
        {
            FString AssetName = FPaths::GetBaseFilename(ObjectPath);
            ObjectPath = ObjectPath + TEXT(".") + AssetName;
        }

        FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
        if (AssetData.IsValid())
        {
            return Cast<UBlueprint>(AssetData.GetAsset());
        }
    }

    // Handle simple name using FAssetDiscoveryService
    TArray<FString> FoundBlueprints = FAssetDiscoveryService::Get().FindBlueprints(BlueprintName);
    if (FoundBlueprints.Num() > 0)
    {
        return LoadObject<UBlueprint>(nullptr, *FoundBlueprints[0]);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildMetadata(UBlueprint* Blueprint, const TArray<FString>& Fields, const FGraphNodesFilter& Filter) const
{
    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();

    if (ShouldIncludeField(TEXT("parent_class"), Fields))
    {
        Metadata->SetObjectField(TEXT("parent_class"), MetadataBuilder.BuildParentClassInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("interfaces"), Fields))
    {
        Metadata->SetObjectField(TEXT("interfaces"), MetadataBuilder.BuildInterfacesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("variables"), Fields))
    {
        Metadata->SetObjectField(TEXT("variables"), MetadataBuilder.BuildVariablesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("functions"), Fields))
    {
        Metadata->SetObjectField(TEXT("functions"), MetadataBuilder.BuildFunctionsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("components"), Fields))
    {
        Metadata->SetObjectField(TEXT("components"), MetadataBuilder.BuildComponentsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("component_properties"), Fields))
    {
        Metadata->SetObjectField(TEXT("component_properties"), MetadataBuilder.BuildComponentPropertiesInfo(Blueprint, Filter.ComponentName));
    }
    if (ShouldIncludeField(TEXT("graphs"), Fields))
    {
        Metadata->SetObjectField(TEXT("graphs"), MetadataBuilder.BuildGraphsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("status"), Fields))
    {
        Metadata->SetObjectField(TEXT("status"), MetadataBuilder.BuildStatusInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("metadata"), Fields))
    {
        Metadata->SetObjectField(TEXT("metadata"), MetadataBuilder.BuildMetadataInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("timelines"), Fields))
    {
        Metadata->SetObjectField(TEXT("timelines"), MetadataBuilder.BuildTimelinesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("asset_info"), Fields))
    {
        Metadata->SetObjectField(TEXT("asset_info"), MetadataBuilder.BuildAssetInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("orphaned_nodes"), Fields))
    {
        Metadata->SetObjectField(TEXT("orphaned_nodes"), MetadataBuilder.BuildOrphanedNodesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("graph_warnings"), Fields))
    {
        Metadata->SetObjectField(TEXT("graph_warnings"), MetadataBuilder.BuildGraphWarningsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("graph_nodes"), Fields))
    {
        Metadata->SetObjectField(TEXT("graph_nodes"), MetadataBuilder.BuildGraphNodesInfo(Blueprint, Filter));
    }

    return Metadata;
}

FString FGetBlueprintMetadataCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), Metadata);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetBlueprintMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

bool FGetBlueprintMetadataCommand::ShouldIncludeField(const FString& FieldName, const TArray<FString>& RequestedFields) const
{
    // If "*" is in the list, include all fields
    if (RequestedFields.Contains(TEXT("*")))
    {
        return true;
    }

    return RequestedFields.Contains(FieldName);
}
