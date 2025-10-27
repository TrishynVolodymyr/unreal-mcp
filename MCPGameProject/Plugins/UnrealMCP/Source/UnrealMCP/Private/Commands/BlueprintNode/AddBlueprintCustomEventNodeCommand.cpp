#include "Commands/BlueprintNode/AddBlueprintCustomEventNodeCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FAddBlueprintCustomEventNodeCommand::FAddBlueprintCustomEventNodeCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FAddBlueprintCustomEventNodeCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, EventName;
    FVector2D Position;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, EventName, Position, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    FString NodeId;
    if (BlueprintNodeService.AddCustomEventNode(Blueprint, EventName, Position, NodeId))
    {
        return CreateSuccessResponse(NodeId);
    }
    else
    {
        return CreateErrorResponse(TEXT("Failed to add custom event node"));
    }
}

FString FAddBlueprintCustomEventNodeCommand::GetCommandName() const
{
    return TEXT("add_blueprint_custom_event_node");
}

bool FAddBlueprintCustomEventNodeCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, EventName;
    FVector2D Position;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, EventName, Position, ParseError);
}

bool FAddBlueprintCustomEventNodeCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutEventName, FVector2D& OutPosition, FString& OutError) const
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
    
    if (!JsonObject->TryGetStringField(TEXT("event_name"), OutEventName))
    {
        OutError = TEXT("Missing required 'event_name' parameter");
        return false;
    }
    
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

FString FAddBlueprintCustomEventNodeCommand::CreateSuccessResponse(const FString& NodeId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddBlueprintCustomEventNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
