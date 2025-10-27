#include "Commands/Blueprint/ListBlueprintComponentsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FListBlueprintComponentsCommand::FListBlueprintComponentsCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FListBlueprintComponentsCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Get component list using the service
    TArray<TPair<FString, FString>> Components;
    if (!BlueprintService.GetBlueprintComponents(Blueprint, Components))
    {
        return CreateErrorResponse(TEXT("Failed to get blueprint components"));
    }
    
    return CreateSuccessResponse(BlueprintName, Components);
}

FString FListBlueprintComponentsCommand::GetCommandName() const
{
    return TEXT("list_blueprint_components");
}

bool FListBlueprintComponentsCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, ParseError);
}

bool FListBlueprintComponentsCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                                     FString& OutError) const
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
    
    return true;
}

FString FListBlueprintComponentsCommand::CreateSuccessResponse(const FString& BlueprintName, 
                                                              const TArray<TPair<FString, FString>>& Components) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    
    TArray<TSharedPtr<FJsonValue>> ComponentArray;
    for (const auto& Component : Components)
    {
        TSharedPtr<FJsonObject> ComponentObj = MakeShared<FJsonObject>();
        ComponentObj->SetStringField(TEXT("name"), Component.Key);
        ComponentObj->SetStringField(TEXT("type"), Component.Value);
        ComponentArray.Add(MakeShared<FJsonValueObject>(ComponentObj));
    }
    ResponseObj->SetArrayField(TEXT("components"), ComponentArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FListBlueprintComponentsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



