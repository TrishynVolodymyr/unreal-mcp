// UMGSlotPropertyService.cpp - Widget slot property operations
// SetSlotProperty implementation split from UMGService.cpp

#include "Services/UMG/UMGService.h"
#include "Components/Widget.h"
#include "Components/PanelSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Dom/JsonValue.h"

bool FUMGService::SetSlotProperty(UWidget* Widget, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) const
{
    if (!Widget || !Widget->Slot)
    {
        OutError = TEXT("Widget has no slot");
        return false;
    }

    UPanelSlot* Slot = Widget->Slot;

    // Try HorizontalBoxSlot first
    if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
    {
        if (PropertyName.Equals(TEXT("SizeRule"), ESearchCase::IgnoreCase) ||
            PropertyName.Equals(TEXT("Size"), ESearchCase::IgnoreCase))
        {
            FString SizeRuleStr = PropertyValue->AsString();
            FSlateChildSize ChildSize = HBoxSlot->GetSize();

            if (SizeRuleStr.Equals(TEXT("Auto"), ESearchCase::IgnoreCase) ||
                SizeRuleStr.Equals(TEXT("ESlateSizeRule::Auto"), ESearchCase::IgnoreCase))
            {
                ChildSize.SizeRule = ESlateSizeRule::Automatic;
            }
            else if (SizeRuleStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase) ||
                     SizeRuleStr.Equals(TEXT("ESlateSizeRule::Fill"), ESearchCase::IgnoreCase))
            {
                ChildSize.SizeRule = ESlateSizeRule::Fill;
            }
            else
            {
                OutError = FString::Printf(TEXT("Unknown SizeRule value: %s"), *SizeRuleStr);
                return false;
            }

            HBoxSlot->SetSize(ChildSize);
            UE_LOG(LogTemp, Log, TEXT("UMGService: Set HorizontalBoxSlot.SizeRule to %s"), *SizeRuleStr);
            return true;
        }
        else if (PropertyName.Equals(TEXT("FillSpanWhenLessThan"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("SizeValue"), ESearchCase::IgnoreCase))
        {
            double Value = 0.0;
            if (PropertyValue->TryGetNumber(Value))
            {
                FSlateChildSize ChildSize = HBoxSlot->GetSize();
                ChildSize.Value = static_cast<float>(Value);
                HBoxSlot->SetSize(ChildSize);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set HorizontalBoxSlot.SizeValue to %f"), Value);
                return true;
            }
            OutError = TEXT("SizeValue must be a number");
            return false;
        }
        else if (PropertyName.Equals(TEXT("VerticalAlignment"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("VAlign"), ESearchCase::IgnoreCase))
        {
            FString AlignStr = PropertyValue->AsString();
            EVerticalAlignment VAlign = EVerticalAlignment::VAlign_Fill;

            if (AlignStr.Contains(TEXT("Top")))
            {
                VAlign = EVerticalAlignment::VAlign_Top;
            }
            else if (AlignStr.Contains(TEXT("Center")))
            {
                VAlign = EVerticalAlignment::VAlign_Center;
            }
            else if (AlignStr.Contains(TEXT("Bottom")))
            {
                VAlign = EVerticalAlignment::VAlign_Bottom;
            }
            else if (AlignStr.Contains(TEXT("Fill")))
            {
                VAlign = EVerticalAlignment::VAlign_Fill;
            }
            else
            {
                OutError = FString::Printf(TEXT("Unknown VerticalAlignment value: %s"), *AlignStr);
                return false;
            }

            HBoxSlot->SetVerticalAlignment(VAlign);
            UE_LOG(LogTemp, Log, TEXT("UMGService: Set HorizontalBoxSlot.VerticalAlignment to %s"), *AlignStr);
            return true;
        }
        else if (PropertyName.Equals(TEXT("HorizontalAlignment"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("HAlign"), ESearchCase::IgnoreCase))
        {
            FString AlignStr = PropertyValue->AsString();
            EHorizontalAlignment HAlign = EHorizontalAlignment::HAlign_Fill;

            if (AlignStr.Contains(TEXT("Left")))
            {
                HAlign = EHorizontalAlignment::HAlign_Left;
            }
            else if (AlignStr.Contains(TEXT("Center")))
            {
                HAlign = EHorizontalAlignment::HAlign_Center;
            }
            else if (AlignStr.Contains(TEXT("Right")))
            {
                HAlign = EHorizontalAlignment::HAlign_Right;
            }
            else if (AlignStr.Contains(TEXT("Fill")))
            {
                HAlign = EHorizontalAlignment::HAlign_Fill;
            }
            else
            {
                OutError = FString::Printf(TEXT("Unknown HorizontalAlignment value: %s"), *AlignStr);
                return false;
            }

            HBoxSlot->SetHorizontalAlignment(HAlign);
            UE_LOG(LogTemp, Log, TEXT("UMGService: Set HorizontalBoxSlot.HorizontalAlignment to %s"), *AlignStr);
            return true;
        }
        else if (PropertyName.Equals(TEXT("Padding"), ESearchCase::IgnoreCase))
        {
            const TArray<TSharedPtr<FJsonValue>>* PaddingArray;
            if (PropertyValue->TryGetArray(PaddingArray) && PaddingArray->Num() == 4)
            {
                FMargin Padding(
                    static_cast<float>((*PaddingArray)[0]->AsNumber()),
                    static_cast<float>((*PaddingArray)[1]->AsNumber()),
                    static_cast<float>((*PaddingArray)[2]->AsNumber()),
                    static_cast<float>((*PaddingArray)[3]->AsNumber())
                );
                HBoxSlot->SetPadding(Padding);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set HorizontalBoxSlot.Padding"));
                return true;
            }
            // Try single value for uniform padding
            double UniformPadding = 0.0;
            if (PropertyValue->TryGetNumber(UniformPadding))
            {
                HBoxSlot->SetPadding(FMargin(static_cast<float>(UniformPadding)));
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set HorizontalBoxSlot.Padding (uniform) to %f"), UniformPadding);
                return true;
            }
            OutError = TEXT("Padding must be array [left, top, right, bottom] or single number");
            return false;
        }
    }
    // Try VerticalBoxSlot
    else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Slot))
    {
        if (PropertyName.Equals(TEXT("SizeRule"), ESearchCase::IgnoreCase) ||
            PropertyName.Equals(TEXT("Size"), ESearchCase::IgnoreCase))
        {
            FString SizeRuleStr = PropertyValue->AsString();
            FSlateChildSize ChildSize = VBoxSlot->GetSize();

            if (SizeRuleStr.Equals(TEXT("Auto"), ESearchCase::IgnoreCase) ||
                SizeRuleStr.Equals(TEXT("ESlateSizeRule::Auto"), ESearchCase::IgnoreCase))
            {
                ChildSize.SizeRule = ESlateSizeRule::Automatic;
            }
            else if (SizeRuleStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase) ||
                     SizeRuleStr.Equals(TEXT("ESlateSizeRule::Fill"), ESearchCase::IgnoreCase))
            {
                ChildSize.SizeRule = ESlateSizeRule::Fill;
            }
            else
            {
                OutError = FString::Printf(TEXT("Unknown SizeRule value: %s"), *SizeRuleStr);
                return false;
            }

            VBoxSlot->SetSize(ChildSize);
            UE_LOG(LogTemp, Log, TEXT("UMGService: Set VerticalBoxSlot.SizeRule to %s"), *SizeRuleStr);
            return true;
        }
        else if (PropertyName.Equals(TEXT("FillSpanWhenLessThan"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("SizeValue"), ESearchCase::IgnoreCase))
        {
            double Value = 0.0;
            if (PropertyValue->TryGetNumber(Value))
            {
                FSlateChildSize ChildSize = VBoxSlot->GetSize();
                ChildSize.Value = static_cast<float>(Value);
                VBoxSlot->SetSize(ChildSize);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set VerticalBoxSlot.SizeValue to %f"), Value);
                return true;
            }
            OutError = TEXT("SizeValue must be a number");
            return false;
        }
        else if (PropertyName.Equals(TEXT("VerticalAlignment"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("VAlign"), ESearchCase::IgnoreCase))
        {
            FString AlignStr = PropertyValue->AsString();
            EVerticalAlignment VAlign = EVerticalAlignment::VAlign_Fill;

            if (AlignStr.Contains(TEXT("Top")))
            {
                VAlign = EVerticalAlignment::VAlign_Top;
            }
            else if (AlignStr.Contains(TEXT("Center")))
            {
                VAlign = EVerticalAlignment::VAlign_Center;
            }
            else if (AlignStr.Contains(TEXT("Bottom")))
            {
                VAlign = EVerticalAlignment::VAlign_Bottom;
            }
            else if (AlignStr.Contains(TEXT("Fill")))
            {
                VAlign = EVerticalAlignment::VAlign_Fill;
            }
            else
            {
                OutError = FString::Printf(TEXT("Unknown VerticalAlignment value: %s"), *AlignStr);
                return false;
            }

            VBoxSlot->SetVerticalAlignment(VAlign);
            UE_LOG(LogTemp, Log, TEXT("UMGService: Set VerticalBoxSlot.VerticalAlignment to %s"), *AlignStr);
            return true;
        }
        else if (PropertyName.Equals(TEXT("HorizontalAlignment"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("HAlign"), ESearchCase::IgnoreCase))
        {
            FString AlignStr = PropertyValue->AsString();
            EHorizontalAlignment HAlign = EHorizontalAlignment::HAlign_Fill;

            if (AlignStr.Contains(TEXT("Left")))
            {
                HAlign = EHorizontalAlignment::HAlign_Left;
            }
            else if (AlignStr.Contains(TEXT("Center")))
            {
                HAlign = EHorizontalAlignment::HAlign_Center;
            }
            else if (AlignStr.Contains(TEXT("Right")))
            {
                HAlign = EHorizontalAlignment::HAlign_Right;
            }
            else if (AlignStr.Contains(TEXT("Fill")))
            {
                HAlign = EHorizontalAlignment::HAlign_Fill;
            }
            else
            {
                OutError = FString::Printf(TEXT("Unknown HorizontalAlignment value: %s"), *AlignStr);
                return false;
            }

            VBoxSlot->SetHorizontalAlignment(HAlign);
            UE_LOG(LogTemp, Log, TEXT("UMGService: Set VerticalBoxSlot.HorizontalAlignment to %s"), *AlignStr);
            return true;
        }
        else if (PropertyName.Equals(TEXT("Padding"), ESearchCase::IgnoreCase))
        {
            const TArray<TSharedPtr<FJsonValue>>* PaddingArray;
            if (PropertyValue->TryGetArray(PaddingArray) && PaddingArray->Num() == 4)
            {
                FMargin Padding(
                    static_cast<float>((*PaddingArray)[0]->AsNumber()),
                    static_cast<float>((*PaddingArray)[1]->AsNumber()),
                    static_cast<float>((*PaddingArray)[2]->AsNumber()),
                    static_cast<float>((*PaddingArray)[3]->AsNumber())
                );
                VBoxSlot->SetPadding(Padding);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set VerticalBoxSlot.Padding"));
                return true;
            }
            // Try single value for uniform padding
            double UniformPadding = 0.0;
            if (PropertyValue->TryGetNumber(UniformPadding))
            {
                VBoxSlot->SetPadding(FMargin(static_cast<float>(UniformPadding)));
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set VerticalBoxSlot.Padding (uniform) to %f"), UniformPadding);
                return true;
            }
            OutError = TEXT("Padding must be array [left, top, right, bottom] or single number");
            return false;
        }
    }
    // Try CanvasPanelSlot
    else if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        if (PropertyName.Equals(TEXT("Anchors"), ESearchCase::IgnoreCase))
        {
            // Parse FAnchors from JSON: {"Minimum": {"X": 0, "Y": 0}, "Maximum": {"X": 1, "Y": 1}}
            const TSharedPtr<FJsonObject>* AnchorsObject;
            if (PropertyValue->TryGetObject(AnchorsObject))
            {
                FAnchors Anchors;

                const TSharedPtr<FJsonObject>* MinObject;
                if ((*AnchorsObject)->TryGetObjectField(TEXT("Minimum"), MinObject))
                {
                    (*MinObject)->TryGetNumberField(TEXT("X"), Anchors.Minimum.X);
                    (*MinObject)->TryGetNumberField(TEXT("Y"), Anchors.Minimum.Y);
                }

                const TSharedPtr<FJsonObject>* MaxObject;
                if ((*AnchorsObject)->TryGetObjectField(TEXT("Maximum"), MaxObject))
                {
                    (*MaxObject)->TryGetNumberField(TEXT("X"), Anchors.Maximum.X);
                    (*MaxObject)->TryGetNumberField(TEXT("Y"), Anchors.Maximum.Y);
                }

                CanvasSlot->SetAnchors(Anchors);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.Anchors Min(%.2f,%.2f) Max(%.2f,%.2f)"),
                       Anchors.Minimum.X, Anchors.Minimum.Y, Anchors.Maximum.X, Anchors.Maximum.Y);
                return true;
            }
            OutError = TEXT("Anchors must be object with Minimum and Maximum fields");
            return false;
        }
        else if (PropertyName.Equals(TEXT("Offsets"), ESearchCase::IgnoreCase))
        {
            // Parse offsets: {"Left": 0, "Top": 0, "Right": 0, "Bottom": 0}
            const TSharedPtr<FJsonObject>* OffsetsObject;
            if (PropertyValue->TryGetObject(OffsetsObject))
            {
                double Left = 0, Top = 0, Right = 0, Bottom = 0;
                (*OffsetsObject)->TryGetNumberField(TEXT("Left"), Left);
                (*OffsetsObject)->TryGetNumberField(TEXT("Top"), Top);
                (*OffsetsObject)->TryGetNumberField(TEXT("Right"), Right);
                (*OffsetsObject)->TryGetNumberField(TEXT("Bottom"), Bottom);

                FMargin Offsets(static_cast<float>(Left), static_cast<float>(Top),
                               static_cast<float>(Right), static_cast<float>(Bottom));
                CanvasSlot->SetOffsets(Offsets);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.Offsets L:%.1f T:%.1f R:%.1f B:%.1f"),
                       Left, Top, Right, Bottom);
                return true;
            }
            // Try array format [left, top, right, bottom]
            const TArray<TSharedPtr<FJsonValue>>* OffsetsArray;
            if (PropertyValue->TryGetArray(OffsetsArray) && OffsetsArray->Num() == 4)
            {
                FMargin Offsets(
                    static_cast<float>((*OffsetsArray)[0]->AsNumber()),
                    static_cast<float>((*OffsetsArray)[1]->AsNumber()),
                    static_cast<float>((*OffsetsArray)[2]->AsNumber()),
                    static_cast<float>((*OffsetsArray)[3]->AsNumber())
                );
                CanvasSlot->SetOffsets(Offsets);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.Offsets from array"));
                return true;
            }
            OutError = TEXT("Offsets must be object {Left,Top,Right,Bottom} or array [l,t,r,b]");
            return false;
        }
        else if (PropertyName.Equals(TEXT("Position"), ESearchCase::IgnoreCase))
        {
            const TArray<TSharedPtr<FJsonValue>>* PosArray;
            if (PropertyValue->TryGetArray(PosArray) && PosArray->Num() == 2)
            {
                FVector2D Position(
                    static_cast<float>((*PosArray)[0]->AsNumber()),
                    static_cast<float>((*PosArray)[1]->AsNumber())
                );
                CanvasSlot->SetPosition(Position);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.Position to (%.1f, %.1f)"),
                       Position.X, Position.Y);
                return true;
            }
            OutError = TEXT("Position must be array [X, Y]");
            return false;
        }
        else if (PropertyName.Equals(TEXT("Size"), ESearchCase::IgnoreCase))
        {
            const TArray<TSharedPtr<FJsonValue>>* SizeArray;
            if (PropertyValue->TryGetArray(SizeArray) && SizeArray->Num() == 2)
            {
                FVector2D Size(
                    static_cast<float>((*SizeArray)[0]->AsNumber()),
                    static_cast<float>((*SizeArray)[1]->AsNumber())
                );
                CanvasSlot->SetSize(Size);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.Size to (%.1f, %.1f)"),
                       Size.X, Size.Y);
                return true;
            }
            OutError = TEXT("Size must be array [Width, Height]");
            return false;
        }
        else if (PropertyName.Equals(TEXT("Alignment"), ESearchCase::IgnoreCase))
        {
            const TArray<TSharedPtr<FJsonValue>>* AlignArray;
            if (PropertyValue->TryGetArray(AlignArray) && AlignArray->Num() == 2)
            {
                FVector2D Alignment(
                    static_cast<float>((*AlignArray)[0]->AsNumber()),
                    static_cast<float>((*AlignArray)[1]->AsNumber())
                );
                CanvasSlot->SetAlignment(Alignment);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.Alignment to (%.2f, %.2f)"),
                       Alignment.X, Alignment.Y);
                return true;
            }
            OutError = TEXT("Alignment must be array [X, Y] with values 0.0-1.0");
            return false;
        }
        else if (PropertyName.Equals(TEXT("AutoSize"), ESearchCase::IgnoreCase) ||
                 PropertyName.Equals(TEXT("bAutoSize"), ESearchCase::IgnoreCase))
        {
            bool bAutoSize = false;
            if (PropertyValue->TryGetBool(bAutoSize))
            {
                CanvasSlot->SetAutoSize(bAutoSize);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.AutoSize to %s"),
                       bAutoSize ? TEXT("true") : TEXT("false"));
                return true;
            }
            OutError = TEXT("AutoSize must be a boolean");
            return false;
        }
        else if (PropertyName.Equals(TEXT("ZOrder"), ESearchCase::IgnoreCase))
        {
            int32 ZOrder = 0;
            if (PropertyValue->TryGetNumber(ZOrder))
            {
                CanvasSlot->SetZOrder(ZOrder);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set CanvasPanelSlot.ZOrder to %d"), ZOrder);
                return true;
            }
            OutError = TEXT("ZOrder must be an integer");
            return false;
        }
    }

    // Unknown slot type or property
    OutError = FString::Printf(TEXT("Unsupported slot property '%s' for slot type '%s'"),
                               *PropertyName, *Slot->GetClass()->GetName());
    return false;
}
