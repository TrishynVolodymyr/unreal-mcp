#include "Commands/UMG/CreateWidgetBlueprintCommand.h"
#include "Services/UMG/IUMGService.h"
#include "WidgetBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FCreateWidgetBlueprintCommand::FCreateWidgetBlueprintCommand(IUMGService& InUMGService)
    : UMGService(InUMGService)
{
}

FString FCreateWidgetBlueprintCommand::Execute(const FString& Parameters)
{
    FString Name, ParentClass, Path, Error;
    if (!ParseParameters(Parameters, Name, ParentClass, Path, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Use the UMG service to create the widget blueprint
    UWidgetBlueprint* CreatedBlueprint = UMGService.CreateWidgetBlueprint(Name, ParentClass, Path);
    if (!CreatedBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to create widget blueprint: %s"), *Name));
    }

    // Check if it already existed
    bool bAlreadyExists = UMGService.DoesWidgetBlueprintExist(Name, Path);
    
    return CreateSuccessResponse(CreatedBlueprint, Path, bAlreadyExists);
}

FString FCreateWidgetBlueprintCommand::GetCommandName() const
{
    return TEXT("create_umg_widget_blueprint");
}

bool FCreateWidgetBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FString Name, ParentClass, Path, Error;
    return ParseParameters(Parameters, Name, ParentClass, Path, Error);
}

bool FCreateWidgetBlueprintCommand::ParseParameters(const FString& JsonString, FString& OutName, FString& OutParentClass, FString& OutPath, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        return false;
    }

    // Required parameter: name
    if (!JsonObject->TryGetStringField(TEXT("name"), OutName) || OutName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'name' parameter");
        return false;
    }

    // Optional parameters
    JsonObject->TryGetStringField(TEXT("parent_class"), OutParentClass);
    JsonObject->TryGetStringField(TEXT("path"), OutPath);
    
    // Set defaults if not provided
    if (OutParentClass.IsEmpty())
    {
        OutParentClass = TEXT("UserWidget");
    }
    
    if (OutPath.IsEmpty())
    {
        OutPath = TEXT("/Game/Widgets");
    }

    return true;
}

FString FCreateWidgetBlueprintCommand::CreateSuccessResponse(UWidgetBlueprint* WidgetBlueprint, const FString& Path, bool bAlreadyExists) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), WidgetBlueprint->GetName());
    ResponseObj->SetStringField(TEXT("path"), Path + TEXT("/") + WidgetBlueprint->GetName());
    ResponseObj->SetBoolField(TEXT("already_exists"), bAlreadyExists);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCreateWidgetBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}