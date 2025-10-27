#include "Commands/UMG/CreateParentChildWidgetCommand.h"
#include "Services/UMG/IUMGService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FCreateParentChildWidgetCommand::FCreateParentChildWidgetCommand(IUMGService& InUMGService)
    : UMGService(InUMGService)
{
}

FString FCreateParentChildWidgetCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, ParentComponentName, ChildComponentName, ParentComponentType, ChildComponentType, Error;
    FVector2D ParentPosition, ParentSize;
    TSharedPtr<FJsonObject> ChildAttributes;
    
    if (!ParseParameters(Parameters, BlueprintName, ParentComponentName, ChildComponentName, 
                        ParentComponentType, ChildComponentType, ParentPosition, ParentSize, ChildAttributes, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Use the UMG service to create parent and child widget components
    bool bSuccess = UMGService.CreateParentAndChildWidgetComponents(BlueprintName, ParentComponentName, ChildComponentName,
                                                                   ParentComponentType, ChildComponentType, 
                                                                   ParentPosition, ParentSize, ChildAttributes);
    if (!bSuccess)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to create parent '%s' and child '%s' widget components"), 
                                                 *ParentComponentName, *ChildComponentName));
    }

    return CreateSuccessResponse(BlueprintName, ParentComponentName, ChildComponentName);
}

bool FCreateParentChildWidgetCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutParentComponentName,
                                                                   FString& OutChildComponentName, FString& OutParentComponentType, FString& OutChildComponentType,
                                                                   FVector2D& OutParentPosition, FVector2D& OutParentSize, TSharedPtr<FJsonObject>& OutChildAttributes,
                                                                   FString& OutError) const
{
    UE_LOG(LogTemp, Warning, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Parsing JSON: %s"), *JsonString);
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        UE_LOG(LogTemp, Error, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Invalid JSON format"));
        return false;
    }

    // Required parameters - try both widget_name and blueprint_name for compatibility
    if (!JsonObject->TryGetStringField(TEXT("widget_name"), OutBlueprintName) && 
        !JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing 'widget_name' or 'blueprint_name' parameter");
        UE_LOG(LogTemp, Error, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Missing widget_name/blueprint_name parameter"));
        return false;
    }
    
    if (OutBlueprintName.IsEmpty())
    {
        OutError = TEXT("Empty 'widget_name' or 'blueprint_name' parameter");
        UE_LOG(LogTemp, Error, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Empty widget_name/blueprint_name parameter"));
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("parent_component_name"), OutParentComponentName) || OutParentComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'parent_component_name' parameter");
        UE_LOG(LogTemp, Error, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Missing parent_component_name parameter"));
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("child_component_name"), OutChildComponentName) || OutChildComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'child_component_name' parameter");
        UE_LOG(LogTemp, Error, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Missing child_component_name parameter"));
        return false;
    }

    // Optional parameters with defaults
    if (!JsonObject->TryGetStringField(TEXT("parent_component_type"), OutParentComponentType) || OutParentComponentType.IsEmpty())
    {
        OutParentComponentType = TEXT("Border");
    }

    if (!JsonObject->TryGetStringField(TEXT("child_component_type"), OutChildComponentType) || OutChildComponentType.IsEmpty())
    {
        OutChildComponentType = TEXT("TextBlock");
    }

    // Optional position parameter
    const TArray<TSharedPtr<FJsonValue>>* PositionArray;
    if (JsonObject->TryGetArrayField(TEXT("parent_position"), PositionArray) && PositionArray->Num() >= 2)
    {
        OutParentPosition.X = (*PositionArray)[0]->AsNumber();
        OutParentPosition.Y = (*PositionArray)[1]->AsNumber();
    }
    else
    {
        OutParentPosition = FVector2D(0.0f, 0.0f);
    }

    // Optional size parameter
    const TArray<TSharedPtr<FJsonValue>>* SizeArray;
    if (JsonObject->TryGetArrayField(TEXT("parent_size"), SizeArray) && SizeArray->Num() >= 2)
    {
        OutParentSize.X = (*SizeArray)[0]->AsNumber();
        OutParentSize.Y = (*SizeArray)[1]->AsNumber();
    }
    else
    {
        OutParentSize = FVector2D(300.0f, 200.0f);
    }

    // Optional child attributes parameter
    const TSharedPtr<FJsonObject>* ChildAttributesObject;
    if (JsonObject->TryGetObjectField(TEXT("child_attributes"), ChildAttributesObject))
    {
        OutChildAttributes = *ChildAttributesObject;
    }
    else
    {
        OutChildAttributes = MakeShared<FJsonObject>();
    }

    UE_LOG(LogTemp, Warning, TEXT("FCreateParentChildWidgetCommand::ParseParameters: Successfully parsed parameters"));
    return true;
}

FString FCreateParentChildWidgetCommand::GetCommandName() const
{
    return TEXT("create_parent_and_child_widget_components");
}

bool FCreateParentChildWidgetCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, ParentComponentName, ChildComponentName, ParentComponentType, ChildComponentType, Error;
    FVector2D ParentPosition, ParentSize;
    TSharedPtr<FJsonObject> ChildAttributes;
    return ParseParameters(Parameters, BlueprintName, ParentComponentName, ChildComponentName, 
                          ParentComponentType, ChildComponentType, ParentPosition, ParentSize, ChildAttributes, Error);
}

FString FCreateParentChildWidgetCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& ParentComponentName, const FString& ChildComponentName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("parent_component_name"), ParentComponentName);
    ResponseObj->SetStringField(TEXT("child_component_name"), ChildComponentName);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Parent and child widget components created successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCreateParentChildWidgetCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

