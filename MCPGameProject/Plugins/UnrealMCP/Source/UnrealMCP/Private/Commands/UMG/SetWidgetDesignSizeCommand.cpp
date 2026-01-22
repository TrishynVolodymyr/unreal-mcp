#include "Commands/UMG/SetWidgetDesignSizeCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FSetWidgetDesignSizeCommand::FSetWidgetDesignSizeCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FSetWidgetDesignSizeCommand::Execute(const FString& Parameters)
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

TSharedPtr<FJsonObject> FSetWidgetDesignSizeCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
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
    FString DesignSizeMode = Params->GetStringField(TEXT("design_size_mode"));

    // Get optional custom dimensions (default to 1920x1080)
    int32 CustomWidth = 1920;
    int32 CustomHeight = 1080;

    if (Params->HasField(TEXT("custom_width")))
    {
        CustomWidth = static_cast<int32>(Params->GetNumberField(TEXT("custom_width")));
    }
    if (Params->HasField(TEXT("custom_height")))
    {
        CustomHeight = static_cast<int32>(Params->GetNumberField(TEXT("custom_height")));
    }

    // Use the UMG service to set design size mode
    FString OutError;
    bool bSuccess = UMGService->SetWidgetDesignSizeMode(WidgetName, DesignSizeMode, CustomWidth, CustomHeight, OutError);
    if (!bSuccess)
    {
        FString ErrorMessage = OutError.IsEmpty()
            ? FString::Printf(TEXT("Failed to set design size mode for widget '%s'"), *WidgetName)
            : OutError;
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(ErrorMessage);
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(WidgetName, DesignSizeMode, CustomWidth, CustomHeight);
}

FString FSetWidgetDesignSizeCommand::GetCommandName() const
{
    return TEXT("set_widget_design_size_mode");
}

bool FSetWidgetDesignSizeCommand::ValidateParams(const FString& Parameters) const
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

bool FSetWidgetDesignSizeCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_name' parameter");
        return false;
    }

    FString DesignSizeMode;
    if (!Params->TryGetStringField(TEXT("design_size_mode"), DesignSizeMode) || DesignSizeMode.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'design_size_mode' parameter");
        return false;
    }

    // Validate design size mode value
    TArray<FString> ValidModes = { TEXT("DesiredOnScreen"), TEXT("Custom"), TEXT("FillScreen"), TEXT("CustomOnScreen") };
    if (!ValidModes.Contains(DesignSizeMode))
    {
        OutError = FString::Printf(TEXT("Invalid design_size_mode '%s'. Must be one of: DesiredOnScreen, Custom, FillScreen, CustomOnScreen"), *DesignSizeMode);
        return false;
    }

    return true;
}

TSharedPtr<FJsonObject> FSetWidgetDesignSizeCommand::CreateSuccessResponse(const FString& WidgetName, const FString& DesignSizeMode,
                                                                            int32 Width, int32 Height) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResponseObj->SetStringField(TEXT("design_size_mode"), DesignSizeMode);
    ResponseObj->SetNumberField(TEXT("width"), Width);
    ResponseObj->SetNumberField(TEXT("height"), Height);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Design size mode set to '%s' (%dx%d)"), *DesignSizeMode, Width, Height));

    return ResponseObj;
}

TSharedPtr<FJsonObject> FSetWidgetDesignSizeCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);

    return ResponseObj;
}
