#include "Commands/UMG/GetWidgetComponentDetailsCommand.h"
#include "Services/UMG/IUMGService.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Widget types
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"

// Slot types
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/GridSlot.h"
#include "Components/UniformGridSlot.h"
#include "Components/ScaleBoxSlot.h"

// Brush
#include "Styling/SlateBrush.h"

DEFINE_LOG_CATEGORY_STATIC(LogGetWidgetComponentDetails, Log, All);

FGetWidgetComponentDetailsCommand::FGetWidgetComponentDetailsCommand(TSharedPtr<IUMGService> InUMGService)
	: UMGService(InUMGService)
{
}

FString FGetWidgetComponentDetailsCommand::GetCommandName() const
{
	return TEXT("get_widget_component_details");
}

FString FGetWidgetComponentDetailsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(TEXT("Invalid JSON parameters"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	FString ValidationError;
	if (!ValidateParamsInternal(JsonObject, ValidationError))
	{
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(ValidationError);
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

bool FGetWidgetComponentDetailsCommand::ValidateParams(const FString& Parameters) const
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

bool FGetWidgetComponentDetailsCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
	if (!Params.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	if (!Params->HasField(TEXT("widget_name")) || Params->GetStringField(TEXT("widget_name")).IsEmpty())
	{
		OutError = TEXT("Missing required parameter: widget_name");
		return false;
	}

	if (!Params->HasField(TEXT("component_name")) || Params->GetStringField(TEXT("component_name")).IsEmpty())
	{
		OutError = TEXT("Missing required parameter: component_name");
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FGetWidgetComponentDetailsCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString ComponentName = Params->GetStringField(TEXT("component_name"));
	FString WidgetPath = Params->HasField(TEXT("widget_path")) ? Params->GetStringField(TEXT("widget_path")) : TEXT("");

	// Find widget blueprint
	UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(WidgetName, WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget blueprint '%s' not found"), *WidgetName));
	}

	if (!WidgetBP->WidgetTree)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget blueprint '%s' has no widget tree"), *WidgetName));
	}

	// Find component
	UWidget* Widget = FindWidgetInTree(WidgetBP, ComponentName);
	if (!Widget)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Component '%s' not found in widget '%s'"), *ComponentName, *WidgetName));
	}

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("component_name"), ComponentName);

	// Common properties
	BuildCommonProperties(Widget, ResultObj);

	// Type-specific properties
	if (UImage* Image = Cast<UImage>(Widget))
	{
		BuildImageProperties(Image, ResultObj);
	}
	else if (USizeBox* SizeBox = Cast<USizeBox>(Widget))
	{
		BuildSizeBoxProperties(SizeBox, ResultObj);
	}
	else if (UProgressBar* ProgressBar = Cast<UProgressBar>(Widget))
	{
		BuildProgressBarProperties(ProgressBar, ResultObj);
	}
	else if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		BuildTextBlockProperties(TextBlock, ResultObj);
	}
	else if (UBorder* Border = Cast<UBorder>(Widget))
	{
		BuildBorderProperties(Border, ResultObj);
	}
	else if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		ResultObj->SetStringField(TEXT("user_widget_class"), UserWidget->GetClass()->GetPathName());
	}

	return CreateSuccessResponse(ResultObj);
}

UWidgetBlueprint* FGetWidgetComponentDetailsCommand::FindWidgetBlueprint(const FString& WidgetName, const FString& WidgetPath) const
{
	// Try direct path first if provided
	if (!WidgetPath.IsEmpty())
	{
		FString FullPath = WidgetPath / WidgetName + TEXT(".") + WidgetName;
		UObject* Asset = UEditorAssetLibrary::LoadAsset(FullPath);
		if (Asset)
		{
			UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Asset);
			if (WBP) return WBP;
		}
	}

	// Try full path if widget_name looks like a path
	if (WidgetName.StartsWith(TEXT("/Game/")) || WidgetName.StartsWith(TEXT("/Script/")))
	{
		FString Path = WidgetName;
		if (!Path.Contains(TEXT(".")))
		{
			FString AssetName = FPaths::GetBaseFilename(Path);
			Path = Path + TEXT(".") + AssetName;
		}
		UObject* Asset = UEditorAssetLibrary::LoadAsset(Path);
		if (Asset)
		{
			UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Asset);
			if (WBP) return WBP;
		}
	}

	// Search asset registry by name
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	FString SearchName = FPaths::GetBaseFilename(WidgetName);
	if (SearchName.IsEmpty()) SearchName = WidgetName;

	for (const FAssetData& Asset : AssetData)
	{
		if (Asset.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
		{
			UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(Asset.GetObjectPathString());
			if (LoadedAsset)
			{
				UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(LoadedAsset);
				if (WBP) return WBP;
			}
		}
	}

	return nullptr;
}

UWidget* FGetWidgetComponentDetailsCommand::FindWidgetInTree(UWidgetBlueprint* WidgetBP, const FString& ComponentName) const
{
	if (!WidgetBP || !WidgetBP->WidgetTree) return nullptr;

	UWidget* FoundWidget = nullptr;
	WidgetBP->WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		if (Widget && Widget->GetName().Equals(ComponentName, ESearchCase::IgnoreCase))
		{
			FoundWidget = Widget;
		}
	});

	return FoundWidget;
}

void FGetWidgetComponentDetailsCommand::BuildCommonProperties(UWidget* Widget, TSharedPtr<FJsonObject>& OutObj) const
{
	OutObj->SetStringField(TEXT("name"), Widget->GetName());
	OutObj->SetStringField(TEXT("type"), Widget->GetClass()->GetName());

	// Visibility
	ESlateVisibility Vis = Widget->GetVisibility();
	FString VisStr;
	switch (Vis)
	{
		case ESlateVisibility::Visible: VisStr = TEXT("Visible"); break;
		case ESlateVisibility::Collapsed: VisStr = TEXT("Collapsed"); break;
		case ESlateVisibility::Hidden: VisStr = TEXT("Hidden"); break;
		case ESlateVisibility::HitTestInvisible: VisStr = TEXT("HitTestInvisible"); break;
		case ESlateVisibility::SelfHitTestInvisible: VisStr = TEXT("SelfHitTestInvisible"); break;
		default: VisStr = TEXT("Unknown"); break;
	}
	OutObj->SetStringField(TEXT("visibility"), VisStr);
	OutObj->SetBoolField(TEXT("is_visible"), Vis == ESlateVisibility::Visible || Vis == ESlateVisibility::HitTestInvisible || Vis == ESlateVisibility::SelfHitTestInvisible);

	OutObj->SetBoolField(TEXT("is_enabled"), Widget->GetIsEnabled());
	OutObj->SetNumberField(TEXT("render_opacity"), Widget->GetRenderOpacity());

	// Clipping
	EWidgetClipping Clip = Widget->GetClipping();
	FString ClipStr;
	switch (Clip)
	{
		case EWidgetClipping::Inherit: ClipStr = TEXT("Inherit"); break;
		case EWidgetClipping::ClipToBounds: ClipStr = TEXT("ClipToBounds"); break;
		case EWidgetClipping::ClipToBoundsWithoutIntersecting: ClipStr = TEXT("ClipToBoundsWithoutIntersecting"); break;
		case EWidgetClipping::ClipToBoundsAlways: ClipStr = TEXT("ClipToBoundsAlways"); break;
		case EWidgetClipping::OnDemand: ClipStr = TEXT("OnDemand"); break;
		default: ClipStr = TEXT("Unknown"); break;
	}
	OutObj->SetStringField(TEXT("clipping"), ClipStr);

	// Slot properties
	TSharedPtr<FJsonObject> SlotObj = BuildSlotProperties(Widget);
	if (SlotObj.IsValid())
	{
		OutObj->SetObjectField(TEXT("slot"), SlotObj);
	}
}

TSharedPtr<FJsonObject> FGetWidgetComponentDetailsCommand::BuildSlotProperties(UWidget* Widget) const
{
	if (!Widget || !Widget->Slot) return nullptr;

	TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
	UPanelSlot* Slot = Widget->Slot;
	SlotObj->SetStringField(TEXT("slot_type"), Slot->GetClass()->GetName());

	// Canvas Panel Slot
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		FAnchorData Anchors = CanvasSlot->GetLayout();

		TSharedPtr<FJsonObject> AnchorsObj = MakeShared<FJsonObject>();
		AnchorsObj->SetNumberField(TEXT("minimum_x"), Anchors.Anchors.Minimum.X);
		AnchorsObj->SetNumberField(TEXT("minimum_y"), Anchors.Anchors.Minimum.Y);
		AnchorsObj->SetNumberField(TEXT("maximum_x"), Anchors.Anchors.Maximum.X);
		AnchorsObj->SetNumberField(TEXT("maximum_y"), Anchors.Anchors.Maximum.Y);
		SlotObj->SetObjectField(TEXT("anchors"), AnchorsObj);

		TSharedPtr<FJsonObject> OffsetObj = MakeShared<FJsonObject>();
		OffsetObj->SetNumberField(TEXT("left"), Anchors.Offsets.Left);
		OffsetObj->SetNumberField(TEXT("top"), Anchors.Offsets.Top);
		OffsetObj->SetNumberField(TEXT("right"), Anchors.Offsets.Right);
		OffsetObj->SetNumberField(TEXT("bottom"), Anchors.Offsets.Bottom);
		SlotObj->SetObjectField(TEXT("offsets"), OffsetObj);

		TSharedPtr<FJsonObject> AlignObj = MakeShared<FJsonObject>();
		AlignObj->SetNumberField(TEXT("x"), Anchors.Alignment.X);
		AlignObj->SetNumberField(TEXT("y"), Anchors.Alignment.Y);
		SlotObj->SetObjectField(TEXT("alignment"), AlignObj);

		SlotObj->SetBoolField(TEXT("auto_size"), CanvasSlot->GetAutoSize());
		SlotObj->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
	}
	// Overlay Slot
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
	{
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		FMargin Pad = OverlaySlot->GetPadding();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotObj->SetObjectField(TEXT("padding"), PadObj);

		SlotObj->SetNumberField(TEXT("horizontal_alignment"), (int32)OverlaySlot->GetHorizontalAlignment());
		SlotObj->SetNumberField(TEXT("vertical_alignment"), (int32)OverlaySlot->GetVerticalAlignment());
	}
	// Horizontal Box Slot
	else if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(Slot))
	{
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		FMargin Pad = HSlot->GetPadding();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotObj->SetObjectField(TEXT("padding"), PadObj);

		TSharedPtr<FJsonObject> SizeObj = MakeShared<FJsonObject>();
		FSlateChildSize SlotSize = HSlot->GetSize();
		SizeObj->SetNumberField(TEXT("value"), SlotSize.Value);
		SizeObj->SetNumberField(TEXT("size_rule"), (int32)SlotSize.SizeRule);
		SlotObj->SetObjectField(TEXT("size"), SizeObj);

		SlotObj->SetNumberField(TEXT("horizontal_alignment"), (int32)HSlot->GetHorizontalAlignment());
		SlotObj->SetNumberField(TEXT("vertical_alignment"), (int32)HSlot->GetVerticalAlignment());
	}
	// Vertical Box Slot
	else if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(Slot))
	{
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		FMargin Pad = VSlot->GetPadding();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotObj->SetObjectField(TEXT("padding"), PadObj);

		TSharedPtr<FJsonObject> SizeObj = MakeShared<FJsonObject>();
		FSlateChildSize SlotSize = VSlot->GetSize();
		SizeObj->SetNumberField(TEXT("value"), SlotSize.Value);
		SizeObj->SetNumberField(TEXT("size_rule"), (int32)SlotSize.SizeRule);
		SlotObj->SetObjectField(TEXT("size"), SizeObj);

		SlotObj->SetNumberField(TEXT("horizontal_alignment"), (int32)VSlot->GetHorizontalAlignment());
		SlotObj->SetNumberField(TEXT("vertical_alignment"), (int32)VSlot->GetVerticalAlignment());
	}
	// SizeBox Slot
	else if (USizeBoxSlot* SBSlot = Cast<USizeBoxSlot>(Slot))
	{
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		FMargin Pad = SBSlot->GetPadding();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotObj->SetObjectField(TEXT("padding"), PadObj);

		SlotObj->SetNumberField(TEXT("horizontal_alignment"), (int32)SBSlot->GetHorizontalAlignment());
		SlotObj->SetNumberField(TEXT("vertical_alignment"), (int32)SBSlot->GetVerticalAlignment());
	}
	// Border Slot
	else if (UBorderSlot* BSlot = Cast<UBorderSlot>(Slot))
	{
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		FMargin Pad = BSlot->GetPadding();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotObj->SetObjectField(TEXT("padding"), PadObj);

		SlotObj->SetNumberField(TEXT("horizontal_alignment"), (int32)BSlot->GetHorizontalAlignment());
		SlotObj->SetNumberField(TEXT("vertical_alignment"), (int32)BSlot->GetVerticalAlignment());
	}

	return SlotObj;
}

void FGetWidgetComponentDetailsCommand::BuildImageProperties(UImage* Image, TSharedPtr<FJsonObject>& OutObj) const
{
	const FSlateBrush& Brush = Image->GetBrush();
	OutObj->SetObjectField(TEXT("brush"), BrushToJson(Brush));

	FLinearColor ColorAndOpacity = Image->GetColorAndOpacity();
	OutObj->SetObjectField(TEXT("color_and_opacity"), ColorToJson(ColorAndOpacity));
}

void FGetWidgetComponentDetailsCommand::BuildSizeBoxProperties(USizeBox* SizeBox, TSharedPtr<FJsonObject>& OutObj) const
{
	TSharedPtr<FJsonObject> SBObj = MakeShared<FJsonObject>();

	SBObj->SetBoolField(TEXT("width_override_enabled"), SizeBox->bOverride_WidthOverride);
	SBObj->SetBoolField(TEXT("height_override_enabled"), SizeBox->bOverride_HeightOverride);

	if (SizeBox->bOverride_WidthOverride)
	{
		SBObj->SetNumberField(TEXT("width_override"), SizeBox->GetWidthOverride());
	}
	if (SizeBox->bOverride_HeightOverride)
	{
		SBObj->SetNumberField(TEXT("height_override"), SizeBox->GetHeightOverride());
	}

	SBObj->SetBoolField(TEXT("min_desired_width_enabled"), SizeBox->bOverride_MinDesiredWidth);
	SBObj->SetBoolField(TEXT("min_desired_height_enabled"), SizeBox->bOverride_MinDesiredHeight);
	SBObj->SetBoolField(TEXT("max_desired_width_enabled"), SizeBox->bOverride_MaxDesiredWidth);
	SBObj->SetBoolField(TEXT("max_desired_height_enabled"), SizeBox->bOverride_MaxDesiredHeight);

	if (SizeBox->bOverride_MinDesiredWidth)
	{
		SBObj->SetNumberField(TEXT("min_desired_width"), SizeBox->GetMinDesiredWidth());
	}
	if (SizeBox->bOverride_MinDesiredHeight)
	{
		SBObj->SetNumberField(TEXT("min_desired_height"), SizeBox->GetMinDesiredHeight());
	}
	if (SizeBox->bOverride_MaxDesiredWidth)
	{
		SBObj->SetNumberField(TEXT("max_desired_width"), SizeBox->GetMaxDesiredWidth());
	}
	if (SizeBox->bOverride_MaxDesiredHeight)
	{
		SBObj->SetNumberField(TEXT("max_desired_height"), SizeBox->GetMaxDesiredHeight());
	}

	OutObj->SetObjectField(TEXT("size_box"), SBObj);
}

void FGetWidgetComponentDetailsCommand::BuildProgressBarProperties(UProgressBar* ProgressBar, TSharedPtr<FJsonObject>& OutObj) const
{
	TSharedPtr<FJsonObject> PBObj = MakeShared<FJsonObject>();

	PBObj->SetNumberField(TEXT("percent"), ProgressBar->GetPercent());
	PBObj->SetObjectField(TEXT("fill_color_and_opacity"), ColorToJson(ProgressBar->GetFillColorAndOpacity()));

	// Bar fill type
	EProgressBarFillType::Type FillType = ProgressBar->GetBarFillType();
	FString FillTypeStr;
	switch (FillType)
	{
		case EProgressBarFillType::LeftToRight: FillTypeStr = TEXT("LeftToRight"); break;
		case EProgressBarFillType::RightToLeft: FillTypeStr = TEXT("RightToLeft"); break;
		case EProgressBarFillType::FillFromCenter: FillTypeStr = TEXT("FillFromCenter"); break;
		case EProgressBarFillType::FillFromCenterHorizontal: FillTypeStr = TEXT("FillFromCenterHorizontal"); break;
		case EProgressBarFillType::FillFromCenterVertical: FillTypeStr = TEXT("FillFromCenterVertical"); break;
		case EProgressBarFillType::TopToBottom: FillTypeStr = TEXT("TopToBottom"); break;
		case EProgressBarFillType::BottomToTop: FillTypeStr = TEXT("BottomToTop"); break;
		default: FillTypeStr = TEXT("Unknown"); break;
	}
	PBObj->SetStringField(TEXT("bar_fill_type"), FillTypeStr);

	// Bar fill style
	EProgressBarFillStyle::Type FillStyle = ProgressBar->GetBarFillStyle();
	PBObj->SetStringField(TEXT("bar_fill_style"), FillStyle == EProgressBarFillStyle::Mask ? TEXT("Mask") : TEXT("Scale"));

	OutObj->SetObjectField(TEXT("progress_bar"), PBObj);
}

void FGetWidgetComponentDetailsCommand::BuildTextBlockProperties(UTextBlock* TextBlock, TSharedPtr<FJsonObject>& OutObj) const
{
	TSharedPtr<FJsonObject> TBObj = MakeShared<FJsonObject>();

	TBObj->SetStringField(TEXT("text"), TextBlock->GetText().ToString());
	TBObj->SetObjectField(TEXT("color_and_opacity"), ColorToJson(TextBlock->GetColorAndOpacity().GetSpecifiedColor()));

	// Font info
	FSlateFontInfo FontInfo = TextBlock->GetFont();
	TSharedPtr<FJsonObject> FontObj = MakeShared<FJsonObject>();
	FontObj->SetNumberField(TEXT("size"), FontInfo.Size);
	if (FontInfo.FontObject)
	{
		FontObj->SetStringField(TEXT("font_object"), FontInfo.FontObject->GetPathName());
	}
	FontObj->SetStringField(TEXT("typeface"), FontInfo.TypefaceFontName.ToString());
	TBObj->SetObjectField(TEXT("font"), FontObj);

	// Justification
	// Justification is protected in UTextLayoutWidget, access via reflection
	ETextJustify::Type Justify = ETextJustify::Left;
	if (FProperty* JustProp = TextBlock->GetClass()->FindPropertyByName(TEXT("Justification")))
	{
		JustProp->GetValue_InContainer(TextBlock, &Justify);
	}
	FString JustifyStr;
	switch (Justify)
	{
		case ETextJustify::Left: JustifyStr = TEXT("Left"); break;
		case ETextJustify::Center: JustifyStr = TEXT("Center"); break;
		case ETextJustify::Right: JustifyStr = TEXT("Right"); break;
		default: JustifyStr = TEXT("Left"); break;
	}
	TBObj->SetStringField(TEXT("justification"), JustifyStr);

	OutObj->SetObjectField(TEXT("text_block"), TBObj);
}

void FGetWidgetComponentDetailsCommand::BuildBorderProperties(UBorder* Border, TSharedPtr<FJsonObject>& OutObj) const
{
	TSharedPtr<FJsonObject> BObj = MakeShared<FJsonObject>();

	// Brush (background)
	OutObj->SetObjectField(TEXT("brush"), BrushToJson(Border->Background));

	BObj->SetObjectField(TEXT("brush_color"), ColorToJson(Border->GetBrushColor()));
	BObj->SetObjectField(TEXT("content_color_and_opacity"), ColorToJson(Border->GetContentColorAndOpacity()));

	// Padding
	FMargin Pad = Border->GetPadding();
	TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
	PadObj->SetNumberField(TEXT("left"), Pad.Left);
	PadObj->SetNumberField(TEXT("top"), Pad.Top);
	PadObj->SetNumberField(TEXT("right"), Pad.Right);
	PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
	BObj->SetObjectField(TEXT("padding"), PadObj);

	OutObj->SetObjectField(TEXT("border"), BObj);
}

TSharedPtr<FJsonObject> FGetWidgetComponentDetailsCommand::ColorToJson(const FLinearColor& Color) const
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("r"), Color.R);
	Obj->SetNumberField(TEXT("g"), Color.G);
	Obj->SetNumberField(TEXT("b"), Color.B);
	Obj->SetNumberField(TEXT("a"), Color.A);
	return Obj;
}

TSharedPtr<FJsonObject> FGetWidgetComponentDetailsCommand::BrushToJson(const FSlateBrush& Brush) const
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

	// Resource object (material/texture)
	UObject* Resource = Brush.GetResourceObject();
	if (Resource)
	{
		Obj->SetStringField(TEXT("resource_object"), Resource->GetPathName());
	}
	else
	{
		Obj->SetStringField(TEXT("resource_object"), TEXT("None"));
	}

	// Image size
	TSharedPtr<FJsonObject> SizeObj = MakeShared<FJsonObject>();
	SizeObj->SetNumberField(TEXT("x"), Brush.ImageSize.X);
	SizeObj->SetNumberField(TEXT("y"), Brush.ImageSize.Y);
	Obj->SetObjectField(TEXT("image_size"), SizeObj);

	// Draw as
	FString DrawAsStr;
	switch (Brush.DrawAs)
	{
		case ESlateBrushDrawType::NoDrawType: DrawAsStr = TEXT("None"); break;
		case ESlateBrushDrawType::Box: DrawAsStr = TEXT("Box"); break;
		case ESlateBrushDrawType::Border: DrawAsStr = TEXT("Border"); break;
		case ESlateBrushDrawType::Image: DrawAsStr = TEXT("Image"); break;
		case ESlateBrushDrawType::RoundedBox: DrawAsStr = TEXT("RoundedBox"); break;
		default: DrawAsStr = TEXT("Unknown"); break;
	}
	Obj->SetStringField(TEXT("draw_as"), DrawAsStr);

	// Tint
	FLinearColor Tint = Brush.TintColor.GetSpecifiedColor();
	Obj->SetObjectField(TEXT("tint"), ColorToJson(Tint));

	// Tiling
	FString TilingStr;
	switch (Brush.Tiling)
	{
		case ESlateBrushTileType::NoTile: TilingStr = TEXT("NoTile"); break;
		case ESlateBrushTileType::Horizontal: TilingStr = TEXT("Horizontal"); break;
		case ESlateBrushTileType::Vertical: TilingStr = TEXT("Vertical"); break;
		case ESlateBrushTileType::Both: TilingStr = TEXT("Both"); break;
		default: TilingStr = TEXT("Unknown"); break;
	}
	Obj->SetStringField(TEXT("tiling"), TilingStr);

	// Margin
	TSharedPtr<FJsonObject> MarginObj = MakeShared<FJsonObject>();
	MarginObj->SetNumberField(TEXT("left"), Brush.Margin.Left);
	MarginObj->SetNumberField(TEXT("top"), Brush.Margin.Top);
	MarginObj->SetNumberField(TEXT("right"), Brush.Margin.Right);
	MarginObj->SetNumberField(TEXT("bottom"), Brush.Margin.Bottom);
	Obj->SetObjectField(TEXT("margin"), MarginObj);

	return Obj;
}

TSharedPtr<FJsonObject> FGetWidgetComponentDetailsCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	if (Data.IsValid())
	{
		for (auto& Pair : Data->Values)
		{
			ResponseObj->SetField(Pair.Key, Pair.Value);
		}
	}
	return ResponseObj;
}

TSharedPtr<FJsonObject> FGetWidgetComponentDetailsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
	return ResponseObj;
}
