#include "Services/UMG/WidgetLayoutService.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateBrush.h"

// Includes for screenshot capture
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Base64.h"
#include "ImageUtils.h"
#include "Engine/World.h"
#include "Editor.h"

bool FWidgetLayoutService::GetWidgetComponentLayout(UWidgetBlueprint* WidgetBlueprint, TSharedPtr<FJsonObject>& OutLayoutInfo)
{
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService: Widget blueprint is null"));
        return false;
    }

    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService: Widget blueprint '%s' has no widget tree"), *WidgetBlueprint->GetName());
        return false;
    }

    OutLayoutInfo = MakeShareable(new FJsonObject);

    // Get the root widget
    UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget;
    if (!RootWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetLayoutService: Widget blueprint '%s' has no root widget"), *WidgetBlueprint->GetName());
        OutLayoutInfo->SetBoolField(TEXT("success"), true);
        OutLayoutInfo->SetStringField(TEXT("message"), TEXT("Widget has no root widget"));
        return true;
    }

    // Build hierarchical layout information
    TSharedPtr<FJsonObject> HierarchyInfo = BuildWidgetHierarchy(RootWidget);
    if (HierarchyInfo.IsValid())
    {
        OutLayoutInfo->SetObjectField(TEXT("hierarchy"), HierarchyInfo);
        OutLayoutInfo->SetBoolField(TEXT("success"), true);
        OutLayoutInfo->SetStringField(TEXT("message"), TEXT("Successfully retrieved widget component layout"));
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService: Failed to build widget hierarchy for '%s'"), *WidgetBlueprint->GetName());
    return false;
}

TSharedPtr<FJsonObject> FWidgetLayoutService::BuildWidgetHierarchy(UWidget* Widget)
{
    if (!Widget)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> WidgetInfo = MakeShareable(new FJsonObject);

    // Basic widget information
    WidgetInfo->SetStringField(TEXT("name"), Widget->GetName());
    WidgetInfo->SetStringField(TEXT("type"), Widget->GetClass()->GetName());

    // Visibility
    WidgetInfo->SetBoolField(TEXT("is_visible"), Widget->GetVisibility() != ESlateVisibility::Hidden);
    WidgetInfo->SetBoolField(TEXT("is_enabled"), Widget->GetIsEnabled());

    // Get slot properties based on slot type
    TSharedPtr<FJsonObject> SlotProperties = MakeShareable(new FJsonObject);
    if (Widget->Slot)
    {
        FString SlotTypeName = Widget->Slot->GetClass()->GetName();
        SlotProperties->SetStringField(TEXT("slot_type"), SlotTypeName);

        // Handle different slot types
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            // Canvas panel slot properties
            FVector2D Position = CanvasSlot->GetPosition();
            FVector2D Size = CanvasSlot->GetSize();

            TArray<TSharedPtr<FJsonValue>> PositionArray;
            PositionArray.Add(MakeShareable(new FJsonValueNumber(Position.X)));
            PositionArray.Add(MakeShareable(new FJsonValueNumber(Position.Y)));
            SlotProperties->SetArrayField(TEXT("position"), PositionArray);

            TArray<TSharedPtr<FJsonValue>> SizeArray;
            SizeArray.Add(MakeShareable(new FJsonValueNumber(Size.X)));
            SizeArray.Add(MakeShareable(new FJsonValueNumber(Size.Y)));
            SlotProperties->SetArrayField(TEXT("size"), SizeArray);

            SlotProperties->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());

            // Anchors
            FAnchors Anchors = CanvasSlot->GetAnchors();
            TSharedPtr<FJsonObject> AnchorsObj = MakeShareable(new FJsonObject);
            AnchorsObj->SetNumberField(TEXT("min_x"), Anchors.Minimum.X);
            AnchorsObj->SetNumberField(TEXT("min_y"), Anchors.Minimum.Y);
            AnchorsObj->SetNumberField(TEXT("max_x"), Anchors.Maximum.X);
            AnchorsObj->SetNumberField(TEXT("max_y"), Anchors.Maximum.Y);
            SlotProperties->SetObjectField(TEXT("anchors"), AnchorsObj);

            // Alignment
            FVector2D Alignment = CanvasSlot->GetAlignment();
            TArray<TSharedPtr<FJsonValue>> AlignmentArray;
            AlignmentArray.Add(MakeShareable(new FJsonValueNumber(Alignment.X)));
            AlignmentArray.Add(MakeShareable(new FJsonValueNumber(Alignment.Y)));
            SlotProperties->SetArrayField(TEXT("alignment"), AlignmentArray);
        }
        else if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Widget->Slot))
        {
            // Horizontal box slot properties
            FMargin Padding = HBoxSlot->GetPadding();
            TSharedPtr<FJsonObject> PaddingObj = MakeShareable(new FJsonObject);
            PaddingObj->SetNumberField(TEXT("left"), Padding.Left);
            PaddingObj->SetNumberField(TEXT("top"), Padding.Top);
            PaddingObj->SetNumberField(TEXT("right"), Padding.Right);
            PaddingObj->SetNumberField(TEXT("bottom"), Padding.Bottom);
            SlotProperties->SetObjectField(TEXT("padding"), PaddingObj);

            SlotProperties->SetStringField(TEXT("horizontal_alignment"),
                UEnum::GetValueAsString(HBoxSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"),
                UEnum::GetValueAsString(HBoxSlot->GetVerticalAlignment()));

            FSlateChildSize ChildSize = HBoxSlot->GetSize();
            SlotProperties->SetStringField(TEXT("size_rule"), UEnum::GetValueAsString(ChildSize.SizeRule));
            SlotProperties->SetNumberField(TEXT("size_value"), ChildSize.Value);
        }
        else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Widget->Slot))
        {
            // Vertical box slot properties
            FMargin Padding = VBoxSlot->GetPadding();
            TSharedPtr<FJsonObject> PaddingObj = MakeShareable(new FJsonObject);
            PaddingObj->SetNumberField(TEXT("left"), Padding.Left);
            PaddingObj->SetNumberField(TEXT("top"), Padding.Top);
            PaddingObj->SetNumberField(TEXT("right"), Padding.Right);
            PaddingObj->SetNumberField(TEXT("bottom"), Padding.Bottom);
            SlotProperties->SetObjectField(TEXT("padding"), PaddingObj);

            SlotProperties->SetStringField(TEXT("horizontal_alignment"),
                UEnum::GetValueAsString(VBoxSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"),
                UEnum::GetValueAsString(VBoxSlot->GetVerticalAlignment()));

            FSlateChildSize ChildSize = VBoxSlot->GetSize();
            SlotProperties->SetStringField(TEXT("size_rule"), UEnum::GetValueAsString(ChildSize.SizeRule));
            SlotProperties->SetNumberField(TEXT("size_value"), ChildSize.Value);
        }
    }

    WidgetInfo->SetObjectField(TEXT("slot_properties"), SlotProperties);

    // Widget-specific properties
    if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        TSharedPtr<FJsonObject> TextProps = MakeShareable(new FJsonObject);
        TextProps->SetStringField(TEXT("text"), TextBlock->GetText().ToString());

        const FSlateFontInfo& Font = TextBlock->GetFont();
        TextProps->SetNumberField(TEXT("font_size"), Font.Size);

        FSlateColor SlateColor = TextBlock->GetColorAndOpacity();
        FLinearColor Color = SlateColor.GetSpecifiedColor();
        TextProps->SetStringField(TEXT("color"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(Color.R * 255), (int)(Color.G * 255), (int)(Color.B * 255), Color.A));

        // Note: Justification property exists on UTextLayoutWidget base class but is protected
        // and has no public getter in UE 5.7, so we cannot access it for TextBlock

        WidgetInfo->SetObjectField(TEXT("text_properties"), TextProps);
    }
    else if (UImage* Image = Cast<UImage>(Widget))
    {
        TSharedPtr<FJsonObject> ImageProps = MakeShareable(new FJsonObject);

        FSlateBrush Brush = Image->GetBrush();
        if (Brush.GetResourceObject())
        {
            ImageProps->SetStringField(TEXT("texture"), Brush.GetResourceObject()->GetPathName());
        }

        FSlateColor SlateTint = Image->GetColorAndOpacity();
        FLinearColor Tint = SlateTint.GetSpecifiedColor();
        ImageProps->SetStringField(TEXT("tint"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(Tint.R * 255), (int)(Tint.G * 255), (int)(Tint.B * 255), Tint.A));

        WidgetInfo->SetObjectField(TEXT("image_properties"), ImageProps);
    }
    else if (UButton* Button = Cast<UButton>(Widget))
    {
        TSharedPtr<FJsonObject> ButtonProps = MakeShareable(new FJsonObject);

        // Button style (colors)
        FButtonStyle Style = Button->GetStyle();
        FLinearColor NormalColor = Style.Normal.TintColor.GetSpecifiedColor();
        FLinearColor HoverColor = Style.Hovered.TintColor.GetSpecifiedColor();
        FLinearColor PressedColor = Style.Pressed.TintColor.GetSpecifiedColor();

        ButtonProps->SetStringField(TEXT("normal_color"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(NormalColor.R * 255), (int)(NormalColor.G * 255), (int)(NormalColor.B * 255), NormalColor.A));
        ButtonProps->SetStringField(TEXT("hover_color"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(HoverColor.R * 255), (int)(HoverColor.G * 255), (int)(HoverColor.B * 255), HoverColor.A));
        ButtonProps->SetStringField(TEXT("pressed_color"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(PressedColor.R * 255), (int)(PressedColor.G * 255), (int)(PressedColor.B * 255), PressedColor.A));

        ButtonProps->SetBoolField(TEXT("is_focusable"), Button->GetIsFocusable());

        WidgetInfo->SetObjectField(TEXT("button_properties"), ButtonProps);
    }
    else if (UBorder* Border = Cast<UBorder>(Widget))
    {
        TSharedPtr<FJsonObject> BorderProps = MakeShareable(new FJsonObject);

        // Border brush color
        FLinearColor BrushColor = Border->GetBrushColor();
        BorderProps->SetStringField(TEXT("brush_color"), FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
            (int)(BrushColor.R * 255), (int)(BrushColor.G * 255), (int)(BrushColor.B * 255), BrushColor.A));

        // Padding
        FMargin Padding = Border->GetPadding();
        TSharedPtr<FJsonObject> PaddingObj = MakeShareable(new FJsonObject);
        PaddingObj->SetNumberField(TEXT("left"), Padding.Left);
        PaddingObj->SetNumberField(TEXT("top"), Padding.Top);
        PaddingObj->SetNumberField(TEXT("right"), Padding.Right);
        PaddingObj->SetNumberField(TEXT("bottom"), Padding.Bottom);
        BorderProps->SetObjectField(TEXT("padding"), PaddingObj);

        BorderProps->SetStringField(TEXT("horizontal_alignment"),
            UEnum::GetValueAsString(Border->GetHorizontalAlignment()));
        BorderProps->SetStringField(TEXT("vertical_alignment"),
            UEnum::GetValueAsString(Border->GetVerticalAlignment()));

        WidgetInfo->SetObjectField(TEXT("border_properties"), BorderProps);
    }

    // Handle child widgets for panel widgets
    TArray<TSharedPtr<FJsonValue>> ChildrenArray;
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
    {
        for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
        {
            UWidget* ChildWidget = PanelWidget->GetChildAt(i);
            if (ChildWidget)
            {
                TSharedPtr<FJsonObject> ChildInfo = BuildWidgetHierarchy(ChildWidget);
                if (ChildInfo.IsValid())
                {
                    ChildrenArray.Add(MakeShareable(new FJsonValueObject(ChildInfo)));
                }
            }
        }
    }

    WidgetInfo->SetArrayField(TEXT("children"), ChildrenArray);

    return WidgetInfo;
}

bool FWidgetLayoutService::CaptureWidgetScreenshot(UWidgetBlueprint* WidgetBlueprint, int32 Width, int32 Height,
                                                   const FString& Format, TSharedPtr<FJsonObject>& OutScreenshotData)
{
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Widget blueprint is null"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Capturing screenshot for '%s' at %dx%d"),
           *WidgetBlueprint->GetName(), Width, Height);

    // Verify the widget has a generated class
    if (!WidgetBlueprint->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Widget blueprint '%s' has no generated class"), *WidgetBlueprint->GetName());
        return false;
    }

    // Get the editor world
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - No editor world available"));
        return false;
    }

    // Create a preview instance of the widget
    UClass* GeneratedClass = WidgetBlueprint->GeneratedClass;
    if (!GeneratedClass || !GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Invalid or incompatible generated class"));
        return false;
    }
    UUserWidget* PreviewWidget = CreateWidget<UUserWidget>(EditorWorld, GeneratedClass);
    if (!PreviewWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Failed to create widget preview instance"));
        return false;
    }

    // Get the Slate widget
    TSharedPtr<SWidget> SlateWidget = PreviewWidget->TakeWidget();
    if (!SlateWidget.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Failed to get Slate widget"));
        PreviewWidget->RemoveFromParent();
        PreviewWidget->MarkAsGarbage();
        return false;
    }

    // Create render target
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, true);
    RenderTarget->UpdateResourceImmediate(true);

    // Render widget to texture
    FWidgetRenderer WidgetRenderer(true, false);
    WidgetRenderer.DrawWidget(
        RenderTarget,
        SlateWidget.ToSharedRef(),
        FVector2D(Width, Height),
        0.0f);

    // Flush rendering commands to ensure texture is ready
    FlushRenderingCommands();

    // Read pixels from render target
    FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
    if (!RTResource)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Failed to get render target resource"));
        PreviewWidget->RemoveFromParent();
        PreviewWidget->MarkAsGarbage();
        return false;
    }

    TArray<FColor> OutPixels;
    if (!RTResource->ReadPixels(OutPixels))
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Failed to read pixels from render target"));
        PreviewWidget->RemoveFromParent();
        PreviewWidget->MarkAsGarbage();
        return false;
    }

    // Compress to PNG or JPEG
    TArray<uint8> CompressedImage;
    FString ActualFormat = Format.ToLower();

    if (ActualFormat == TEXT("jpg") || ActualFormat == TEXT("jpeg"))
    {
        // JPEG compression - use thumbnail compression for JPEG
        FImageUtils::ThumbnailCompressImageArray(Width, Height, OutPixels, CompressedImage);
    }
    else
    {
        // PNG compression (default)
        ActualFormat = TEXT("png");
        TArray64<uint8> CompressedImage64;
        FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(OutPixels), CompressedImage64);
        // Convert from TArray64 to TArray
        CompressedImage.Append(CompressedImage64.GetData(), CompressedImage64.Num());
    }

    // Encode as base64
    FString Base64Image = FBase64::Encode(CompressedImage);

    // Build response
    OutScreenshotData = MakeShareable(new FJsonObject);
    OutScreenshotData->SetBoolField(TEXT("success"), true);
    OutScreenshotData->SetStringField(TEXT("image_base64"), Base64Image);
    OutScreenshotData->SetNumberField(TEXT("width"), Width);
    OutScreenshotData->SetNumberField(TEXT("height"), Height);
    OutScreenshotData->SetStringField(TEXT("format"), ActualFormat);
    OutScreenshotData->SetNumberField(TEXT("image_size_bytes"), CompressedImage.Num());

    // Clean up
    PreviewWidget->RemoveFromParent();
    PreviewWidget->MarkAsGarbage();
    RenderTarget->MarkAsGarbage();

    UE_LOG(LogTemp, Log, TEXT("WidgetLayoutService::CaptureWidgetScreenshot - Screenshot captured successfully, %d bytes"),
           CompressedImage.Num());

    return true;
}
