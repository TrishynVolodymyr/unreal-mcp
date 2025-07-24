# UnrealMCP Plugin Architecture Guide

## Overview

The UnrealMCP plugin has been refactored from monolithic command handlers into a clean, layered architecture following SOLID principles and established design patterns. This guide explains the architectural decisions, design patterns used, and how the components interact.

## Architectural Principles

### SOLID Principles Implementation

1. **Single Responsibility Principle (SRP)**
   - Each command class handles only one specific operation
   - Service classes focus on a single domain (Blueprint, Component, Property)
   - Factory classes handle only object creation for their specific type

2. **Open/Closed Principle (OCP)**
   - New commands can be added without modifying existing code
   - Service interfaces allow for different implementations
   - Factory pattern enables easy extension of supported types

3. **Liskov Substitution Principle (LSP)**
   - All command implementations are interchangeable through `IUnrealMCPCommand`
   - Service implementations can be substituted through their interfaces
   - Mock implementations can replace real services for testing

4. **Interface Segregation Principle (ISP)**
   - Separate interfaces for different service types (Blueprint, Component, Property)
   - Commands only depend on the interfaces they actually use
   - Clients aren't forced to depend on methods they don't use

5. **Dependency Inversion Principle (DIP)**
   - High-level command classes depend on service abstractions
   - Service implementations depend on Unreal Engine abstractions
   - Dependencies are injected rather than hard-coded

## Layered Architecture

### Layer 1: Command Layer

**Purpose**: Handle HTTP requests, parameter parsing, and response formatting

**Components**:
- Individual command classes implementing `IUnrealMCPCommand`
- Command registry for registration and lookup
- Parameter validation using the validation framework
- Response formatting using error handling utilities

**Responsibilities**:
- Parse incoming JSON parameters
- Validate parameters using validation rules
- Delegate business logic to service layer
- Format responses (success or error)
- Handle command-specific concerns

**Example**:
```cpp
class FCreateBlueprintCommand : public IUnrealMCPCommand
{
private:
    TSharedPtr<IBlueprintService> BlueprintService;
    FParameterValidator Validator;
    
public:
    FCreateBlueprintCommand(TSharedPtr<IBlueprintService> InBlueprintService);
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;
};
```

### Layer 2: Service Layer

**Purpose**: Implement business logic and coordinate operations

**Components**:
- `IBlueprintService` and `FBlueprintService`
- `IComponentService` and `FComponentService`
- `IPropertyService` and `FPropertyService`
- Service-specific parameter structures

**Responsibilities**:
- Implement core business logic
- Coordinate between different Unreal Engine systems
- Provide caching and performance optimizations
- Handle service-specific error conditions
- Abstract Unreal Engine API complexity

**Example**:
```cpp
class FBlueprintService : public IBlueprintService
{
private:
    TSharedPtr<FBlueprintCache> Cache;
    TSharedPtr<IComponentService> ComponentService;
    
public:
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override;
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params) override;
    // ... other methods
};
```

### Layer 3: Factory Layer

**Purpose**: Centralize object creation and type management

**Components**:
- `FComponentFactory` for Unreal Engine components
- `FWidgetFactory` for UMG widgets
- Type registration and lookup systems

**Responsibilities**:
- Register and manage object types
- Provide type-safe object creation
- Handle creation parameter validation
- Support runtime type discovery

**Example**:
```cpp
class FComponentFactory
{
private:
    TMap<FString, UClass*> ComponentTypeMap;
    FCriticalSection ComponentMapLock;
    
public:
    static FComponentFactory& Get();
    void RegisterComponentType(const FString& TypeName, UClass* ComponentClass);
    UClass* GetComponentClass(const FString& TypeName) const;
    TArray<FString> GetAvailableTypes() const;
};
```

### Layer 4: Utility Layer

**Purpose**: Provide common functionality and cross-cutting concerns

**Components**:
- Parameter validation framework
- Error handling and response formatting
- Logging and performance monitoring
- JSON utilities and common helpers

**Responsibilities**:
- Validate input parameters
- Handle errors consistently
- Provide logging and monitoring
- Offer reusable utility functions

## Design Patterns Used

### 1. Command Pattern

**Implementation**: `IUnrealMCPCommand` interface with concrete command classes

**Benefits**:
- Encapsulates requests as objects
- Allows parameterization of clients with different requests
- Enables queuing, logging, and undo operations
- Supports macro commands and command composition

**Usage**:
```cpp
// Register commands
Registry.RegisterCommand(TEXT("create_blueprint"), MakeShared<FCreateBlueprintCommand>(BlueprintService));
Registry.RegisterCommand(TEXT("add_component"), MakeShared<FAddComponentCommand>(ComponentService));

// Execute commands
FString Result = Registry.ExecuteCommand(TEXT("create_blueprint"), Parameters);
```

### 2. Factory Pattern

**Implementation**: `FComponentFactory` and `FWidgetFactory` singletons

**Benefits**:
- Centralizes object creation logic
- Provides type-safe object instantiation
- Enables runtime type registration
- Simplifies adding new types

**Usage**:
```cpp
// Register a new component type
FComponentFactory::Get().RegisterComponentType(TEXT("MyComponent"), UMyComponent::StaticClass());

// Create components
UClass* ComponentClass = FComponentFactory::Get().GetComponentClass(TEXT("StaticMeshComponent"));
```

### 3. Service Layer Pattern

**Implementation**: Service interfaces with concrete implementations

**Benefits**:
- Separates business logic from presentation
- Provides transaction boundaries
- Enables service composition
- Facilitates testing with mock services

**Usage**:
```cpp
// Use services in commands
TSharedPtr<IBlueprintService> BlueprintService = GetBlueprintService();
UBlueprint* Blueprint = BlueprintService->CreateBlueprint(Params);
```

### 4. Registry Pattern

**Implementation**: `FUnrealMCPCommandRegistry` for command management

**Benefits**:
- Centralizes command registration and lookup
- Enables dynamic command discovery
- Supports plugin-based architecture
- Provides consistent command execution

**Usage**:
```cpp
// Register commands during initialization
FUnrealMCPCommandRegistry::Get().RegisterCommand(CommandName, CommandInstance);

// Execute registered commands
FString Result = FUnrealMCPCommandRegistry::Get().ExecuteCommand(CommandName, Parameters);
```

### 5. Singleton Pattern

**Implementation**: Factory classes and registry use singleton pattern

**Benefits**:
- Ensures single instance of critical components
- Provides global access point
- Manages shared resources efficiently
- Maintains consistent state

**Thread Safety**: All singletons use proper locking mechanisms for thread safety.

### 6. Strategy Pattern

**Implementation**: Validation rules and error handling strategies

**Benefits**:
- Enables runtime algorithm selection
- Promotes code reuse
- Simplifies testing different strategies
- Allows easy extension of validation logic

**Usage**:
```cpp
// Define validation strategies
FValidationRule StringRule = FValidationRuleBuilder::RequiredString(TEXT("name"));
FValidationRule NumberRule = FValidationRuleBuilder::OptionalNumber(TEXT("value"));

// Apply strategies
Validator.AddRule(StringRule);
Validator.AddRule(NumberRule);
```

### 7. Template Method Pattern

**Implementation**: Base validation and error handling patterns

**Benefits**:
- Defines algorithm skeleton in base class
- Allows subclasses to override specific steps
- Promotes code reuse
- Ensures consistent behavior

### 8. Observer Pattern

**Implementation**: Logging and monitoring systems

**Benefits**:
- Enables loose coupling between components
- Supports multiple observers
- Allows dynamic subscription/unsubscription
- Facilitates event-driven architecture

## Component Interactions

### Request Flow

1. **HTTP Request** arrives at the MCP server
2. **Command Registry** looks up the appropriate command
3. **Command Class** validates parameters using validation framework
4. **Service Layer** executes business logic
5. **Factory Layer** creates required objects
6. **Utility Layer** provides common functionality
7. **Response** is formatted and returned

### Dependency Flow

```
Commands → Service Interfaces → Service Implementations → Factories → Utilities
    ↓              ↓                      ↓                  ↓           ↓
Validation    Business Logic      Object Creation      Type Safety   Error Handling
```

### Error Handling Flow

1. **Validation Errors** caught at command level
2. **Business Logic Errors** handled by services
3. **Creation Errors** managed by factories
4. **System Errors** caught by utilities
5. **All Errors** formatted consistently using error handler

## Performance Considerations

### Caching Strategy

**Blueprint Cache**:
- Thread-safe LRU cache for Blueprint objects
- Automatic invalidation on Blueprint changes
- Cache warming for frequently used Blueprints

**Component Type Cache**:
- Lazy loading of component type information
- Automatic refresh on module loading
- Fast lookup for type validation

**Object Pooling**:
- Reusable object pools for frequently created objects
- Configurable pool sizes
- Automatic cleanup and memory management

### Memory Management

**Smart Pointers**:
- `TSharedPtr` for shared ownership
- `TWeakObjectPtr` for Unreal Engine objects
- `TUniquePtr` for exclusive ownership

**Resource Management**:
- RAII principles for resource cleanup
- Proper exception safety
- Minimal memory allocations in hot paths

## Testing Strategy

### Unit Testing

**Service Layer Testing**:
- Mock implementations of service interfaces
- Isolated testing of business logic
- Comprehensive parameter validation testing

**Command Testing**:
- Mock service dependencies
- Parameter validation testing
- Error handling verification

**Factory Testing**:
- Type registration and lookup testing
- Creation parameter validation
- Error condition handling

### Integration Testing

**End-to-End Testing**:
- Complete request/response cycle testing
- Multi-service operation testing
- Performance and load testing

**Component Integration**:
- Service interaction testing
- Factory integration with services
- Error propagation testing

## Extension Points

### Adding New Commands

1. Implement `IUnrealMCPCommand` interface
2. Define parameter validation rules
3. Use appropriate service layer methods
4. Register with command registry
5. Add unit tests

### Adding New Services

1. Define service interface
2. Implement concrete service class
3. Add to dependency injection container
4. Create mock implementation for testing
5. Update command classes to use new service

### Adding New Object Types

1. Register type with appropriate factory
2. Implement creation parameters structure
3. Add validation rules for new type
4. Update service layer to handle new type
5. Add integration tests

## Migration Path

### From Monolithic to Layered

1. **Extract Services**: Move business logic to service classes
2. **Create Commands**: Split large handlers into individual commands
3. **Add Validation**: Replace manual checks with validation framework
4. **Implement Factories**: Centralize object creation
5. **Update Tests**: Migrate to new testing patterns

### Backward Compatibility

**Legacy Command Adapter**:
- Maintains compatibility with existing API
- Maps old command names to new implementations
- Provides parameter transformation
- Includes deprecation warnings

## Monitoring and Observability

### Logging Strategy

**Structured Logging**:
- Consistent log format across all components
- Contextual information for debugging
- Performance metrics collection
- Error tracking and aggregation

**Log Categories**:
- `LogUnrealMCP`: General plugin logging
- `LogMCPCommands`: Command execution logging
- `LogMCPServices`: Service operation logging
- `LogMCPValidation`: Parameter validation logging
- `LogMCPPerformance`: Performance metrics logging

### Performance Monitoring

**Metrics Collection**:
- Command execution times
- Service operation durations
- Cache hit/miss ratios
- Memory usage patterns

**Performance Optimization**:
- Identify bottlenecks through metrics
- Optimize hot paths
- Implement caching where beneficial
- Monitor resource usage

## Security Considerations

### Input Validation

**Parameter Validation**:
- Comprehensive validation rules
- Type safety enforcement
- Range and format checking
- Injection attack prevention

### Access Control

**Command Authorization**:
- Role-based access control
- Operation-level permissions
- Audit logging for security events

### Error Information

**Error Response Security**:
- Sanitized error messages
- No sensitive information exposure
- Consistent error format
- Proper error categorization

This architecture provides a solid foundation for maintainable, extensible, and testable code while following industry best practices and design patterns.