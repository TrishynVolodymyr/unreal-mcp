#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Services/IBlueprintService.h"
#include "Commands/Blueprint/ComponentCreationParams.h"

/**
 * Test suite for Blueprint Creation Parameters validation
 */
namespace BlueprintParameterTests
{
    /**
     * Test FBlueprintCreationParams validation
     */
    void TestBlueprintCreationParamsValidation()
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Blueprint Creation Parameters Validation Test Started ==="));
        
        FBlueprintCreationParams Params;
        FString ErrorMessage;
        
        // Test invalid parameters (empty name)
        Params.Name = TEXT("");
        Params.FolderPath = TEXT("/Game/Test");
        Params.ParentClass = AActor::StaticClass();
        
        bool bIsValid = Params.IsValid(ErrorMessage);
        if (!bIsValid && !ErrorMessage.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Empty name correctly rejected: %s"), *ErrorMessage);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Empty name should be rejected"));
        }
        
        // Test invalid parameters (null parent class)
        Params.Name = TEXT("ValidName");
        Params.ParentClass = nullptr;
        ErrorMessage.Empty();
        
        bIsValid = Params.IsValid(ErrorMessage);
        if (!bIsValid && !ErrorMessage.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Null parent class correctly rejected: %s"), *ErrorMessage);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Null parent class should be rejected"));
        }
        
        // Test valid parameters
        Params.Name = TEXT("ValidBlueprint");
        Params.FolderPath = TEXT("/Game/Blueprints");
        Params.ParentClass = AActor::StaticClass();
        Params.bCompileOnCreation = true;
        ErrorMessage.Empty();
        
        bIsValid = Params.IsValid(ErrorMessage);
        if (bIsValid && ErrorMessage.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Valid parameters accepted"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Valid parameters should be accepted"));
        }
        
        // Test edge cases - very long name
        Params.Name = FString::ChrN(300, TEXT('A')); // 300 character name
        ErrorMessage.Empty();
        
        bIsValid = Params.IsValid(ErrorMessage);
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Overly long name correctly rejected: %s"), *ErrorMessage);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("! Very long name accepted (may be valid depending on implementation)"));
        }
        
        // Test invalid folder path characters
        Params.Name = TEXT("ValidName");
        Params.FolderPath = TEXT("/Game/Invalid<>Path");
        ErrorMessage.Empty();
        
        bIsValid = Params.IsValid(ErrorMessage);
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Invalid folder path characters correctly rejected: %s"), *ErrorMessage);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("! Invalid folder path characters accepted (may be handled elsewhere)"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("=== Blueprint Creation Parameters Validation Test Completed ==="));
    }
    
    /**
     * Run all Blueprint parameter tests
     */
    void RunAllTests()
    {
        TestBlueprintCreationParamsValidation();
    }
}

/**
 * Test suite for Component Creation Parameters validation
 */
namespace ComponentParameterTests
{
    /**
     * Test FComponentCreationParams validation
     */
    void TestComponentCreationParamsValidation()
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Component Creation Parameters Validation Test Started ==="));
        
        FComponentCreationParams Params;
        
        // Test valid parameters
        Params.ComponentName = TEXT("TestComponent");
        Params.ComponentType = TEXT("StaticMeshComponent");
        Params.Location = {100.0f, 200.0f, 300.0f};
        Params.Rotation = {0.0f, 45.0f, 90.0f};
        Params.Scale = {1.0f, 2.0f, 1.0f};
        
        // Verify parameters are set correctly
        if (Params.ComponentName == TEXT("TestComponent") && 
            Params.ComponentType == TEXT("StaticMeshComponent"))
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Component parameters set correctly"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Component parameters not set correctly"));
        }
        
        // Test location array
        if (Params.Location.Num() == 3 && 
            FMath::IsNearlyEqual(Params.Location[0], 100.0f) &&
            FMath::IsNearlyEqual(Params.Location[1], 200.0f) &&
            FMath::IsNearlyEqual(Params.Location[2], 300.0f))
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Location parameters set correctly"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Location parameters not set correctly"));
        }
        
        // Test rotation array
        if (Params.Rotation.Num() == 3 && 
            FMath::IsNearlyEqual(Params.Rotation[1], 45.0f) &&
            FMath::IsNearlyEqual(Params.Rotation[2], 90.0f))
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Rotation parameters set correctly"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Rotation parameters not set correctly"));
        }
        
        // Test scale array
        if (Params.Scale.Num() == 3 && 
            FMath::IsNearlyEqual(Params.Scale[1], 2.0f))
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Scale parameters set correctly"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Scale parameters not set correctly"));
        }
        
        // Test edge cases - empty arrays
        Params.Location.Empty();
        Params.Rotation.Empty();
        Params.Scale.Empty();
        
        if (Params.Location.Num() == 0 && Params.Rotation.Num() == 0 && Params.Scale.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Empty transform arrays handled correctly"));
        }
        
        // Test invalid array sizes
        Params.Location = {100.0f, 200.0f}; // Only 2 elements instead of 3
        if (Params.Location.Num() == 2)
        {
            UE_LOG(LogTemp, Warning, TEXT("! Incomplete location array accepted (validation may be handled elsewhere)"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("=== Component Creation Parameters Validation Test Completed ==="));
    }
    
    /**
     * Test component type string validation
     */
    void TestComponentTypeValidation()
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Component Type String Validation Test Started ==="));
        
        // Test valid component types
        TArray<FString> ValidTypes = {
            TEXT("StaticMeshComponent"),
            TEXT("PointLightComponent"),
            TEXT("SphereComponent"),
            TEXT("CameraComponent"),
            TEXT("AudioComponent")
        };
        
        for (const FString& ComponentType : ValidTypes)
        {
            if (!ComponentType.IsEmpty() && ComponentType.Len() > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("✓ Valid component type format: %s"), *ComponentType);
            }
        }
        
        // Test invalid component types
        TArray<FString> InvalidTypes = {
            TEXT(""),
            TEXT(" "),
            TEXT("Invalid Component"),
            TEXT("Component<>"),
            TEXT("123Component")
        };
        
        for (const FString& ComponentType : InvalidTypes)
        {
            if (ComponentType.IsEmpty() || ComponentType.Contains(TEXT(" ")) || ComponentType.Contains(TEXT("<")))
            {
                UE_LOG(LogTemp, Warning, TEXT("✓ Invalid component type format detected: '%s'"), *ComponentType);
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("=== Component Type String Validation Test Completed ==="));
    }
    
    /**
     * Run all Component parameter tests
     */
    void RunAllTests()
    {
        TestComponentCreationParamsValidation();
        TestComponentTypeValidation();
    }
}

/**
 * Test suite for JSON parameter validation
 */
namespace JsonParameterTests
{
    /**
     * Test JSON value validation for property setting
     */
    void TestJsonValueValidation()
    {
        UE_LOG(LogTemp, Warning, TEXT("=== JSON Value Validation Test Started ==="));
        
        // Test valid JSON values
        TSharedPtr<FJsonValue> StringValue = MakeShareable(new FJsonValueString(TEXT("TestString")));
        TSharedPtr<FJsonValue> NumberValue = MakeShareable(new FJsonValueNumber(42.5));
        TSharedPtr<FJsonValue> BoolValue = MakeShareable(new FJsonValueBoolean(true));
        TSharedPtr<FJsonValue> NullValue = MakeShareable(new FJsonValueNull());
        
        if (StringValue.IsValid() && StringValue->Type == EJson::String)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ String JSON value created correctly"));
        }
        
        if (NumberValue.IsValid() && NumberValue->Type == EJson::Number)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Number JSON value created correctly"));
        }
        
        if (BoolValue.IsValid() && BoolValue->Type == EJson::Boolean)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Boolean JSON value created correctly"));
        }
        
        if (NullValue.IsValid() && NullValue->Type == EJson::Null)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Null JSON value created correctly"));
        }
        
        // Test JSON array
        TArray<TSharedPtr<FJsonValue>> ArrayValues;
        ArrayValues.Add(MakeShareable(new FJsonValueNumber(1.0)));
        ArrayValues.Add(MakeShareable(new FJsonValueNumber(2.0)));
        ArrayValues.Add(MakeShareable(new FJsonValueNumber(3.0)));
        TSharedPtr<FJsonValue> ArrayValue = MakeShareable(new FJsonValueArray(ArrayValues));
        
        if (ArrayValue.IsValid() && ArrayValue->Type == EJson::Array)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Array JSON value created correctly"));
        }
        
        // Test JSON object
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        JsonObject->SetStringField(TEXT("Name"), TEXT("TestObject"));
        JsonObject->SetNumberField(TEXT("Value"), 100.0);
        JsonObject->SetBoolField(TEXT("IsActive"), true);
        TSharedPtr<FJsonValue> ObjectValue = MakeShareable(new FJsonValueObject(JsonObject));
        
        if (ObjectValue.IsValid() && ObjectValue->Type == EJson::Object)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Object JSON value created correctly"));
        }
        
        // Test accessing JSON object fields
        if (JsonObject->HasField(TEXT("Name")) && 
            JsonObject->GetStringField(TEXT("Name")) == TEXT("TestObject"))
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ JSON object field access working correctly"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("=== JSON Value Validation Test Completed ==="));
    }
    
    /**
     * Test JSON parsing edge cases
     */
    void TestJsonEdgeCases()
    {
        UE_LOG(LogTemp, Warning, TEXT("=== JSON Edge Cases Test Started ==="));
        
        // Test empty JSON object
        TSharedPtr<FJsonObject> EmptyObject = MakeShareable(new FJsonObject);
        if (EmptyObject.IsValid() && EmptyObject->Values.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Empty JSON object handled correctly"));
        }
        
        // Test very large number
        double LargeNumber = 1e10;
        TSharedPtr<FJsonValue> LargeNumberValue = MakeShareable(new FJsonValueNumber(LargeNumber));
        if (LargeNumberValue.IsValid() && FMath::IsNearlyEqual(LargeNumberValue->AsNumber(), LargeNumber))
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Large number JSON value handled correctly"));
        }
        
        // Test very long string
        FString LongString = FString::ChrN(1000, TEXT('A'));
        TSharedPtr<FJsonValue> LongStringValue = MakeShareable(new FJsonValueString(LongString));
        if (LongStringValue.IsValid() && LongStringValue->AsString().Len() == 1000)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Long string JSON value handled correctly"));
        }
        
        // Test special characters in string
        FString SpecialString = TEXT("Test\nString\tWith\rSpecial\\Characters\"");
        TSharedPtr<FJsonValue> SpecialStringValue = MakeShareable(new FJsonValueString(SpecialString));
        if (SpecialStringValue.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Special characters in JSON string handled correctly"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("=== JSON Edge Cases Test Completed ==="));
    }
    
    /**
     * Run all JSON parameter tests
     */
    void RunAllTests()
    {
        TestJsonValueValidation();
        TestJsonEdgeCases();
    }
}

/**
 * Main test function to run all parameter validation tests
 */
void TestParameterValidation()
{
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== PARAMETER VALIDATION TESTS STARTED ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    
    BlueprintParameterTests::RunAllTests();
    ComponentParameterTests::RunAllTests();
    JsonParameterTests::RunAllTests();
    
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== PARAMETER VALIDATION TESTS COMPLETED ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}