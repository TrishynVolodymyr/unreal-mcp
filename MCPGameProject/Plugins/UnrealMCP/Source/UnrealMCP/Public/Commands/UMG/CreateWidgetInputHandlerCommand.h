#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for creating input event handlers in Widget Blueprints.
 *
 * This command creates event handlers for input events that are not exposed as
 * standard delegates (like OnClicked), such as:
 * - Mouse button events (Left, Right, Middle, Thumb buttons)
 * - Keyboard events on focusable widgets
 * - Touch/gesture events
 * - Focus events
 *
 * It works by:
 * 1. Creating a custom event function in the Widget Blueprint
 * 2. Overriding the appropriate input handler (OnMouseButtonDown, OnKeyDown, etc.)
 * 3. Adding logic to check for the specific input and call the custom event
 *
 * Example usage:
 * {
 *   "widget_name": "InventoryWidget",
 *   "component_name": "ItemSlot",        // Optional - if empty, handles at widget level
 *   "input_type": "MouseButton",         // MouseButton, Key, Touch, Focus
 *   "input_event": "RightMouseButton",   // Specific button/key
 *   "trigger": "Pressed",                // Pressed, Released, DoubleClick
 *   "handler_name": "OnSlotRightClicked" // Function name to create
 * }
 */
class UNREALMCP_API FCreateWidgetInputHandlerCommand : public IUnrealMCPCommand
{
public:
	/**
	 * Constructor
	 * @param InUMGService - Shared pointer to the UMG service for operations
	 */
	explicit FCreateWidgetInputHandlerCommand(TSharedPtr<IUMGService> InUMGService);

	// IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	/** Shared pointer to the UMG service */
	TSharedPtr<IUMGService> UMGService;

	/**
	 * Internal execution with JSON objects
	 * @param Params - JSON parameters
	 * @return JSON response object
	 */
	TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Internal validation with JSON objects
	 * @param Params - JSON parameters
	 * @param OutError - Error message if validation fails
	 * @return true if validation passes
	 */
	bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;

	/**
	 * Create success response JSON object
	 */
	TSharedPtr<FJsonObject> CreateSuccessResponse(
		const FString& WidgetName,
		const FString& ComponentName,
		const FString& InputType,
		const FString& InputEvent,
		const FString& HandlerName) const;

	/**
	 * Create error response JSON object from MCP error
	 */
	TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;

	/**
	 * Validate input type is supported
	 */
	bool IsValidInputType(const FString& InputType) const;

	/**
	 * Validate input event is valid for the given input type
	 */
	bool IsValidInputEvent(const FString& InputType, const FString& InputEvent) const;

	/**
	 * Validate trigger type is supported
	 */
	bool IsValidTrigger(const FString& Trigger) const;
};
