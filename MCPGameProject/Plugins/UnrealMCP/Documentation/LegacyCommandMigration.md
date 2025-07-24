# Legacy Command Migration Guide

## Overview

This guide helps developers migrate from the old monolithic command handlers to the new layered architecture. The refactoring introduces better separation of concerns, improved testability, and enhanced maintainability.

## Architecture Changes

### Before: Monolithic Command Handlers

```cpp
// Old approach - everything in one large handler
class FUnrealMCPBlueprintCommands
{
public:
    static FString HandleCreateBlueprint(const FString& Parameters)
    {
        // Parameter parsing
        // Validation
        // Business logic
        // Error handling
        // Response formatting
        // All mixed together in one function
    }
    
    static FString HandleAddComponent(const FString& Parameters)
    {
        // Another large function with mixed concerns
    }
    
    // Many more static functions...
};
```

### After: Layered Architecture

```cpp
// New approach - separated concerns

// 1. Command Layer
class FCreateBlueprintCommand : public IUnrealMCPCommand
{
private:
    TSharedPtr<IBlueprintService> BlueprintService;
    FParameterValidator Validator;
    
public:
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;
};

// 2. Service Layer
class FBlueprintService : public IBlueprintService
{
public:
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override;
};

// 3. Factory Layer
class FComponentFactory
{
public:
    static FComponentFactory& Get();
    UClass* GetComponentClass(const FString& TypeName) const;
};
```

## Migration Steps

### Step 1: Extract Service Layer

**Before:**
```cpp
static FString HandleCreateBlueprint(const FString& Parameters)
{
    // Parse JSON
    TSharedPtr<FJsonObject> JsonObject;
    // ... parsing code ...
    
    // Validate parameters
    if (!JsonObject->HasField(TEXT("name")))
    {
        return TEXT("{\"error\": \"Missing name parameter\"}");
    }
    
    // Business logic mixed with validation and formatting
    FString BlueprintName = JsonObject->GetStringField(TEXT("name"));
    UBlueprint* Blueprint = CreateBlueprintInternal(BlueprintName);
    
    if (!Blueprint)
    {
        return TEXT("{\"error\": \"Failed to create blueprint\"}");
    }
    
    return TEXT("{\"success\": true}");
}
```

**After:**
```cpp
// Service handles pure business logic
class FBlueprintService : public IBlueprintService
{
public:
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override
    {
        // Pure business logic - no JSON parsing or response formatting
        if (!Params.IsValid())
        {
            return nullptr;
        }
        
        return CreateBlueprintInternal(Params);
    }
};

// Command handles request/response concerns
class FCreateBlueprintCommand : public IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) override
    {
        // Parse parameters
        FBlueprintCreationParams Params = ParseParameters(Parameters);
        
        // Call service
        UBlueprint* Blueprint = BlueprintService->CreateBlueprint(Params);
        
        // Format response
        return FormatResponse(Blueprint);
    }
};
```

### Step 2: Implement Parameter Validation

**Before:**
```cpp
static FString HandleCommand(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    // Manual validation scattered throughout
    if (!JsonObject->HasField(TEXT("name")) || JsonObject->GetStringField(TEXT("name")).IsEmpty())
    {
        return CreateErrorResponse(TEXT("Invalid name"));
    }
    
    if (!JsonObject->HasField(TEXT("type")))
    {
        return CreateErrorResponse(TEXT("Missing type"));
    }
    
    // More validation...
}
```

**After:**
```cpp
class FCreateBlueprintCommand : public IUnrealMCPCommand
{
private:
    FParameterValidator Validator;
    
public:
    FCreateBlueprintCommand()
    {
        // Set up validation rules once
        Validator.AddRule(FValidationRuleBuilder::RequiredString(TEXT("name")));
        Validator.AddRule(FValidationRuleBuilder::OptionalString(TEXT("folder_path")));
        Validator.AddRule(FValidationRuleBuilder::RequiredString(TEXT("parent_class")));
    }
    
    virtual bool ValidateParams(const FString& Parameters) const override
    {
        TSharedPtr<FJsonObject> JsonObject = ParseJson(Parameters);
        FString ErrorMessage;
        return Validator.ValidateParams(JsonObject, ErrorMessage);
    }
};
```

### Step 3: Use Structured Error Handling

**Before:**
```cpp
static FString HandleCommand(const FString& Parameters)
{
    if (SomeCondition)
    {
        return TEXT("{\"error\": \"Something went wrong\"}");
    }
    
    if (AnotherCondition)
    {
        return TEXT("{\"error\": \"Different error\", \"code\": 123}");
    }
    
    // Inconsistent error format
}
```

**After:**
```cpp
class FMyCommand : public IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) override
    {
        try
        {
            // Business logic
            auto Result = DoSomething();
            return FMCPErrorHandler::CreateSuccessResponse(TEXT("Operation completed"), Result);
        }
        catch (const FValidationException& Ex)
        {
            FMCPError Error(EMCPErrorType::ValidationFailed, 1001, Ex.GetMessage());
            return FMCPErrorHandler::CreateErrorResponse(Error);
        }
        catch (const FBusinessLogicException& Ex)
        {
            FMCPError Error(EMCPErrorType::ExecutionFailed, 2001, Ex.GetMessage());
            return FMCPErrorHandler::CreateErrorResponse(Error);
        }
    }
};
```

### Step 4: Register Commands

**Before:**
```cpp
// Commands were called directly through static dispatch
FString Result = FUnrealMCPBlueprintCommands::HandleCreateBlueprint(Parameters);
```

**After:**
```cpp
// Commands are registered and executed through registry
void RegisterBlueprintCommands()
{
    auto& Registry = FUnrealMCPCommandRegistry::Get();
    
    Registry.RegisterCommand(MakeShared<FCreateBlueprintCommand>(GetBlueprintService()));
    Registry.RegisterCommand(MakeShared<FAddComponentCommand>(GetComponentService()));
    Registry.RegisterCommand(MakeShared<FCompileBlueprintCommand>(GetBlueprintService()));
}

// Execution through registry
FString Result = FUnrealMCPCommandRegistry::Get().ExecuteCommand(TEXT("create_blueprint"), Parameters);
```

## Common Migration Patterns

### Pattern 1: Parameter Structure Migration

**Before:**
```cpp
static FString HandleAddComponent(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject = ParseJson(Parameters);
    
    FString BlueprintName = JsonObject->GetStringField(TEXT("blueprint_name"));
    FString ComponentType = JsonObject->GetStringField(TEXT("component_type"));
    FString ComponentName = JsonObject->GetStringField(TEXT("component_name"));
    
    // Use individual parameters
}
```

**After:**
```cpp
// Define parameter structure
struct FComponentCreationParams
{
    FString BlueprintName;
    FString ComponentType;
    FString ComponentName;
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    
    bool IsValid(FString& OutError) const
    {
        if (BlueprintName.IsEmpty())
        {
            OutError = TEXT("Blueprint name is required");
            return false;
        }
        return true;
    }
};

// Use structured parameters
class FAddComponentCommand : public IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) override
    {
        FComponentCreationParams Params = ParseComponentParams(Parameters);
        return ComponentService->AddComponent(Params);
    }
};
```

### Pattern 2: Factory Integration

**Before:**
```cpp
static UClass* GetComponentClass(const FString& ComponentType)
{
    // Hard-coded type mapping
    if (ComponentType == TEXT("StaticMeshComponent"))
    {
        return UStaticMeshComponent::StaticClass();
    }
    else if (ComponentType == TEXT("SkeletalMeshComponent"))
    {
        return USkeletalMeshComponent::StaticClass();
    }
    // ... many more hard-coded types
    
    return nullptr;
}
```

**After:**
```cpp
// Use factory pattern
class FComponentService : public IComponentService
{
public:
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params) override
    {
        // Use factory for type resolution
        UClass* ComponentClass = FComponentFactory::Get().GetComponentClass(Params.ComponentType);
        if (!ComponentClass)
        {
            return false;
        }
        
        // Create component using resolved class
        return CreateComponentInternal(Blueprint, ComponentClass, Params);
    }
};

// Register types during initialization
void InitializeComponentFactory()
{
    auto& Factory = FComponentFactory::Get();
    Factory.RegisterComponentType(TEXT("StaticMeshComponent"), UStaticMeshComponent::StaticClass());
    Factory.RegisterComponentType(TEXT("SkeletalMeshComponent"), USkeletalMeshComponent::StaticClass());
    // Extensible type registration
}
```

### Pattern 3: Caching Integration

**Before:**
```cpp
static UBlueprint* FindBlueprint(const FString& BlueprintName)
{
    // No caching - expensive lookup every time
    return LoadObject<UBlueprint>(nullptr, *BlueprintPath);
}
```

**After:**
```cpp
class FBlueprintService : public IBlueprintService
{
private:
    FBlueprintCache Cache;
    
public:
    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) override
    {
        // Check cache first
        UBlueprint* CachedBlueprint = Cache.GetBlueprint(BlueprintName);
        if (CachedBlueprint)
        {
            return CachedBlueprint;
        }
        
        // Load and cache
        UBlueprint* LoadedBlueprint = LoadBlueprintInternal(BlueprintName);
        if (LoadedBlueprint)
        {
            Cache.CacheBlueprint(BlueprintName, LoadedBlueprint);
        }
        
        return LoadedBlueprint;
    }
};
```

## Testing Migration

### Before: Difficult to Test

```cpp
// Hard to test due to static methods and mixed concerns
static FString HandleCreateBlueprint(const FString& Parameters)
{
    // Direct Unreal Engine calls mixed with business logic
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(/*...*/);
    // Hard to mock or test in isolation
}
```

### After: Testable Architecture

```cpp
// Easy to test with dependency injection and mocking
class FCreateBlueprintCommandTest
{
public:
    void TestCreateBlueprint_ValidParams_ReturnsSuccess()
    {
        // Arrange
        auto MockService = MakeShared<FMockBlueprintService>();
        MockService->SetupCreateBlueprintResult(MockBlueprint);
        
        FCreateBlueprintCommand Command(MockService);
        FString Parameters = TEXT("{\"name\":\"TestBP\",\"parent_class\":\"Actor\"}");
        
        // Act
        FString Result = Command.Execute(Parameters);
        
        // Assert
        VerifySuccessResponse(Result);
        VerifyServiceWasCalled(MockService, TEXT("TestBP"));
    }
};
```

## Performance Considerations

### Caching Strategy

**Old Approach:**
- No caching
- Expensive lookups repeated
- Poor performance for frequent operations

**New Approach:**
```cpp
// Implement caching at service layer
class FBlueprintService
{
private:
    FBlueprintCache Cache;
    
public:
    void WarmCache()
    {
        // Pre-load frequently used blueprints
        TArray<FString> CommonBlueprints = GetCommonBlueprintNames();
        Cache.WarmCache(CommonBlueprints);
    }
};
```

### Object Pooling

**Old Approach:**
- Frequent object allocation/deallocation
- Memory fragmentation
- GC pressure

**New Approach:**
```cpp
// Use object pooling for frequently created objects
class FParameterObjectPool
{
public:
    TSharedPtr<FJsonObject> AcquireJsonObject()
    {
        return ObjectPool.Acquire();
    }
    
    void ReleaseJsonObject(TSharedPtr<FJsonObject> Object)
    {
        Object->Values.Empty(); // Clear for reuse
        ObjectPool.Release(Object);
    }
    
private:
    TObjectPool<FJsonObject> ObjectPool;
};
```

## Backward Compatibility

### Legacy Command Adapter

To maintain backward compatibility during migration:

```cpp
class FLegacyCommandAdapter
{
public:
    static FString HandleLegacyCommand(const FString& CommandName, const FString& Parameters)
    {
        // Map old command names to new implementations
        if (CommandName == TEXT("create_blueprint"))
        {
            return FUnrealMCPCommandRegistry::Get().ExecuteCommand(TEXT("create_blueprint"), Parameters);
        }
        else if (CommandName == TEXT("add_component"))
        {
            return FUnrealMCPCommandRegistry::Get().ExecuteCommand(TEXT("add_component"), Parameters);
        }
        
        // Return error for unknown commands
        FMCPError Error(EMCPErrorType::CommandNotFound, 404, 
                       FString::Printf(TEXT("Unknown legacy command: %s"), *CommandName));
        return FMCPErrorHandler::CreateErrorResponse(Error);
    }
};
```

## Migration Checklist

### For Each Command

- [ ] Extract business logic to appropriate service class
- [ ] Create individual command class implementing `IUnrealMCPCommand`
- [ ] Define parameter structure with validation
- [ ] Implement proper error handling with structured errors
- [ ] Add unit tests for command and service
- [ ] Register command with registry
- [ ] Update integration tests
- [ ] Add performance monitoring if needed

### For Each Service

- [ ] Define service interface
- [ ] Implement concrete service class
- [ ] Add caching if appropriate
- [ ] Implement proper error handling
- [ ] Create mock implementation for testing
- [ ] Add comprehensive unit tests
- [ ] Document service API
- [ ] Consider thread safety requirements

### For the Overall System

- [ ] Update module initialization to register all commands
- [ ] Implement factory initialization
- [ ] Set up caching systems
- [ ] Update logging configuration
- [ ] Migrate integration tests
- [ ] Update documentation
- [ ] Plan rollout strategy
- [ ] Monitor performance impact

## Common Pitfalls

### 1. Not Separating Concerns

**Wrong:**
```cpp
class FMyCommand : public IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) override
    {
        // Don't put business logic directly in commands
        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(/*...*/);
        // This makes testing difficult
    }
};
```

**Right:**
```cpp
class FMyCommand : public IUnrealMCPCommand
{
private:
    TSharedPtr<IBlueprintService> Service;
    
public:
    virtual FString Execute(const FString& Parameters) override
    {
        // Commands should orchestrate, not implement business logic
        auto Params = ParseParameters(Parameters);
        auto Result = Service->CreateBlueprint(Params);
        return FormatResponse(Result);
    }
};
```

### 2. Ignoring Error Handling

**Wrong:**
```cpp
virtual FString Execute(const FString& Parameters) override
{
    auto Result = Service->DoSomething(Parameters);
    return TEXT("{\"success\": true}"); // What if it failed?
}
```

**Right:**
```cpp
virtual FString Execute(const FString& Parameters) override
{
    FString ErrorMessage;
    auto Result = Service->DoSomething(Parameters, ErrorMessage);
    
    if (!Result)
    {
        FMCPError Error(EMCPErrorType::ExecutionFailed, 500, ErrorMessage);
        return FMCPErrorHandler::CreateErrorResponse(Error);
    }
    
    return FMCPErrorHandler::CreateSuccessResponse(TEXT("Operation completed"));
}
```

### 3. Not Using Validation Framework

**Wrong:**
```cpp
virtual bool ValidateParams(const FString& Parameters) const override
{
    // Manual validation is error-prone and inconsistent
    TSharedPtr<FJsonObject> Json = ParseJson(Parameters);
    return Json.IsValid() && Json->HasField(TEXT("name"));
}
```

**Right:**
```cpp
class FMyCommand : public IUnrealMCPCommand
{
private:
    FParameterValidator Validator;
    
public:
    FMyCommand()
    {
        Validator.AddRule(FValidationRuleBuilder::RequiredString(TEXT("name")));
        Validator.AddRule(FValidationRuleBuilder::OptionalNumber(TEXT("timeout")));
    }
    
    virtual bool ValidateParams(const FString& Parameters) const override
    {
        TSharedPtr<FJsonObject> Json = ParseJson(Parameters);
        FString ErrorMessage;
        return Validator.ValidateParams(Json, ErrorMessage);
    }
};
```

This migration guide provides a comprehensive roadmap for transitioning from the old monolithic architecture to the new layered, maintainable system.