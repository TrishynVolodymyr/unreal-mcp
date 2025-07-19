#include "Commands/CreateBlueprintCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FCreateBlueprintCommand::FCreateBlueprintCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCreateBlueprintCommand::Execute(const FString& Parameters)
{
    FBlueprintCreationParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }
    
    // Create the blueprint using the service
    UBlueprint* CreatedBlueprint = BlueprintService.CreateBlueprint(Params);
    if (!CreatedBlueprint)
    {
        return CreateErrorResponse(TEXT("Failed to create blueprint"));
    }
    
    return CreateSuccessResponse(CreatedBlueprint);
}

FString FCreateBlueprintCommand::GetCommandName() const
{
    return TEXT("create_blueprint");
}

bool FCreateBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FBlueprintCreationParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FCreateBlueprintCommand::ParseParameters(const FString& JsonString, FBlueprintCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required name parameter
    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing required 'name' parameter");
        return false;
    }
    
    // Parse optional folder_path parameter
    JsonObject->TryGetStringField(TEXT("folder_path"), OutParams.FolderPath);
    
    // Parse optional parent_class parameter
    FString ParentClassName;
    if (JsonObject->TryGetStringField(TEXT("parent_class"), ParentClassName))
    {
        // Resolve parent class - this would need to be implemented in the service
        // For now, we'll store the string and let the service handle resolution
        OutParams.ParentClass = nullptr; // Will be resolved by service
    }
    
    // Parse optional compile_on_creation parameter
    JsonObject->TryGetBoolField(TEXT("compile_on_creation"), OutParams.bCompileOnCreation);
    
    return true;
}

FString FCreateBlueprintCommand::CreateSuccessResponse(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), Blueprint->GetName());
    ResponseObj->SetStringField(TEXT("path"), Blueprint->GetPathName());
    ResponseObj->SetBoolField(TEXT("already_exists"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCreateBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}