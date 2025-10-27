#include "Factories/WidgetFactory.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"

/**
 * Integration test function to verify WidgetFactory functionality
 * This function tests widget type registration, retrieval, and initialization
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void TestWidgetFactoryIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("=== WidgetFactory Test Started ==="));
    
    FWidgetFactory& Factory = FWidgetFactory::Get();
    
    // Test getting available types
    TArray<FString> AvailableTypes = Factory.GetAvailableWidgetTypes();
    UE_LOG(LogTemp, Warning, TEXT("WidgetFactory has %d registered widget types"), AvailableTypes.Num());
    
    // Test some common widget types
    TArray<FString> TestTypes = {
        TEXT("TextBlock"),
        TEXT("Button"), 
        TEXT("Image"),
        TEXT("Border"),
        TEXT("CanvasPanel"),
        TEXT("VerticalBox"),
        TEXT("HorizontalBox"),
        TEXT("CheckBox"),
        TEXT("Slider"),
        TEXT("ProgressBar")
    };
    
    int32 SuccessCount = 0;
    for (const FString& TypeName : TestTypes)
    {
        UClass* WidgetClass = Factory.GetWidgetClass(TypeName);
        if (WidgetClass)
        {
            UE_LOG(LogTemp, Log, TEXT("✓ Found widget type '%s' -> '%s'"), *TypeName, *WidgetClass->GetName());
            SuccessCount++;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Failed to find widget type '%s'"), *TypeName);
        }
    }
    
    // Test custom widget type registration
    UE_LOG(LogTemp, Warning, TEXT("Testing custom widget type registration..."));
    Factory.RegisterWidgetType(TEXT("CustomTestWidget"), UTextBlock::StaticClass());
    
    UClass* CustomWidgetClass = Factory.GetWidgetClass(TEXT("CustomTestWidget"));
    if (CustomWidgetClass)
    {
        UE_LOG(LogTemp, Log, TEXT("✓ Custom widget type registration successful"));
        SuccessCount++;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Custom widget type registration failed"));
    }
    
    // Test invalid widget type
    UClass* InvalidWidgetClass = Factory.GetWidgetClass(TEXT("NonExistentWidget"));
    if (!InvalidWidgetClass)
    {
        UE_LOG(LogTemp, Log, TEXT("✓ Correctly returned nullptr for invalid widget type"));
        SuccessCount++;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Should have returned nullptr for invalid widget type"));
    }
    
    // Print all available widget types
    UE_LOG(LogTemp, Warning, TEXT("All registered widget types:"));
    TArray<FString> AllTypes = Factory.GetAvailableWidgetTypes();
    for (int32 i = 0; i < FMath::Min(AllTypes.Num(), 20); i++) // Limit to first 20 for readability
    {
        UE_LOG(LogTemp, Log, TEXT("  %d. %s"), i + 1, *AllTypes[i]);
    }
    if (AllTypes.Num() > 20)
    {
        UE_LOG(LogTemp, Log, TEXT("  ... and %d more types"), AllTypes.Num() - 20);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== WidgetFactory Test Completed: %d/%d tests passed ==="), 
           SuccessCount, TestTypes.Num() + 2);
}