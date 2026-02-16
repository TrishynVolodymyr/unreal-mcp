#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IUMGService;

/**
 * Command to retrieve detailed properties for a specific widget component.
 * Returns type-specific properties (Brush, SizeBox overrides, slot details, etc.)
 * that get_widget_blueprint_metadata doesn't provide.
 *
 * Parameters:
 * - "widget_name" (required) - Widget blueprint name
 * - "widget_path" (optional) - Content browser path
 * - "component_name" (required) - Name of the component to inspect
 */
class UNREALMCP_API FGetWidgetComponentDetailsCommand : public IUnrealMCPCommand
{
public:
	FGetWidgetComponentDetailsCommand(TSharedPtr<IUMGService> InUMGService);
	virtual ~FGetWidgetComponentDetailsCommand() = default;

	virtual FString GetCommandName() const override;
	virtual FString Execute(const FString& Parameters) override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	TSharedPtr<IUMGService> UMGService;

	TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);
	bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;

	/** Find widget blueprint using asset registry search */
	class UWidgetBlueprint* FindWidgetBlueprint(const FString& WidgetName, const FString& WidgetPath) const;

	/** Find a widget by name in the widget tree */
	class UWidget* FindWidgetInTree(class UWidgetBlueprint* WidgetBP, const FString& ComponentName) const;

	/** Build common properties for any widget */
	void BuildCommonProperties(class UWidget* Widget, TSharedPtr<FJsonObject>& OutObj) const;

	/** Build slot properties */
	TSharedPtr<FJsonObject> BuildSlotProperties(class UWidget* Widget) const;

	/** Build type-specific properties */
	void BuildImageProperties(class UImage* Image, TSharedPtr<FJsonObject>& OutObj) const;
	void BuildSizeBoxProperties(class USizeBox* SizeBox, TSharedPtr<FJsonObject>& OutObj) const;
	void BuildProgressBarProperties(class UProgressBar* ProgressBar, TSharedPtr<FJsonObject>& OutObj) const;
	void BuildTextBlockProperties(class UTextBlock* TextBlock, TSharedPtr<FJsonObject>& OutObj) const;
	void BuildBorderProperties(class UBorder* Border, TSharedPtr<FJsonObject>& OutObj) const;
	void BuildButtonProperties(class UButton* Button, TSharedPtr<FJsonObject>& OutObj) const;

	/** Helper to serialize a linear color */
	TSharedPtr<FJsonObject> ColorToJson(const FLinearColor& Color) const;

	/** Helper to serialize an FSlateBrush */
	TSharedPtr<FJsonObject> BrushToJson(const FSlateBrush& Brush) const;

	TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data) const;
	TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage) const;
};
