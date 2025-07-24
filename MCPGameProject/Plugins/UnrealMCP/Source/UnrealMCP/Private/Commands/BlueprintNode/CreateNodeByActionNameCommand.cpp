#include "Commands/BlueprintNode/CreateNodeByActionNameCommand.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPBlueprintActionCommands.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FCreateNodeByActionNameCommand::FCreateNodeByActionNameCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FCreateNodeByActionNameCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, FunctionName, ClassName, NodePosition, JsonParams;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, FunctionName, ClassName, NodePosition, JsonParams, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Verify Blueprint exists
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Use the Blueprint Action Commands to create the node
    FString Result = UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(
        BlueprintName, FunctionName, ClassName, NodePosition, JsonParams
    );
    
    // Parse the result to extract information and reformat if needed
    TSharedPtr<FJsonObject> ResultObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result);
    
    if (FJsonSerializer::Deserialize(Reader, ResultObject) && ResultObject.IsValid())
    {
        bool bSuccess = false;
        if (ResultObject->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
        {
            FString NodeId, NodeType, Message;
            ResultObject->TryGetStringField(TEXT("node_id"), NodeId);
            ResultObject->TryGetStringField(TEXT("node_type"), NodeType);
            ResultObject->TryGetStringField(TEXT("message"), Message);
            
            return CreateSuccessResponse(NodeId, NodeType, Message);
        }
        else
        {
            FString ErrorMessage;
            if (ResultObject->TryGetStringField(TEXT("message"), ErrorMessage))
            {
                return CreateErrorResponse(ErrorMessage);
            }
        }
    }
    
    return CreateErrorResponse(TEXT("Failed to create node by action name"));
}

FString FCreateNodeByActionNameCommand::GetCommandName() const
{
    return TEXT("create_node_by_action_name");
}

bool FCreateNodeByActionNameCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, FunctionName, ClassName, NodePosition, JsonParams;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, FunctionName, ClassName, NodePosition, JsonParams, ParseError);
}

bool FCreateNodeByActionNameCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutFunctionName, 
                                                    FString& OutClassName, FString& OutNodePosition, FString& OutJsonParams, FString& OutError) const
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
    
    if (!JsonObject->TryGetStringField(TEXT("function_name"), OutFunctionName))
    {
        OutError = TEXT("Missing required 'function_name' parameter");
        return false;
    }
    
    // Optional parameters
    JsonObject->TryGetStringField(TEXT("class_name"), OutClassName);
    JsonObject->TryGetStringField(TEXT("node_position"), OutNodePosition);
    JsonObject->TryGetStringField(TEXT("kwargs"), OutJsonParams);
    
    return true;
}

FString FCreateNodeByActionNameCommand::CreateSuccessResponse(const FString& NodeId, const FString& NodeType, const FString& Message) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    ResponseObj->SetStringField(TEXT("node_type"), NodeType);
    ResponseObj->SetStringField(TEXT("message"), Message);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCreateNodeByActionNameCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}