#include "Commands/BlueprintNode/CreateNodeByActionNameCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Services/BlueprintActionService.h"
#include "MCPErrorHandler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FCreateNodeByActionNameCommand::FCreateNodeByActionNameCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService)
    : BlueprintActionService(InBlueprintActionService)
{
}

FString FCreateNodeByActionNameCommand::Execute(const FString& Parameters)
{
    if (!BlueprintActionService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionNameCommand: BlueprintActionService is not valid!"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("Blueprint action service is not available"));
        return FMCPErrorHandler::CreateStructuredErrorResponse(Error);
    }

    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
        return FMCPErrorHandler::CreateStructuredErrorResponse(Error);
    }

    // Basic parameter validation
    if (!JsonObject->HasField(TEXT("blueprint_name")) || JsonObject->GetStringField(TEXT("blueprint_name")).IsEmpty())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Blueprint name is required"));
        return FMCPErrorHandler::CreateStructuredErrorResponse(Error);
    }
    
    if (!JsonObject->HasField(TEXT("function_name")) || JsonObject->GetStringField(TEXT("function_name")).IsEmpty())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Function name is required"));
        return FMCPErrorHandler::CreateStructuredErrorResponse(Error);
    }

    // Extract parameters
    FString BlueprintName = JsonObject->GetStringField(TEXT("blueprint_name"));
    FString FunctionName = JsonObject->GetStringField(TEXT("function_name"));
    FString ClassName = JsonObject->GetStringField(TEXT("class_name"));
    FString NodePosition = JsonObject->GetStringField(TEXT("node_position"));
    FString TargetGraph = JsonObject->GetStringField(TEXT("target_graph"));
    FString JsonParams = JsonObject->GetStringField(TEXT("json_params"));

    UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionNameCommand: BlueprintName=%s, FunctionName=%s, ClassName=%s, TargetGraph=%s"), 
           *BlueprintName, *FunctionName, *ClassName, *TargetGraph);

    // Pass TargetGraph explicitly through the entire service chain
    FString Result = BlueprintActionService->CreateNodeByActionName(
        BlueprintName, FunctionName, ClassName, NodePosition, JsonParams, TargetGraph
    );
    
    // The service already returns a properly formatted JSON response
    return Result;
}

FString FCreateNodeByActionNameCommand::GetCommandName() const
{
    return TEXT("create_node_by_action_name");
}

bool FCreateNodeByActionNameCommand::ValidateParams(const FString& Parameters) const
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // Basic parameter validation
    if (!JsonObject->HasField(TEXT("blueprint_name")) || JsonObject->GetStringField(TEXT("blueprint_name")).IsEmpty())
    {
        return false;
    }
    
    if (!JsonObject->HasField(TEXT("function_name")) || JsonObject->GetStringField(TEXT("function_name")).IsEmpty())
    {
        return false;
    }
    
    return true;
}
