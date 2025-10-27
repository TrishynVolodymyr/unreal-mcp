#include "Commands/Blueprint/SetPawnPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FSetPawnPropertiesCommand::FSetPawnPropertiesCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FSetPawnPropertiesCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    TMap<FString, FString> PawnParams;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, PawnParams, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Set pawn properties using the service
    if (!BlueprintService.SetPawnProperties(Blueprint, PawnParams))
    {
        return CreateErrorResponse(TEXT("Failed to set pawn properties"));
    }
    
    return CreateSuccessResponse(BlueprintName);
}

FString FSetPawnPropertiesCommand::GetCommandName() const
{
    return TEXT("set_pawn_properties");
}

bool FSetPawnPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    TMap<FString, FString> PawnParams;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, PawnParams, ParseError);
}

bool FSetPawnPropertiesCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                               TMap<FString, FString>& OutPawnParams, FString& OutError) const
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
    
    // Parse optional pawn parameters
    FString StringValue;
    if (JsonObject->TryGetStringField(TEXT("auto_possess_player"), StringValue))
    {
        OutPawnParams.Add(TEXT("auto_possess_player"), StringValue);
    }
    
    bool BoolValue;
    if (JsonObject->TryGetBoolField(TEXT("use_controller_rotation_yaw"), BoolValue))
    {
        OutPawnParams.Add(TEXT("use_controller_rotation_yaw"), BoolValue ? TEXT("true") : TEXT("false"));
    }
    if (JsonObject->TryGetBoolField(TEXT("use_controller_rotation_pitch"), BoolValue))
    {
        OutPawnParams.Add(TEXT("use_controller_rotation_pitch"), BoolValue ? TEXT("true") : TEXT("false"));
    }
    if (JsonObject->TryGetBoolField(TEXT("use_controller_rotation_roll"), BoolValue))
    {
        OutPawnParams.Add(TEXT("use_controller_rotation_roll"), BoolValue ? TEXT("true") : TEXT("false"));
    }
    if (JsonObject->TryGetBoolField(TEXT("can_be_damaged"), BoolValue))
    {
        OutPawnParams.Add(TEXT("can_be_damaged"), BoolValue ? TEXT("true") : TEXT("false"));
    }
    
    return true;
}

FString FSetPawnPropertiesCommand::CreateSuccessResponse(const FString& BlueprintName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetPawnPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



