#include "Commands/BlueprintNode/AddBlueprintInputActionNodeCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FAddBlueprintInputActionNodeCommand::FAddBlueprintInputActionNodeCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FAddBlueprintInputActionNodeCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, ActionName;
    FVector2D Position;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ActionName, Position, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the Blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Add the input action node using the service
    FString NodeId;
    if (BlueprintNodeService.AddInputActionNode(Blueprint, ActionName, Position, NodeId))
    {
        return CreateSuccessResponse(NodeId);
    }
    else
    {
        return CreateErrorResponse(TEXT("Failed to add input action node"));
    }
}

FString FAddBlueprintInputActionNodeCommand::GetCommandName() const
{
    return TEXT("add_blueprint_input_action_node");
}

bool FAddBlueprintInputActionNodeCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, ActionName;
    FVector2D Position;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, ActionName, Position, ParseError);
}

bool FAddBlueprintInputActionNodeCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutActionName, FVector2D& OutPosition, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Parse required action_name parameter
    if (!JsonObject->TryGetStringField(TEXT("action_name"), OutActionName))
    {
        OutError = TEXT("Missing required 'action_name' parameter");
        return false;
    }
    
    // Parse optional node_position parameter
    const TArray<TSharedPtr<FJsonValue>>* PositionArray;
    if (JsonObject->TryGetArrayField(TEXT("node_position"), PositionArray) && PositionArray->Num() >= 2)
    {
        OutPosition.X = (*PositionArray)[0]->AsNumber();
        OutPosition.Y = (*PositionArray)[1]->AsNumber();
    }
    else
    {
        OutPosition = FVector2D(0.0f, 0.0f);
    }
    
    return true;
}

FString FAddBlueprintInputActionNodeCommand::CreateSuccessResponse(const FString& NodeId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddBlueprintInputActionNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
