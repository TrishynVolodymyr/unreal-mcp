# Design Document

## Overview

The UnrealMCP plugin currently suffers from several large monolithic command handler classes (1800+ lines each) that violate SOLID principles and make the codebase difficult to maintain. This design outlines a comprehensive refactoring approach using established design patterns to create a more maintainable, testable, and extensible architecture.

The refactoring will transform the current procedural command handlers into a layered architecture with clear separation of concerns, following industry best practices for C++ and Unreal Engine development.

## Architecture

### Current Architecture Issues

**Code Smells Identified:**
- **God Classes**: `FUnrealMCPBlueprintCommands` (1854 lines), `FUnrealMCPUMGCommands` (2161 lines)
- **Long Methods**: Individual command handlers often exceed 100-200 lines
- **Duplicate Code**: Parameter validation, error handling, and response formatting repeated across methods
- **Mixed Responsibilities**: Command parsing, business logic, and Unreal Engine API calls in same methods
- **Hard Dependencies**: Direct coupling to Unreal Engine classes throughout command handlers

### New Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Command Layer                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Blueprint       │  │ UMG             │  │ Editor      │ │
│  │ CommandHandler  │  │ CommandHandler  │  │ CommandHandler│ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Service Layer                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Blueprint       │  │ Component       │  │ Property    │ │
│  │ Service         │  │ Service         │  │ Service     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                  Factory Layer                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Component       │  │ Widget          │  │ Node        │ │
│  │ Factory         │  │ Factory         │  │ Factory     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                 Utility Layer                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Validation      │  │ JSON            │  │ Asset       │ │
│  │ Utils           │  │ Utils           │  │ Utils       │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. Command Pattern Implementation

**Base Command Interface:**
```cpp
class UNREALMCP_API IUnrealMCPCommand
{
public:
    virtual ~IUnrealMCPCommand() = default;
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params) = 0;
    virtual FString GetCommandName() const = 0;
    virtual bool ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const = 0;
};
```

**Command Registry:**
```cpp
class UNREALMCP_API FUnrealMCPCommandRegistry
{
public:
    void RegisterCommand(const FString& CommandName, TSharedPtr<IUnrealMCPCommand> Command);
    TSharedPtr<IUnrealMCPCommand> GetCommand(const FString& CommandName) const;
    TSharedPtr<FJsonObject> ExecuteCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params);

private:
    TMap<FString, TSharedPtr<IUnrealMCPCommand>> Commands;
};
```

### 2. Service Layer Architecture

**Blueprint Service Interface:**
```cpp
class UNREALMCP_API IBlueprintService
{
public:
    virtual ~IBlueprintService() = default;
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) = 0;
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params) = 0;
    virtual bool CompileBlueprint(UBlueprint* Blueprint, FString& OutError) = 0;
    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) = 0;
};
```

**Component Service Interface:**
```cpp
class UNREALMCP_API IComponentService
{
public:
    virtual ~IComponentService() = default;
    virtual UActorComponent* CreateComponent(UClass* ComponentClass, const FString& ComponentName) = 0;
    virtual bool SetComponentProperty(UActorComponent* Component, const FString& PropertyName, const TSharedPtr<FJsonValue>& Value) = 0;
    virtual TArray<FString> GetAvailableComponentTypes() const = 0;
};
```

### 3. Factory Pattern for Component Creation

**Component Factory:**
```cpp
class UNREALMCP_API FComponentFactory
{
public:
    static FComponentFactory& Get();
    
    void RegisterComponentType(const FString& TypeName, UClass* ComponentClass);
    UClass* GetComponentClass(const FString& TypeName) const;
    TArray<FString> GetAvailableTypes() const;
    
    // Specialized creation methods
    UActorComponent* CreateComponent(const FString& TypeName, const FString& ComponentName);
    
private:
    TMap<FString, UClass*> ComponentTypeMap;
    void InitializeDefaultTypes();
};
```

**Widget Factory:**
```cpp
class UNREALMCP_API FWidgetFactory
{
public:
    static FWidgetFactory& Get();
    
    UWidget* CreateWidget(const FString& WidgetType, const FString& WidgetName);
    void RegisterWidgetType(const FString& TypeName, UClass* WidgetClass);
    TArray<FString> GetAvailableWidgetTypes() const;
    
private:
    TMap<FString, UClass*> WidgetTypeMap;
    void InitializeDefaultWidgetTypes();
};
```

### 4. Validation Framework

**Parameter Validator:**
```cpp
class UNREALMCP_API FParameterValidator
{
public:
    struct FValidationRule
    {
        FString FieldName;
        bool bRequired;
        TFunction<bool(const TSharedPtr<FJsonValue>&)> ValidationFunc;
        FString ErrorMessage;
    };
    
    void AddRule(const FValidationRule& Rule);
    bool ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;
    
    // Common validation functions
    static bool IsValidString(const TSharedPtr<FJsonValue>& Value);
    static bool IsValidNumber(const TSharedPtr<FJsonValue>& Value);
    static bool IsValidArray(const TSharedPtr<FJsonValue>& Value);
    
private:
    TArray<FValidationRule> ValidationRules;
};
```

### 5. Error Handling System

**Structured Error Response:**
```cpp
struct UNREALMCP_API FMCPError
{
    enum class EErrorType
    {
        ValidationError,
        NotFoundError,
        OperationError,
        InternalError
    };
    
    EErrorType Type;
    FString Message;
    FString Details;
    TMap<FString, FString> Context;
    
    TSharedPtr<FJsonObject> ToJson() const;
};

class UNREALMCP_API FMCPErrorHandler
{
public:
    static TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error);
    static TSharedPtr<FJsonObject> CreateValidationError(const FString& Message, const TMap<FString, FString>& FieldErrors = {});
    static TSharedPtr<FJsonObject> CreateNotFoundError(const FString& ResourceType, const FString& ResourceName);
};
```

## Data Models

### Configuration Structures

**Blueprint Creation Parameters:**
```cpp
struct UNREALMCP_API FBlueprintCreationParams
{
    FString Name;
    FString FolderPath;
    UClass* ParentClass;
    bool bCompileOnCreation = true;
    
    bool IsValid(FString& OutError) const;
};
```

**Component Creation Parameters:**
```cpp
struct UNREALMCP_API FComponentCreationParams
{
    FString ComponentType;
    FString ComponentName;
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Scale = FVector::OneVector;
    TSharedPtr<FJsonObject> Properties;
    
    bool IsValid(FString& OutError) const;
};
```

### Response Models

**Standard Response Format:**
```cpp
struct UNREALMCP_API FMCPResponse
{
    bool bSuccess;
    FString Message;
    TSharedPtr<FJsonObject> Data;
    TArray<FMCPError> Errors;
    
    TSharedPtr<FJsonObject> ToJson() const;
    static FMCPResponse Success(const FString& Message = TEXT(""), const TSharedPtr<FJsonObject>& Data = nullptr);
    static FMCPResponse Error(const FMCPError& Error);
};
```

## Error Handling

### Centralized Error Management

**Error Categories:**
1. **Validation Errors**: Invalid parameters, missing required fields
2. **Resource Errors**: Blueprint/Component not found, asset loading failures
3. **Operation Errors**: Compilation failures, property setting errors
4. **System Errors**: Unreal Engine API failures, memory allocation errors

**Error Handling Strategy:**
```cpp
class UNREALMCP_API FMCPOperationContext
{
public:
    void AddError(const FMCPError& Error);
    void AddWarning(const FString& Warning);
    bool HasErrors() const { return Errors.Num() > 0; }
    TSharedPtr<FJsonObject> CreateResponse() const;
    
private:
    TArray<FMCPError> Errors;
    TArray<FString> Warnings;
};
```

### Logging Strategy

**Structured Logging:**
```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogUnrealMCP, Log, All);

#define UE_LOG_MCP_ERROR(Format, ...) UE_LOG(LogUnrealMCP, Error, Format, ##__VA_ARGS__)
#define UE_LOG_MCP_WARNING(Format, ...) UE_LOG(LogUnrealMCP, Warning, Format, ##__VA_ARGS__)
#define UE_LOG_MCP_INFO(Format, ...) UE_LOG(LogUnrealMCP, Log, Format, ##__VA_ARGS__)
#define UE_LOG_MCP_VERBOSE(Format, ...) UE_LOG(LogUnrealMCP, Verbose, Format, ##__VA_ARGS__)
```

## Testing Strategy

### Unit Testing Framework

**Testable Service Interfaces:**
```cpp
class UNREALMCP_API FMockBlueprintService : public IBlueprintService
{
public:
    // Mock implementations for testing
    MOCK_METHOD(UBlueprint*, CreateBlueprint, (const FBlueprintCreationParams& Params), (override));
    MOCK_METHOD(bool, AddComponentToBlueprint, (UBlueprint* Blueprint, const FComponentCreationParams& Params), (override));
    // ... other mock methods
};
```

**Test Utilities:**
```cpp
class UNREALMCP_API FMCPTestUtils
{
public:
    static TSharedPtr<FJsonObject> CreateTestParams(const TMap<FString, FString>& StringParams);
    static void AssertResponseSuccess(const TSharedPtr<FJsonObject>& Response);
    static void AssertResponseError(const TSharedPtr<FJsonObject>& Response, const FString& ExpectedError);
};
```

### Integration Testing

**Test Scenarios:**
1. **Command Registration**: Verify all commands are properly registered
2. **Parameter Validation**: Test validation rules for each command
3. **Error Handling**: Verify proper error responses for various failure scenarios
4. **Blueprint Operations**: Test blueprint creation, modification, and compilation
5. **Component Management**: Test component creation and property setting

## Performance Optimizations

### Caching Strategy

**Blueprint Cache:**
```cpp
class UNREALMCP_API FBlueprintCache
{
public:
    UBlueprint* GetBlueprint(const FString& BlueprintName);
    void InvalidateBlueprint(const FString& BlueprintName);
    void ClearCache();
    
private:
    TMap<FString, TWeakObjectPtr<UBlueprint>> CachedBlueprints;
    FCriticalSection CacheLock;
};
```

**Component Type Cache:**
```cpp
class UNREALMCP_API FComponentTypeCache
{
public:
    UClass* GetComponentClass(const FString& TypeName);
    void RefreshCache();
    
private:
    TMap<FString, UClass*> TypeCache;
    bool bCacheInitialized = false;
};
```

### Memory Management

**Smart Pointer Usage:**
- Use `TSharedPtr` for JSON objects and responses
- Use `TWeakObjectPtr` for Unreal Engine object references
- Use `TUniquePtr` for service implementations

**Object Pooling:**
```cpp
template<typename T>
class UNREALMCP_API TObjectPool
{
public:
    TSharedPtr<T> Acquire();
    void Release(TSharedPtr<T> Object);
    
private:
    TArray<TSharedPtr<T>> AvailableObjects;
    FCriticalSection PoolLock;
};
```

## Migration Strategy

### Phase 1: Foundation (High Priority)
1. Create base interfaces and abstract classes
2. Implement command registry and basic command pattern
3. Create validation framework
4. Implement error handling system

### Phase 2: Service Layer (High Priority)
1. Extract Blueprint service from existing command handler
2. Extract Component service
3. Extract Property service
4. Implement factory patterns

### Phase 3: Command Refactoring (Medium Priority)
1. Refactor Blueprint commands to use new architecture
2. Refactor UMG commands
3. Refactor Editor commands
4. Update all command handlers

### Phase 4: Optimization (Low Priority)
1. Implement caching systems
2. Add performance monitoring
3. Optimize memory usage
4. Add comprehensive logging

### Backward Compatibility

**API Compatibility Layer:**
```cpp
class UNREALMCP_API FLegacyCommandAdapter
{
public:
    static TSharedPtr<FJsonObject> HandleLegacyCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);
    
private:
    static TMap<FString, FString> LegacyCommandMapping;
};
```

This design ensures that existing MCP integrations continue to work while providing a path forward for improved maintainability and extensibility.