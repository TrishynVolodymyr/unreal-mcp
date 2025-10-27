#include "Commands/BlueprintNode/AddBlueprintVariableNodeCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FAddBlueprintVariableNodeCommand::FAddBlueprintVariableNodeCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FAddBlueprintVariableNodeCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, VariableName;
    bool bIsGetter = true;
    FVector2D Position;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, VariableName, bIsGetter, Position, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    FString NodeId;
    if (BlueprintNodeService.AddVariableNode(Blueprint, VariableName, bIsGetter, Position, NodeId))
    {
        return CreateSuccessResponse(NodeId);
    }
    else
    {
        return CreateErrorResponse(TEXT("Failed to add variable node"));
    }
}

FString FAddBlueprintVariableNodeCommand::GetCommandName() const
{
    return TEXT("add_blueprint_variable_node");
}

bool FAddBlueprintVariableNodeCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, VariableName;
    bool bIsGetter;
    FVector2D Position;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, VariableName, bIsGetter, Position, ParseError);
}

bool FAddBlueprintVariableNodeCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutVariableName, bool& OutIsGetter, FVector2D& OutPosition, FString& OutError) const
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
    
    if (!JsonObject->TryGetStringField(TEXT("variable_name"), OutVariableName))
    {
        OutError = TEXT("Missing required 'variable_name' parameter");
        return false;
    }
    
    // Default to getter if not specified
    if (!JsonObject->TryGetBoolField(TEXT("is_getter"), OutIsGetter))
    {
        OutIsGetter = true;
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

FString FAddBlueprintVariableNodeCommand::CreateSuccessResponse(const FString& NodeId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddBlueprintVariableNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
