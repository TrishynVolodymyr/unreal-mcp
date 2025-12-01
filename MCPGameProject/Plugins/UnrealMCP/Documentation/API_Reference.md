# UnrealMCP Plugin API Reference

## Overview

The UnrealMCP plugin provides a comprehensive Model Context Protocol (MCP) integration for Unreal Engine 5.7, enabling AI assistants to control Unreal Engine through natural language commands. The plugin follows a layered architecture with clear separation of concerns.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Command Layer                            │
│  Individual command classes implementing IUnrealMCPCommand  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Service Layer                             │
│  Business logic services (Blueprint, Component, Property)  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                  Factory Layer                              │
│  Component and Widget factories for type-safe creation     │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                 Utility Layer                               │
│  Validation, Error Handling, Logging, and Common Utils     │
└─────────────────────────────────────────────────────────────┘
```

## Core Interfaces

### IUnrealMCPCommand

The base interface for all MCP commands that can be executed by the UnrealMCP system.

```cpp
class UNREALMCP_API IUnrealMCPCommand
{
public:
    virtual ~IUnrealMCPCommand() = default;
    
    /**
     * Execute the command with the provided parameters
     * @param Parameters JSON string containing command parameters
     * @return JSON string containing the command result
     */
    virtual FString Execute(const FString& Parameters) = 0;
    
    /**
     * Get the name/identifier of this command
     * @return Command name used for registration and lookup
     */
    virtual FString GetCommandName() const = 0;
    
    /**
     * Validate the provided parameters before execution
     * @param Parameters JSON string containing command parameters
     * @return True if parameters are valid, false otherwise
     */
    virtual bool ValidateParams(const FString& Parameters) const = 0;
};
```

**Usage Example:**
```cpp
class FMyCustomCommand : public IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("my_custom_command"); }
    virtual bool ValidateParams(const FString& Parameters) const override;
};
```

## Service Layer Interfaces

### IBlueprintService

Interface for Blueprint service operations providing abstraction for Blueprint creation, modification, and management.

```cpp
class UNREALMCP_API IBlueprintService
{
public:
    virtual ~IBlueprintService() = default;
    
    /**
     * Create a new Blueprint
     * @param Params Blueprint creation parameters
     * @return Created Blueprint or nullptr if failed
     */
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) = 0;
    
    /**
     * Add a component to an existing Blueprint
     * @param Blueprint Target Blueprint
     * @param Params Component creation parameters
     * @return true if component was added successfully
     */
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params) = 0;
    
    /**
     * Compile a Blueprint
     * @param Blueprint Blueprint to compile
     * @param OutError Error message if compilation fails
     * @return true if compilation succeeded
     */
    virtual bool CompileBlueprint(UBlueprint* Blueprint, FString& OutError) = 0;
    
    /**
     * Find a Blueprint by name
     * @param BlueprintName Name of the Blueprint to find
     * @return Found Blueprint or nullptr
     */
    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) = 0;
    
    /**
     * Add a variable to a Blueprint
     * @param Blueprint Target Blueprint
     * @param VariableName Name of the variable
     * @param VariableType Type of the variable
     * @param bIsExposed Whether the variable should be exposed to the editor
     * @return true if variable was added successfully
     */
    virtual bool AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, 
                                       const FString& VariableType, bool bIsExposed = false) = 0;
    
    /**
     * Set a property on a Blueprint's default object
     * @param Blueprint Target Blueprint
     * @param PropertyName Name of the property
     * @param PropertyValue Value to set (as JSON)
     * @return true if property was set successfully
     */
    virtual bool SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, 
                                     const TSharedPtr<FJsonValue>& PropertyValue) = 0;
};
```

### IComponentService

Interface for component-related operations providing abstraction for component creation, modification, and management.

```cpp
class UNREALMCP_API IComponentService
{
public:
    virtual ~IComponentService() = default;
    
    /**
     * Add a component to an existing Blueprint
     * @param Blueprint Target Blueprint
     * @param Params Component creation parameters
     * @return true if component was added successfully
     */
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params) = 0;
    
    /**
     * Remove a component from a Blueprint
     * @param Blueprint Target Blueprint
     * @param ComponentName Name of the component to remove
     * @return true if component was removed successfully
     */
    virtual bool RemoveComponentFromBlueprint(UBlueprint* Blueprint, const FString& ComponentName) = 0;
    
    /**
     * Find a component in a Blueprint by name
     * @param Blueprint Target Blueprint
     * @param ComponentName Name of the component to find
     * @return Component template object or nullptr if not found
     */
    virtual UObject* FindComponentInBlueprint(UBlueprint* Blueprint, const FString& ComponentName) = 0;
    
    /**
     * Get all components in a Blueprint
     * @param Blueprint Target Blueprint
     * @return Array of component names and types
     */
    virtual TArray<TPair<FString, FString>> GetBlueprintComponents(UBlueprint* Blueprint) = 0;
    
    /**
     * Validate if a component type is supported
     * @param ComponentType Type of component to validate
     * @return true if component type is supported
     */
    virtual bool IsValidComponentType(const FString& ComponentType) = 0;
    
    /**
     * Get the UClass for a component type string
     * @param ComponentType String representation of component type
     * @return UClass pointer or nullptr if not found
     */
    virtual UClass* GetComponentClass(const FString& ComponentType) = 0;
};
```

### IPropertyService

Interface for property management operations providing type-safe property setting and validation.

```cpp
class UNREALMCP_API IPropertyService
{
public:
    virtual ~IPropertyService() = default;
    
    /**
     * Set a property on an object from JSON value
     * @param Object Target object
     * @param PropertyName Name of the property to set
     * @param PropertyValue JSON value to set
     * @param OutError Error message if setting fails
     * @return true if property was set successfully
     */
    virtual bool SetPropertyFromJson(UObject* Object, const FString& PropertyName, 
                                   const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;
    
    /**
     * Get a property value from an object as JSON
     * @param Object Source object
     * @param PropertyName Name of the property to get
     * @param OutValue JSON value of the property
     * @return true if property was retrieved successfully
     */
    virtual bool GetPropertyAsJson(UObject* Object, const FString& PropertyName, 
                                 TSharedPtr<FJsonValue>& OutValue) = 0;
    
    /**
     * Validate if a property exists on an object
     * @param Object Target object
     * @param PropertyName Name of the property to check
     * @return true if property exists
     */
    virtual bool HasProperty(UObject* Object, const FString& PropertyName) = 0;
    
    /**
     * Get the type of a property
     * @param Object Target object
     * @param PropertyName Name of the property
     * @return String representation of the property type
     */
    virtual FString GetPropertyType(UObject* Object, const FString& PropertyName) = 0;
};
```

## Factory Pattern Classes

### FComponentFactory

Factory class for creating and managing Unreal Engine component types using the Singleton pattern.

```cpp
class UNREALMCP_API FComponentFactory
{
public:
    /**
     * Get the singleton instance of the component factory
     * @return Reference to the singleton instance
     */
    static FComponentFactory& Get();
    
    /**
     * Register a component type with the factory
     * @param TypeName String identifier for the component type
     * @param ComponentClass UClass pointer for the component type
     */
    void RegisterComponentType(const FString& TypeName, UClass* ComponentClass);
    
    /**
     * Get the UClass for a registered component type
     * @param TypeName String identifier for the component type
     * @return UClass pointer for the component type, or nullptr if not found
     */
    UClass* GetComponentClass(const FString& TypeName) const;
    
    /**
     * Get all available component type names
     * @return Array of registered component type names
     */
    TArray<FString> GetAvailableTypes() const;
    
    /**
     * Initialize the factory with default Unreal Engine component types
     */
    void InitializeDefaultTypes();
};
```

**Usage Example:**
```cpp
// Register a custom component type
FComponentFactory::Get().RegisterComponentType(TEXT("MyCustomComponent"), UMyCustomComponent::StaticClass());

// Get a component class
UClass* ComponentClass = FComponentFactory::Get().GetComponentClass(TEXT("StaticMeshComponent"));

// List available types
TArray<FString> AvailableTypes = FComponentFactory::Get().GetAvailableTypes();
```

### FWidgetFactory

Factory class for creating and managing UMG widget types using the Singleton pattern.

```cpp
class UNREALMCP_API FWidgetFactory
{
public:
    /**
     * Get the singleton instance of the widget factory
     * @return Reference to the singleton instance
     */
    static FWidgetFactory& Get();
    
    /**
     * Create a widget of the specified type
     * @param WidgetType String identifier for the widget type
     * @param WidgetName Name for the widget instance
     * @return Created widget or nullptr if failed
     */
    UWidget* CreateWidget(const FString& WidgetType, const FString& WidgetName);
    
    /**
     * Register a widget type with the factory
     * @param TypeName String identifier for the widget type
     * @param WidgetClass UClass pointer for the widget type
     */
    void RegisterWidgetType(const FString& TypeName, UClass* WidgetClass);
    
    /**
     * Get all available widget type names
     * @return Array of registered widget type names
     */
    TArray<FString> GetAvailableWidgetTypes() const;
    
    /**
     * Initialize the factory with default UMG widget types
     */
    void InitializeDefaultWidgetTypes();
};
```

## Error Handling System

### FMCPError

Structure representing an MCP error with type, code, and message for comprehensive error reporting.

```cpp
USTRUCT(BlueprintType)
struct UNREALMCP_API FMCPError
{
    GENERATED_BODY()
    
    /** Type of error that occurred */
    UPROPERTY(BlueprintReadOnly)
    EMCPErrorType ErrorType;
    
    /** Numeric error code for programmatic handling */
    UPROPERTY(BlueprintReadOnly)
    int32 ErrorCode;
    
    /** Human-readable error message */
    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;
    
    /** Additional context or details about the error */
    UPROPERTY(BlueprintReadOnly)
    FString ErrorDetails;
    
    /** Check if this represents an error condition */
    bool HasError() const;
    
    /** Convert error to JSON string for transmission */
    FString ToJsonString() const;
    
    /** Create error from JSON string */
    static FMCPError FromJsonString(const FString& JsonString);
};
```

### FMCPErrorHandler

Static utility class for creating standardized error responses.

```cpp
class UNREALMCP_API FMCPErrorHandler
{
public:
    /**
     * Create a standardized error response JSON object
     * @param Error The error to convert to response
     * @return JSON object containing error information
     */
    static TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error);
    
    /**
     * Create a validation error response
     * @param Message Error message
     * @param FieldErrors Map of field names to error messages
     * @return JSON object containing validation error
     */
    static TSharedPtr<FJsonObject> CreateValidationError(const FString& Message, 
                                                        const TMap<FString, FString>& FieldErrors = {});
    
    /**
     * Create a not found error response
     * @param ResourceType Type of resource that was not found
     * @param ResourceName Name of the resource that was not found
     * @return JSON object containing not found error
     */
    static TSharedPtr<FJsonObject> CreateNotFoundError(const FString& ResourceType, const FString& ResourceName);
    
    /**
     * Create a success response
     * @param Message Success message
     * @param Data Optional data to include in response
     * @return JSON object containing success response
     */
    static TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& Message = TEXT(""), 
                                                        const TSharedPtr<FJsonObject>& Data = nullptr);
};
```

## Parameter Validation Framework

### FParameterValidator

Parameter validation framework providing flexible validation rules system with common validation functions.

```cpp
class UNREALMCP_API FParameterValidator
{
public:
    /**
     * Add a validation rule to this validator
     * @param Rule The validation rule to add
     */
    void AddRule(const FValidationRule& Rule);
    
    /**
     * Validate parameters against all registered rules
     * @param Params The JSON parameters to validate
     * @param OutError Error message if validation fails
     * @return true if all validations pass, false otherwise
     */
    bool ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;
    
    /**
     * Clear all validation rules
     */
    void ClearRules();
    
    // Static validation functions for common types
    static bool IsValidString(const TSharedPtr<FJsonValue>& Value);
    static bool IsValidNumber(const TSharedPtr<FJsonValue>& Value);
    static bool IsValidArray(const TSharedPtr<FJsonValue>& Value);
    static bool IsValidBoolean(const TSharedPtr<FJsonValue>& Value);
    static bool IsValidObject(const TSharedPtr<FJsonValue>& Value);
    static bool MatchesPattern(const TSharedPtr<FJsonValue>& Value, const FString& Pattern);
    static bool IsInRange(const TSharedPtr<FJsonValue>& Value, double MinValue, double MaxValue);
    static bool HasMinElements(const TSharedPtr<FJsonValue>& Value, int32 MinElements);
    static bool HasMaxElements(const TSharedPtr<FJsonValue>& Value, int32 MaxElements);
};
```

### FValidationRuleBuilder

Helper class for building common validation rules with predefined patterns.

```cpp
class UNREALMCP_API FValidationRuleBuilder
{
public:
    static FValidationRule RequiredString(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule OptionalString(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule RequiredNumber(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule OptionalNumber(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule RequiredArray(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule OptionalArray(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule RequiredBoolean(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    static FValidationRule OptionalBoolean(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
};
```

**Usage Example:**
```cpp
FParameterValidator Validator;
Validator.AddRule(FValidationRuleBuilder::RequiredString(TEXT("blueprint_name")));
Validator.AddRule(FValidationRuleBuilder::OptionalString(TEXT("folder_path")));
Validator.AddRule(FValidationRuleBuilder::RequiredBoolean(TEXT("compile_on_creation")));

FString ErrorMessage;
if (!Validator.ValidateParams(JsonParams, ErrorMessage))
{
    // Handle validation error
    UE_LOG(LogTemp, Error, TEXT("Validation failed: %s"), *ErrorMessage);
}
```

## Command Registry System

### FUnrealMCPCommandRegistry

Central registry for managing and executing MCP commands with thread-safe operations.

```cpp
class UNREALMCP_API FUnrealMCPCommandRegistry
{
public:
    /**
     * Get the singleton instance of the command registry
     * @return Reference to the singleton instance
     */
    static FUnrealMCPCommandRegistry& Get();
    
    /**
     * Register a command with the registry
     * @param CommandName Name of the command
     * @param Command Shared pointer to the command implementation
     */
    void RegisterCommand(const FString& CommandName, TSharedPtr<IUnrealMCPCommand> Command);
    
    /**
     * Get a registered command by name
     * @param CommandName Name of the command to retrieve
     * @return Shared pointer to the command or nullptr if not found
     */
    TSharedPtr<IUnrealMCPCommand> GetCommand(const FString& CommandName) const;
    
    /**
     * Execute a command by name with parameters
     * @param CommandName Name of the command to execute
     * @param Parameters JSON parameters for the command
     * @return JSON response from the command execution
     */
    FString ExecuteCommand(const FString& CommandName, const FString& Parameters);
    
    /**
     * Get all registered command names
     * @return Array of registered command names
     */
    TArray<FString> GetRegisteredCommands() const;
    
    /**
     * Check if a command is registered
     * @param CommandName Name of the command to check
     * @return true if command is registered
     */
    bool IsCommandRegistered(const FString& CommandName) const;
    
    /**
     * Unregister a command
     * @param CommandName Name of the command to unregister
     * @return true if command was unregistered
     */
    bool UnregisterCommand(const FString& CommandName);
    
    /**
     * Clear all registered commands
     */
    void ClearAllCommands();
};
```

## Performance and Caching

### Object Pooling System

Template-based object pooling system for frequently created objects to improve performance.

```cpp
template<typename T>
class UNREALMCP_API TObjectPool
{
public:
    /**
     * Acquire an object from the pool
     * @return Shared pointer to an object (new or reused)
     */
    TSharedPtr<T> Acquire();
    
    /**
     * Release an object back to the pool
     * @param Object Object to return to the pool
     */
    void Release(TSharedPtr<T> Object);
    
    /**
     * Get the current pool size
     * @return Number of objects in the pool
     */
    int32 GetPoolSize() const;
    
    /**
     * Clear all objects from the pool
     */
    void Clear();
    
    /**
     * Set the maximum pool size
     * @param MaxSize Maximum number of objects to keep in pool
     */
    void SetMaxPoolSize(int32 MaxSize);
};
```

### Blueprint Caching System

Thread-safe caching system for Blueprint objects to improve lookup performance.

```cpp
class UNREALMCP_API FBlueprintCache
{
public:
    /**
     * Get a Blueprint from cache or load if not cached
     * @param BlueprintName Name of the Blueprint
     * @return Blueprint object or nullptr if not found
     */
    UBlueprint* GetBlueprint(const FString& BlueprintName);
    
    /**
     * Invalidate a cached Blueprint
     * @param BlueprintName Name of the Blueprint to invalidate
     */
    void InvalidateBlueprint(const FString& BlueprintName);
    
    /**
     * Clear all cached Blueprints
     */
    void ClearCache();
    
    /**
     * Get cache statistics
     * @return JSON object containing cache statistics
     */
    TSharedPtr<FJsonObject> GetCacheStats() const;
    
    /**
     * Warm the cache with commonly used Blueprints
     * @param BlueprintNames Array of Blueprint names to preload
     */
    void WarmCache(const TArray<FString>& BlueprintNames);
};
```

## Best Practices

### Command Implementation

1. **Always validate parameters** before execution using the validation framework
2. **Use structured error responses** with appropriate error types and messages
3. **Implement proper logging** for debugging and monitoring
4. **Follow the single responsibility principle** - one command should do one thing well
5. **Use the service layer** for business logic, keep commands focused on request/response handling

### Service Implementation

1. **Design for testability** with mockable interfaces
2. **Use dependency injection** where possible
3. **Implement proper error handling** with detailed error messages
4. **Cache expensive operations** using the provided caching systems
5. **Follow thread safety practices** when accessing shared resources

### Factory Usage

1. **Register all custom types** during plugin initialization
2. **Use type-safe creation methods** provided by factories
3. **Handle creation failures gracefully** with proper error reporting
4. **Initialize default types** before using factories

### Error Handling

1. **Use structured error types** for consistent error reporting
2. **Provide detailed error context** to help with debugging
3. **Log errors appropriately** with proper severity levels
4. **Return user-friendly error messages** while maintaining technical details for debugging

## Migration Guide

When migrating from the old monolithic command handlers to the new architecture:

1. **Extract business logic** to appropriate service classes
2. **Create individual command classes** implementing `IUnrealMCPCommand`
3. **Use the validation framework** instead of manual parameter checking
4. **Replace direct error strings** with structured `FMCPError` objects
5. **Register commands** with the command registry
6. **Update tests** to use the new interfaces and mock implementations

## Thread Safety

The UnrealMCP plugin is designed to be thread-safe where necessary:

- **Command Registry**: Thread-safe registration and lookup
- **Factory Classes**: Thread-safe type registration and creation
- **Caching Systems**: Thread-safe cache operations with proper locking
- **Object Pools**: Thread-safe acquire/release operations

Always use the provided thread-safe mechanisms when implementing custom extensions.