#include "Commands/UMG/AddWidgetComponentCommand.h"
#include "Services/UMG/IUMGService.h"
#include "Components/Widget.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FAddWidgetComponentCommand::FAddWidgetComponentCommand(IUMGService& InUMGService)
    : UMGService(InUMGService)
{
}

FString FAddWidgetComponentCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, ComponentName, ComponentType, Error;
    FVector2D Position, Size;
    TSharedPtr<FJsonObject> Kwargs;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentName, ComponentType, Position, Size, Kwargs, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Use the UMG service to add the widget component
    UWidget* CreatedWidget = UMGService.AddWidgetComponent(BlueprintName, ComponentName, ComponentType, Position, Size, Kwargs);
    if (!CreatedWidget)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to create widget component: %s of type %s"), *ComponentName, *ComponentType));
    }

    return CreateSuccessResponse(CreatedWidget, ComponentName, ComponentType);
}

FString FAddWidgetComponentCommand::GetCommandName() const
{
    return TEXT("add_widget_component_to_widget");
}

bool FAddWidgetComponentCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, ComponentName, ComponentType, Error;
    FVector2D Position, Size;
    TSharedPtr<FJsonObject> Kwargs;
    return ParseParameters(Parameters, BlueprintName, ComponentName, ComponentType, Position, Size, Kwargs, Error);
}

bool FAddWidgetComponentCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName, 
                                               FString& OutComponentType, FVector2D& OutPosition, FVector2D& OutSize, 
                                               TSharedPtr<FJsonObject>& OutKwargs, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        return false;
    }

    // Required parameters
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName) || OutBlueprintName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'blueprint_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("component_name"), OutComponentName) || OutComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'component_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("component_type"), OutComponentType) || OutComponentType.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'component_type' parameter");
        return false;
    }

    // Optional position parameter
    const TArray<TSharedPtr<FJsonValue>>* PositionArray;
    if (JsonObject->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
    {
        OutPosition.X = (*PositionArray)[0]->AsNumber();
        OutPosition.Y = (*PositionArray)[1]->AsNumber();
    }
    else
    {
        OutPosition = FVector2D(0.0f, 0.0f);
    }

    // Optional size parameter
    const TArray<TSharedPtr<FJsonValue>>* SizeArray;
    if (JsonObject->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
    {
        OutSize.X = (*SizeArray)[0]->AsNumber();
        OutSize.Y = (*SizeArray)[1]->AsNumber();
    }
    else
    {
        OutSize = FVector2D(100.0f, 50.0f);
    }

    // Optional kwargs parameter
    FString KwargsString;
    if (JsonObject->TryGetStringField(TEXT("kwargs"), KwargsString))
    {
        TSharedRef<TJsonReader<>> KwargsReader = TJsonReaderFactory<>::Create(KwargsString);
        if (!FJsonSerializer::Deserialize(KwargsReader, OutKwargs))
        {
            OutKwargs = MakeShared<FJsonObject>();
        }
    }
    else
    {
        OutKwargs = MakeShared<FJsonObject>();
    }

    return true;
}

FString FAddWidgetComponentCommand::CreateSuccessResponse(UWidget* Widget, const FString& ComponentName, const FString& ComponentType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    ResponseObj->SetStringField(TEXT("component_type"), ComponentType);
    ResponseObj->SetStringField(TEXT("widget_class"), Widget ? Widget->GetClass()->GetName() : TEXT("Unknown"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddWidgetComponentCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

