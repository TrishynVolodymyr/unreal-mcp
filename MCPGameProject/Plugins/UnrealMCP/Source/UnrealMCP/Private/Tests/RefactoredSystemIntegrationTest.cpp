#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/UnrealMCPMainDispatcher.h"
#include "Services/BlueprintService.h"
#include "Services/ComponentService.h"
#include "Factories/ComponentFactory.h"
#include "Factories/WidgetFactory.h"
#include "MCPOperationContext.h"
#include "MCPErrorHandler.h"
#include "MCPResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Forward declarations
void TestCommandRegistryIntegration();
void TestMainDispatcherIntegration();
void TestServiceLayerIntegration();
void TestFactoryPatternIntegration();
void TestErrorHandlingIntegration();
void TestEndToEndCommandFlow();

/**
 * Comprehensive integration test for the refactored MCP system
 * Tests complete command execution flow through new architecture
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void TestRefactoredSystemIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Refactored System Integration Test Started ==="));
    
    // Test 1: Command Registry Functionality
    TestCommandRegistryIntegration();
    
    // Test 2: Main Dispatcher Integration
    TestMainDispatcherIntegration();
    
    // Test 3: Service Layer Integration
    TestServiceLayerIntegration();
    
    // Test 4: Factory Pattern Integration
    TestFactoryPatternIntegration();
    
    // Test 5: Error Handling Integration
    TestErrorHandlingIntegration();
    
    // Test 6: End-to-End Command Flow
    TestEndToEndCommandFlow();
    
    UE_LOG(LogTemp, Warning, TEXT("=== Refactored System Integration Test Completed ==="));
}

/**
 * Test command registry functionality
 */
void TestCommandRegistryIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Testing Command Registry Integration ---"));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    // Test getting registered commands
    TArray<FString> RegisteredCommands = Registry.GetRegisteredCommandNames();
    UE_LOG(LogTemp, Warning, TEXT("✓ Found %d registered commands"), RegisteredCommands.Num());
    
    // Test some expected commands
    bool bHasCreateBlueprint = Registry.IsCommandRegistered(TEXT("create_blueprint"));
    bool bHasAddComponent = Registry.IsCommandRegistered(TEXT("add_component_to_blueprint"));
    bool bHasCompileBlueprint = Registry.IsCommandRegistered(TEXT("compile_blueprint"));
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Expected commands registered:"));
    UE_LOG(LogTemp, Warning, TEXT("  - create_blueprint: %s"), bHasCreateBlueprint ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Warning, TEXT("  - add_component_to_blueprint: %s"), bHasAddComponent ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Warning, TEXT("  - compile_blueprint: %s"), bHasCompileBlueprint ? TEXT("Yes") : TEXT("No"));
    
    // Test command help
    if (bHasCreateBlueprint)
    {
        FString HelpInfo = Registry.GetCommandHelp(TEXT("create_blueprint"));
        bool bHasHelpInfo = !HelpInfo.IsEmpty();
        UE_LOG(LogTemp, Warning, TEXT("✓ Command help available: %s"), bHasHelpInfo ? TEXT("Yes") : TEXT("No"));
    }
}

/**
 * Test main dispatcher integration
 */
void TestMainDispatcherIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Testing Main Dispatcher Integration ---"));
    
    FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
    
    // Test getting available commands
    TSharedPtr<FJsonObject> AvailableCommands = Dispatcher.GetAvailableCommands();
    bool bHasCommands = AvailableCommands.IsValid() && AvailableCommands->HasField(TEXT("commands"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Available commands response: %s"), bHasCommands ? TEXT("Valid") : TEXT("Invalid"));
    
    // Test invalid command handling
    TSharedPtr<FJsonObject> InvalidParams = MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> ErrorResponse = Dispatcher.HandleCommand(TEXT("invalid_command"), InvalidParams);
    
    bool bHasError = ErrorResponse.IsValid() && ErrorResponse->HasField(TEXT("success")) && 
                     !ErrorResponse->GetBoolField(TEXT("success"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Invalid command error handling: %s"), bHasError ? TEXT("Correct") : TEXT("Failed"));
}

/**
 * Test service layer integration
 */
void TestServiceLayerIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Testing Service Layer Integration ---"));
    
    // Test Blueprint Service
    FBlueprintService& BlueprintService = FBlueprintService::Get();
    
    // Test finding existing blueprints (should not crash)
    UBlueprint* TestBlueprint = BlueprintService.FindBlueprint(TEXT("NonExistentBlueprint"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Blueprint service find test: %s"), 
           TestBlueprint ? TEXT("Found") : TEXT("Not found (expected)"));
    
    // Test Component Service
    FComponentService& ComponentService = FComponentService::Get();
    
    // Test component class resolution
    UClass* StaticMeshClass = ComponentService.GetComponentClass(TEXT("StaticMeshComponent"));
    UClass* PointLightClass = ComponentService.GetComponentClass(TEXT("PointLightComponent"));
    
    bool bHasStaticMesh = (StaticMeshClass != nullptr);
    bool bHasPointLight = (PointLightClass != nullptr);
    UE_LOG(LogTemp, Warning, TEXT("✓ Component service class resolution: StaticMesh=%s, PointLight=%s"),
           bHasStaticMesh ? TEXT("Yes") : TEXT("No"),
           bHasPointLight ? TEXT("Yes") : TEXT("No"));
}

/**
 * Test factory pattern integration
 */
void TestFactoryPatternIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Testing Factory Pattern Integration ---"));
    
    // Test Component Factory
    FComponentFactory& ComponentFactory = FComponentFactory::Get();
    TArray<FString> ComponentTypes = ComponentFactory.GetAvailableTypes();
    UE_LOG(LogTemp, Warning, TEXT("✓ Component factory types: %d"), ComponentTypes.Num());
    
    // Test getting component classes
    UClass* StaticMeshClass = ComponentFactory.GetComponentClass(TEXT("StaticMeshComponent"));
    UClass* InvalidClass = ComponentFactory.GetComponentClass(TEXT("InvalidComponent"));
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Component factory class resolution:"));
    UE_LOG(LogTemp, Warning, TEXT("  - StaticMeshComponent: %s"), StaticMeshClass ? TEXT("Found") : TEXT("Not found"));
    UE_LOG(LogTemp, Warning, TEXT("  - InvalidComponent: %s"), InvalidClass ? TEXT("Found") : TEXT("Not found (expected)"));
    
    // Test Widget Factory
    FWidgetFactory& WidgetFactory = FWidgetFactory::Get();
    TArray<FString> WidgetTypes = WidgetFactory.GetAvailableWidgetTypes();
    UE_LOG(LogTemp, Warning, TEXT("✓ Widget factory types: %d"), WidgetTypes.Num());
    
    bool bHasTextBlock = WidgetTypes.Contains(TEXT("TextBlock"));
    bool bHasButton = WidgetTypes.Contains(TEXT("Button"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Expected widget types: TextBlock=%s, Button=%s"),
           bHasTextBlock ? TEXT("Yes") : TEXT("No"),
           bHasButton ? TEXT("Yes") : TEXT("No"));
}

/**
 * Test error handling integration
 */
void TestErrorHandlingIntegration()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Testing Error Handling Integration ---"));
    
    // Test MCPOperationContext (UObject-based)
    UMCPOperationContext* Context = NewObject<UMCPOperationContext>();
    Context->Initialize(TEXT("TestOperation"), TEXT("test-op-001"));
    
    // Add test errors using the correct structure
    FMCPError ValidationError(EMCPErrorType::ValidationFailed, 1001, 
                             TEXT("Test validation error"), 
                             TEXT("This is a test error for integration testing"));
    
    Context->AddError(ValidationError, EMCPErrorSeverity::Error);
    Context->AddWarning(TEXT("Test warning message"), TEXT("Warning details"));
    
    bool bHasErrors = Context->HasErrors();
    UE_LOG(LogTemp, Warning, TEXT("✓ Operation context error tracking: %s"), bHasErrors ? TEXT("Working") : TEXT("Failed"));
    
    // Test response creation
    FString ErrorResponse = Context->CreateResponse();
    bool bValidErrorResponse = !ErrorResponse.IsEmpty();
    UE_LOG(LogTemp, Warning, TEXT("✓ Error response creation: %s"), bValidErrorResponse ? TEXT("Working") : TEXT("Failed"));
    
    // Test error handler
    FMCPError TestError = FMCPErrorHandler::CreateValidationFailedError(TEXT("Test validation error"));
    FString ValidationErrorResponse = FMCPErrorHandler::CreateStructuredErrorResponse(TestError);
    bool bValidValidationResponse = !ValidationErrorResponse.IsEmpty();
    UE_LOG(LogTemp, Warning, TEXT("✓ Error handler validation response: %s"), 
           bValidValidationResponse ? TEXT("Working") : TEXT("Failed"));
}

/**
 * Test end-to-end command flow
 */
void TestEndToEndCommandFlow()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Testing End-to-End Command Flow ---"));
    
    FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
    
    // Test 1: Create Blueprint command flow
    TSharedPtr<FJsonObject> CreateBlueprintParams = MakeShareable(new FJsonObject);
    CreateBlueprintParams->SetStringField(TEXT("name"), TEXT("TestIntegrationBlueprint"));
    CreateBlueprintParams->SetStringField(TEXT("parent_class"), TEXT("Actor"));
    CreateBlueprintParams->SetStringField(TEXT("folder_path"), TEXT("Test"));
    
    TSharedPtr<FJsonObject> CreateResponse = Dispatcher.HandleCommand(TEXT("create_blueprint"), CreateBlueprintParams);
    bool bCreateSuccess = CreateResponse.IsValid() && CreateResponse->HasField(TEXT("success"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Create blueprint command flow: %s"), bCreateSuccess ? TEXT("Executed") : TEXT("Failed"));
    
    if (bCreateSuccess)
    {
        bool bSuccess = CreateResponse->GetBoolField(TEXT("success"));
        UE_LOG(LogTemp, Warning, TEXT("  - Command result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
        
        if (CreateResponse->HasField(TEXT("message")))
        {
            FString Message = CreateResponse->GetStringField(TEXT("message"));
            UE_LOG(LogTemp, Warning, TEXT("  - Message: %s"), *Message);
        }
    }
    
    // Test 2: Invalid parameter handling
    TSharedPtr<FJsonObject> InvalidParams = MakeShareable(new FJsonObject);
    // Missing required parameters
    
    TSharedPtr<FJsonObject> InvalidResponse = Dispatcher.HandleCommand(TEXT("create_blueprint"), InvalidParams);
    bool bInvalidHandled = InvalidResponse.IsValid() && InvalidResponse->HasField(TEXT("success")) &&
                          !InvalidResponse->GetBoolField(TEXT("success"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Invalid parameter handling: %s"), bInvalidHandled ? TEXT("Correct") : TEXT("Failed"));
    
    // Test 3: Component creation flow (if blueprint creation succeeded)
    if (bCreateSuccess && CreateResponse->GetBoolField(TEXT("success")))
    {
        TSharedPtr<FJsonObject> AddComponentParams = MakeShareable(new FJsonObject);
        AddComponentParams->SetStringField(TEXT("blueprint_name"), TEXT("TestIntegrationBlueprint"));
        AddComponentParams->SetStringField(TEXT("component_type"), TEXT("StaticMeshComponent"));
        AddComponentParams->SetStringField(TEXT("component_name"), TEXT("TestMesh"));
        
        TSharedPtr<FJsonObject> ComponentResponse = Dispatcher.HandleCommand(TEXT("add_component_to_blueprint"), AddComponentParams);
        bool bComponentSuccess = ComponentResponse.IsValid() && ComponentResponse->HasField(TEXT("success"));
        UE_LOG(LogTemp, Warning, TEXT("✓ Add component command flow: %s"), bComponentSuccess ? TEXT("Executed") : TEXT("Failed"));
    }
}