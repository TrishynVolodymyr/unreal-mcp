#include "Commands/UMG/ReorderWidgetChildrenCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FReorderWidgetChildrenCommand::FReorderWidgetChildrenCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FReorderWidgetChildrenCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
        TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);

        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Use internal execution with JSON objects
    TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);

    // Convert response back to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

TSharedPtr<FJsonObject> FReorderWidgetChildrenCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!UMGService.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG Service is not available"));
        return CreateErrorResponse(Error);
    }

    // Validate parameters
    FString ValidationError;
    if (!ValidateParamsInternal(Params, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return CreateErrorResponse(Error);
    }

    // Extract parameters
    FString WidgetName = Params->GetStringField(TEXT("widget_name"));
    FString ContainerName = Params->GetStringField(TEXT("container_name"));

    TArray<FString> ChildOrder;
    const TArray<TSharedPtr<FJsonValue>>* ChildOrderArray;
    if (Params->TryGetArrayField(TEXT("child_order"), ChildOrderArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *ChildOrderArray)
        {
            ChildOrder.Add(Value->AsString());
        }
    }

    // Use the UMG service to reorder children
    bool bSuccess = UMGService->ReorderWidgetChildren(WidgetName, ContainerName, ChildOrder);
    if (!bSuccess)
    {
        FString ErrorMessage = FString::Printf(TEXT("Failed to reorder children in container '%s' of widget '%s'"),
                                               *ContainerName, *WidgetName);
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(ErrorMessage);
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(WidgetName, ContainerName, ChildOrder);
}

FString FReorderWidgetChildrenCommand::GetCommandName() const
{
    return TEXT("reorder_widget_children");
}

bool FReorderWidgetChildrenCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ValidationError;
    return ValidateParamsInternal(JsonObject, ValidationError);
}

bool FReorderWidgetChildrenCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_name' parameter");
        return false;
    }

    FString ContainerName;
    if (!Params->TryGetStringField(TEXT("container_name"), ContainerName) || ContainerName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'container_name' parameter");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* ChildOrderArray;
    if (!Params->TryGetArrayField(TEXT("child_order"), ChildOrderArray) || ChildOrderArray->Num() == 0)
    {
        OutError = TEXT("Missing or empty 'child_order' array parameter");
        return false;
    }

    return true;
}

TSharedPtr<FJsonObject> FReorderWidgetChildrenCommand::CreateSuccessResponse(const FString& WidgetName, const FString& ContainerName, const TArray<FString>& ChildOrder) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResponseObj->SetStringField(TEXT("container_name"), ContainerName);

    TArray<TSharedPtr<FJsonValue>> ChildOrderJson;
    for (const FString& ChildName : ChildOrder)
    {
        ChildOrderJson.Add(MakeShared<FJsonValueString>(ChildName));
    }
    ResponseObj->SetArrayField(TEXT("child_order"), ChildOrderJson);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Children reordered successfully"));

    return ResponseObj;
}

TSharedPtr<FJsonObject> FReorderWidgetChildrenCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);

    return ResponseObj;
}
