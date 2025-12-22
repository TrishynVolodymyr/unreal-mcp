#include "Commands/UMG/CreateWidgetInputHandlerCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FCreateWidgetInputHandlerCommand::FCreateWidgetInputHandlerCommand(TSharedPtr<IUMGService> InUMGService)
	: UMGService(InUMGService)
{
}

FString FCreateWidgetInputHandlerCommand::Execute(const FString& Parameters)
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

FString FCreateWidgetInputHandlerCommand::GetCommandName() const
{
	return TEXT("create_widget_input_handler");
}

bool FCreateWidgetInputHandlerCommand::ValidateParams(const FString& Parameters) const
{
	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString ValidationError;
	return ValidateParamsInternal(JsonObject, ValidationError);
}

TSharedPtr<FJsonObject> FCreateWidgetInputHandlerCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
	if (!UMGService.IsValid())
	{
		FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
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
	FString ComponentName = Params->GetStringField(TEXT("component_name")); // Optional
	FString InputType = Params->GetStringField(TEXT("input_type"));
	FString InputEvent = Params->GetStringField(TEXT("input_event"));
	FString Trigger = Params->GetStringField(TEXT("trigger"));
	FString HandlerName = Params->GetStringField(TEXT("handler_name"));

	// Default trigger if not specified
	if (Trigger.IsEmpty())
	{
		Trigger = TEXT("Pressed");
	}

	// Generate default handler name if not provided
	if (HandlerName.IsEmpty())
	{
		if (ComponentName.IsEmpty())
		{
			HandlerName = FString::Printf(TEXT("On%s%s"), *InputEvent, *Trigger);
		}
		else
		{
			HandlerName = FString::Printf(TEXT("On%s%s%s"), *ComponentName, *InputEvent, *Trigger);
		}
	}

	// Call the UMG service to create the input handler
	FString ActualHandlerName;
	bool bResult = UMGService->CreateWidgetInputHandler(
		WidgetName,
		ComponentName,
		InputType,
		InputEvent,
		Trigger,
		HandlerName,
		ActualHandlerName
	);

	if (!bResult)
	{
		FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
			FString::Printf(TEXT("Failed to create input handler '%s' for %s %s in widget '%s'"),
				*HandlerName, *InputType, *InputEvent, *WidgetName));
		return CreateErrorResponse(Error);
	}

	return CreateSuccessResponse(WidgetName, ComponentName, InputType, InputEvent, ActualHandlerName);
}

bool FCreateWidgetInputHandlerCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
	if (!Params.IsValid())
	{
		OutError = TEXT("Invalid parameters object");
		return false;
	}

	// Check required parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	if (WidgetName.IsEmpty())
	{
		OutError = TEXT("Missing or empty 'widget_name' parameter");
		return false;
	}

	FString InputType = Params->GetStringField(TEXT("input_type"));
	if (InputType.IsEmpty())
	{
		OutError = TEXT("Missing or empty 'input_type' parameter");
		return false;
	}

	if (!IsValidInputType(InputType))
	{
		OutError = FString::Printf(TEXT("Invalid input_type '%s'. Valid types: MouseButton, Key, Touch, Focus, Drag"), *InputType);
		return false;
	}

	FString InputEvent = Params->GetStringField(TEXT("input_event"));
	if (InputEvent.IsEmpty())
	{
		OutError = TEXT("Missing or empty 'input_event' parameter");
		return false;
	}

	if (!IsValidInputEvent(InputType, InputEvent))
	{
		OutError = FString::Printf(TEXT("Invalid input_event '%s' for input_type '%s'"), *InputEvent, *InputType);
		return false;
	}

	FString Trigger = Params->GetStringField(TEXT("trigger"));
	if (!Trigger.IsEmpty() && !IsValidTrigger(Trigger))
	{
		OutError = FString::Printf(TEXT("Invalid trigger '%s'. Valid triggers: Pressed, Released, DoubleClick"), *Trigger);
		return false;
	}

	// component_name and handler_name are optional
	return true;
}

bool FCreateWidgetInputHandlerCommand::IsValidInputType(const FString& InputType) const
{
	static TSet<FString> ValidInputTypes = {
		TEXT("MouseButton"),
		TEXT("Key"),
		TEXT("Touch"),
		TEXT("Focus"),
		TEXT("Drag")
	};
	return ValidInputTypes.Contains(InputType);
}

bool FCreateWidgetInputHandlerCommand::IsValidInputEvent(const FString& InputType, const FString& InputEvent) const
{
	if (InputType == TEXT("MouseButton"))
	{
		static TSet<FString> ValidMouseButtons = {
			TEXT("LeftMouseButton"),
			TEXT("RightMouseButton"),
			TEXT("MiddleMouseButton"),
			TEXT("ThumbMouseButton"),
			TEXT("ThumbMouseButton2")
		};
		return ValidMouseButtons.Contains(InputEvent);
	}
	else if (InputType == TEXT("Key"))
	{
		// For keyboard input, we accept any key name - validation happens at runtime
		// Common keys: Enter, Escape, SpaceBar, Tab, A-Z, 0-9, F1-F12, etc.
		return !InputEvent.IsEmpty();
	}
	else if (InputType == TEXT("Touch"))
	{
		static TSet<FString> ValidTouchEvents = {
			TEXT("Touch"),
			TEXT("Pinch"),
			TEXT("Swipe")
		};
		return ValidTouchEvents.Contains(InputEvent);
	}
	else if (InputType == TEXT("Focus"))
	{
		static TSet<FString> ValidFocusEvents = {
			TEXT("FocusReceived"),
			TEXT("FocusLost")
		};
		return ValidFocusEvents.Contains(InputEvent);
	}
	else if (InputType == TEXT("Drag"))
	{
		static TSet<FString> ValidDragEvents = {
			TEXT("DragDetected"),
			TEXT("DragEnter"),
			TEXT("DragLeave"),
			TEXT("DragOver"),
			TEXT("Drop")
		};
		return ValidDragEvents.Contains(InputEvent);
	}

	return false;
}

bool FCreateWidgetInputHandlerCommand::IsValidTrigger(const FString& Trigger) const
{
	static TSet<FString> ValidTriggers = {
		TEXT("Pressed"),
		TEXT("Released"),
		TEXT("DoubleClick")
	};
	return ValidTriggers.Contains(Trigger);
}

TSharedPtr<FJsonObject> FCreateWidgetInputHandlerCommand::CreateSuccessResponse(
	const FString& WidgetName,
	const FString& ComponentName,
	const FString& InputType,
	const FString& InputEvent,
	const FString& HandlerName) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("widget_name"), WidgetName);
	if (!ComponentName.IsEmpty())
	{
		ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
	}
	ResponseObj->SetStringField(TEXT("input_type"), InputType);
	ResponseObj->SetStringField(TEXT("input_event"), InputEvent);
	ResponseObj->SetStringField(TEXT("handler_name"), HandlerName);
	ResponseObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Successfully created input handler '%s' for %s %s"),
			*HandlerName, *InputType, *InputEvent));

	return ResponseObj;
}

TSharedPtr<FJsonObject> FCreateWidgetInputHandlerCommand::CreateErrorResponse(const FMCPError& Error) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
	ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
	ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
	ResponseObj->SetNumberField(TEXT("error_type"), static_cast<int32>(Error.ErrorType));

	return ResponseObj;
}
