#include "Commands/UnrealMCPBlueprintActionCommandsHandler.h"
#include "Commands/UnrealMCPBlueprintActionCommands.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("get_actions_for_pin"))
    {
        return GetActionsForPin(Params);
    }
    else if (CommandType == TEXT("get_actions_for_class"))
    {
        return GetActionsForClass(Params);
    }
    else if (CommandType == TEXT("get_actions_for_class_hierarchy"))
    {
        return GetActionsForClassHierarchy(Params);
    }
    else if (CommandType == TEXT("get_node_pin_info"))
    {
        return GetNodePinInfo(Params);
    }
    else if (CommandType == TEXT("search_blueprint_actions"))
    {
        return SearchBlueprintActions(Params);
    }
    else if (CommandType == TEXT("create_node_by_action_name"))
    {
        return CreateNodeByActionName(Params);
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown Blueprint action command: %s"), *CommandType));
        return ErrorResponse;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::GetActionsForPin(const TSharedPtr<FJsonObject>& Params)
{
    FString PinType = Params->GetStringField(TEXT("pin_type"));
    FString PinSubCategory = Params->GetStringField(TEXT("pin_subcategory"));
    FString SearchFilter = Params->GetStringField(TEXT("search_filter"));
    int32 MaxResults = Params->GetIntegerField(TEXT("max_results"));
    if (MaxResults <= 0) MaxResults = 50; // Default value
    
    FString JsonResult = UUnrealMCPBlueprintActionCommands::GetActionsForPin(PinType, PinSubCategory, SearchFilter, MaxResults);
    
    // Parse the JSON result back into an object
    TSharedPtr<FJsonObject> ParsedResult;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResult);
    
    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
    {
        return ParsedResult;
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), TEXT("Failed to parse actions for pin result"));
        return ErrorResponse;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::GetActionsForClass(const TSharedPtr<FJsonObject>& Params)
{
    FString ClassName = Params->GetStringField(TEXT("class_name"));
    FString SearchFilter = Params->GetStringField(TEXT("search_filter"));
    int32 MaxResults = Params->GetIntegerField(TEXT("max_results"));
    if (MaxResults <= 0) MaxResults = 50; // Default value
    
    FString JsonResult = UUnrealMCPBlueprintActionCommands::GetActionsForClass(ClassName, SearchFilter, MaxResults);
    
    // Parse the JSON result back into an object
    TSharedPtr<FJsonObject> ParsedResult;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResult);
    
    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
    {
        return ParsedResult;
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), TEXT("Failed to parse actions for class result"));
        return ErrorResponse;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::GetActionsForClassHierarchy(const TSharedPtr<FJsonObject>& Params)
{
    FString ClassName = Params->GetStringField(TEXT("class_name"));
    FString SearchFilter = Params->GetStringField(TEXT("search_filter"));
    int32 MaxResults = Params->GetIntegerField(TEXT("max_results"));
    if (MaxResults <= 0) MaxResults = 50; // Default value
    
    FString JsonResult = UUnrealMCPBlueprintActionCommands::GetActionsForClassHierarchy(ClassName, SearchFilter, MaxResults);
    
    // Parse the JSON result back into an object
    TSharedPtr<FJsonObject> ParsedResult;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResult);
    
    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
    {
        return ParsedResult;
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), TEXT("Failed to parse actions for class hierarchy result"));
        return ErrorResponse;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::GetNodePinInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString NodeName = Params->GetStringField(TEXT("node_name"));
    FString PinName = Params->GetStringField(TEXT("pin_name"));
    
    FString JsonResult = UUnrealMCPBlueprintActionCommands::GetNodePinInfo(NodeName, PinName);
    
    // Parse the JSON result back into an object
    TSharedPtr<FJsonObject> ParsedResult;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResult);
    
    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
    {
        return ParsedResult;
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), TEXT("Failed to parse node pin info result"));
        return ErrorResponse;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::SearchBlueprintActions(const TSharedPtr<FJsonObject>& Params)
{
    FString SearchQuery = Params->GetStringField(TEXT("search_query"));
    FString Category = Params->GetStringField(TEXT("category"));
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    int32 MaxResults = Params->GetIntegerField(TEXT("max_results"));
    if (MaxResults <= 0) MaxResults = 50; // Default value
    
    FString JsonResult = UUnrealMCPBlueprintActionCommands::SearchBlueprintActions(SearchQuery, Category, MaxResults, BlueprintName);
    
    // Parse the JSON result back into an object
    TSharedPtr<FJsonObject> ParsedResult;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResult);
    
    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
    {
        return ParsedResult;
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), TEXT("Failed to parse search blueprint actions result"));
        return ErrorResponse;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintActionCommandsHandler::CreateNodeByActionName(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString FunctionName = Params->GetStringField(TEXT("function_name"));
    FString ClassName;
    FString NodePosition;
    FString JsonParams;
    
    // Handle optional parameters safely
    if (Params->HasField(TEXT("class_name")))
    {
        ClassName = Params->GetStringField(TEXT("class_name"));
    }
    
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = Params->GetStringField(TEXT("node_position"));
    }
    
    if (Params->HasField(TEXT("json_params")))
    {
        JsonParams = Params->GetStringField(TEXT("json_params"));
    }
    
    FString JsonResult = UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(BlueprintName, FunctionName, ClassName, NodePosition, JsonParams);
    
    // Parse the JSON result back into an object
    TSharedPtr<FJsonObject> ParsedResult;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResult);
    
    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
    {
        return ParsedResult;
    }
    else
    {
        TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
        ErrorResponse->SetBoolField(TEXT("success"), false);
        ErrorResponse->SetStringField(TEXT("error"), TEXT("Failed to parse create node by action name result"));
        return ErrorResponse;
    }
} 