#include "Factories/ComponentFactory.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"

/**
 * Simple test function to verify ComponentFactory functionality
 * This can be called from Blueprint or C++ to test the factory
 */
void TestComponentFactory()
{
    UE_LOG(LogTemp, Warning, TEXT("=== ComponentFactory Test Started ==="));
    
    FComponentFactory& Factory = FComponentFactory::Get();
    
    // Test getting available types
    TArray<FString> AvailableTypes = Factory.GetAvailableTypes();
    UE_LOG(LogTemp, Warning, TEXT("ComponentFactory: Found %d available component types"), AvailableTypes.Num());
    
    // Log first few types for verification
    for (int32 i = 0; i < FMath::Min(5, AvailableTypes.Num()); ++i)
    {
        UE_LOG(LogTemp, Warning, TEXT("  - %s"), *AvailableTypes[i]);
    }
    
    // Test specific component type lookups
    TArray<FString> TestTypes = {
        TEXT("StaticMeshComponent"),
        TEXT("PointLightComponent"), 
        TEXT("SphereComponent"),
        TEXT("CameraComponent"),
        TEXT("NonExistentComponent")
    };
    
    for (const FString& TypeName : TestTypes)
    {
        UClass* ComponentClass = Factory.GetComponentClass(TypeName);
        if (ComponentClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Found component type '%s' -> '%s'"), *TypeName, *ComponentClass->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Component type '%s' not found"), *TypeName);
        }
    }
    
    // Test custom registration
    Factory.RegisterComponentType(TEXT("CustomTestComponent"), UStaticMeshComponent::StaticClass());
    UClass* CustomClass = Factory.GetComponentClass(TEXT("CustomTestComponent"));
    if (CustomClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("✓ Custom component registration successful"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("✗ Custom component registration failed"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== ComponentFactory Test Completed ==="));
}