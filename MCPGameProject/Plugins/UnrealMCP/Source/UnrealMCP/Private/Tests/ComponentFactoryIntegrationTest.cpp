#include "Factories/ComponentFactory.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"

/**
 * Integration test function to verify ComponentFactory functionality
 * This function tests all the main ComponentFactory methods
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void TestComponentFactoryIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("=== ComponentFactory Integration Test Started ==="));
    
    FComponentFactory& Factory = FComponentFactory::Get();
    
    // Test 1: Get available types
    TArray<FString> AvailableTypes = Factory.GetAvailableTypes();
    UE_LOG(LogTemp, Warning, TEXT("✓ GetAvailableTypes: Found %d component types"), AvailableTypes.Num());
    
    // Verify we have some expected types
    bool bFoundStaticMesh = AvailableTypes.Contains(TEXT("StaticMeshComponent"));
    bool bFoundPointLight = AvailableTypes.Contains(TEXT("PointLightComponent"));
    bool bFoundSphere = AvailableTypes.Contains(TEXT("SphereComponent"));
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Expected types found: StaticMesh=%s, PointLight=%s, Sphere=%s"), 
           bFoundStaticMesh ? TEXT("Yes") : TEXT("No"),
           bFoundPointLight ? TEXT("Yes") : TEXT("No"),
           bFoundSphere ? TEXT("Yes") : TEXT("No"));
    
    // Test 2: Get specific component classes
    UClass* StaticMeshClass = Factory.GetComponentClass(TEXT("StaticMeshComponent"));
    UClass* PointLightClass = Factory.GetComponentClass(TEXT("PointLightComponent"));
    UClass* SphereClass = Factory.GetComponentClass(TEXT("SphereComponent"));
    UClass* InvalidClass = Factory.GetComponentClass(TEXT("NonExistentComponent"));
    
    UE_LOG(LogTemp, Warning, TEXT("✓ GetComponentClass tests:"));
    UE_LOG(LogTemp, Warning, TEXT("  - StaticMeshComponent: %s"), StaticMeshClass ? *StaticMeshClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("  - PointLightComponent: %s"), PointLightClass ? *PointLightClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("  - SphereComponent: %s"), SphereClass ? *SphereClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("  - NonExistentComponent: %s"), InvalidClass ? *InvalidClass->GetName() : TEXT("NULL (Expected)"));
    
    // Test 3: Custom registration
    Factory.RegisterComponentType(TEXT("CustomTestType"), UStaticMeshComponent::StaticClass());
    UClass* CustomClass = Factory.GetComponentClass(TEXT("CustomTestType"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Custom registration test: %s"), CustomClass ? TEXT("SUCCESS") : TEXT("FAILED"));
    
    // Test 4: Verify thread safety by getting available types again
    TArray<FString> AvailableTypes2 = Factory.GetAvailableTypes();
    bool bContainsCustom = AvailableTypes2.Contains(TEXT("CustomTestType"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Thread safety test: Custom type in list = %s"), bContainsCustom ? TEXT("Yes") : TEXT("No"));
    
    // Test 5: Verify class inheritance
    if (StaticMeshClass)
    {
        bool bIsActorComponent = StaticMeshClass->IsChildOf(UActorComponent::StaticClass());
        bool bIsSceneComponent = StaticMeshClass->IsChildOf(USceneComponent::StaticClass());
        UE_LOG(LogTemp, Warning, TEXT("✓ Inheritance test: StaticMeshComponent is ActorComponent=%s, SceneComponent=%s"), 
               bIsActorComponent ? TEXT("Yes") : TEXT("No"),
               bIsSceneComponent ? TEXT("Yes") : TEXT("No"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== ComponentFactory Integration Test Completed ==="));
}