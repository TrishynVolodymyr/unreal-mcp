---
inclusion: always
---

# Unreal MCP Architecture Guidelines

## Overview

This document provides guidance for implementing and maintaining the UnrealMCP plugin architecture. All new code and modifications to existing code should follow these architectural principles to ensure maintainability, testability, and extensibility.

## Layered Architecture

The UnrealMCP plugin follows a layered architecture with clear separation of concerns:

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

## Key Design Patterns

### 1. Command Pattern

All MCP tools must implement the Command Pattern using the `IUnrealMCPCommand` interface:

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

**Implementation Guidelines:**
- Create a separate command class for each MCP tool
- Register commands with `FUnrealMCPCommandRegistry`
- Implement parameter validation in `ValidateParams`
- Delegate business logic to appropriate service classes

### 2. Service Layer

Business logic should be extracted into service classes with clear interfaces:

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

**Implementation Guidelines:**
- Follow single responsibility principle
- Create interfaces for all services to enable testing
- Use dependency injection for service dependencies
- Avoid direct Unreal Engine API calls in command handlers

### 3. Factory Pattern

Use factory classes for object creation:

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
};
```

**Implementation Guidelines:**
- Use `ComponentFactory` for Blueprint component creation
- Use `WidgetFactory` for UMG widget creation
- Register component and widget types during initialization
- Avoid direct `NewObject<>` calls outside factories

### 4. Validation Framework

Use the validation framework for parameter validation:

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
};
```

**Implementation Guidelines:**
- Define validation rules for each command
- Use common validation functions where possible
- Provide clear error messages for validation failures
- Validate all parameters before execution

### 5. Error Handling System

Use structured error handling:

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
```

**Implementation Guidelines:**
- Categorize errors appropriately
- Include detailed context information
- Use `FMCPErrorHandler` for creating error responses
- Log errors with appropriate severity

### 6. Caching Strategy

Implement appropriate caching:

```cpp
class UNREALMCP_API FBlueprintCache
{
public:
    UBlueprint* GetBlueprint(const FString& BlueprintName);
    void InvalidateBlueprint(const FString& BlueprintName);
    void ClearCache();
};
```

**Implementation Guidelines:**
- Cache frequently accessed resources
- Implement proper cache invalidation
- Use weak pointers for UObject references
- Ensure thread safety with critical sections

## Code Organization

### File Structure

- **Public Headers**: `/Public/Commands/`, `/Public/Services/`, `/Public/Factories/`, `/Public/Utils/`
- **Private Implementation**: `/Private/Commands/`, `/Private/Services/`, `/Private/Factories/`, `/Private/Utils/`
- **Module**: `/Private/UnrealMCPModule.cpp`

### Naming Conventions

- **Commands**: `F[Category][Action]Command` (e.g., `FBlueprintCreateCommand`)
- **Services**: `F[Category]Service` (e.g., `FBlueprintService`)
- **Interfaces**: `I[Category]Service` (e.g., `IBlueprintService`)
- **Factories**: `F[Category]Factory` (e.g., `FComponentFactory`)
- **Utilities**: `F[Category]Utils` (e.g., `FValidationUtils`)

## Testing Guidelines

- Create mock implementations of service interfaces for unit testing
- Test command validation separately from execution
- Use dependency injection to replace services with mocks
- Test error handling paths thoroughly
- Verify proper service delegation in command handlers

## Migration Guidelines

When migrating existing code to the new architecture:

1. Create appropriate service interfaces first
2. Extract business logic into service implementations
3. Create command classes implementing `IUnrealMCPCommand`
4. Update command registration in the module
5. Implement validation using `FParameterValidator`
6. Update error handling to use `FMCPError`
7. Add appropriate caching where beneficial

## Example Implementation

### Command Class

```cpp
class UNREALMCP_API FBlueprintCreateCommand : public IUnrealMCPCommand
{
public:
    FBlueprintCreateCommand(TSharedPtr<IBlueprintService> InBlueprintService)
        : BlueprintService(InBlueprintService)
    {
    }

    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params) override
    {
        FString ErrorMessage;
        if (!ValidateParams(Params, ErrorMessage))
        {
            return FMCPErrorHandler::CreateValidationError(ErrorMessage);
        }

        FBlueprintCreationParams CreationParams;
        // Parse parameters...

        UBlueprint* Blueprint = BlueprintService->CreateBlueprint(CreationParams);
        if (!Blueprint)
        {
            return FMCPErrorHandler::CreateOperationError("Failed to create Blueprint");
        }

        // Create success response...
        return Response->ToJson();
    }

    virtual FString GetCommandName() const override
    {
        return TEXT("create_blueprint");
    }

    virtual bool ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const override
    {
        FParameterValidator Validator;
        Validator.AddRule({ TEXT("name"), true, FParameterValidator::IsValidString, TEXT("Name must be a valid string") });
        Validator.AddRule({ TEXT("parent_class"), true, FParameterValidator::IsValidString, TEXT("Parent class must be a valid string") });
        
        return Validator.ValidateParams(Params, OutError);
    }

private:
    TSharedPtr<IBlueprintService> BlueprintService;
};
```

### Service Implementation

```cpp
class UNREALMCP_API FBlueprintService : public IBlueprintService
{
public:
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override
    {
        // Implementation...
    }

    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params) override
    {
        // Implementation...
    }

    virtual bool CompileBlueprint(UBlueprint* Blueprint, FString& OutError) override
    {
        // Implementation...
    }

    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) override
    {
        // Check cache first
        UBlueprint* CachedBlueprint = FBlueprintCache::Get().GetBlueprint(BlueprintName);
        if (CachedBlueprint)
        {
            return CachedBlueprint;
        }

        // Find blueprint...
        
        // Add to cache
        FBlueprintCache::Get().AddBlueprint(BlueprintName, Blueprint);
        
        return Blueprint;
    }
};
```

## Conclusion

Following these architectural guidelines will ensure that the UnrealMCP plugin remains maintainable, testable, and extensible. All new code and modifications to existing code should adhere to these principles to maintain architectural consistency.