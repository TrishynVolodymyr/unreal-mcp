# UnrealMCP Plugin Documentation

## Overview

Welcome to the UnrealMCP plugin documentation. This plugin provides a comprehensive Model Context Protocol (MCP) integration for Unreal Engine 5.7, enabling AI assistants to control Unreal Engine through natural language commands.

## Documentation Structure

This documentation is organized into several key documents that cover different aspects of the plugin:

### Core Documentation

1. **[API Reference](API_Reference.md)** - Comprehensive API documentation for all interfaces, classes, and functions
2. **[Architecture Guide](Architecture_Guide.md)** - Detailed explanation of the plugin's layered architecture and design patterns
3. **[Coding Style Guide](Coding_Style_Guide.md)** - Coding standards and best practices for contributing to the plugin
4. **[Legacy Command Migration](LegacyCommandMigration.md)** - Guide for migrating from old monolithic commands to the new architecture

### Quick Start

For developers new to the UnrealMCP plugin:

1. **Start with the [Architecture Guide](Architecture_Guide.md)** to understand the overall system design
2. **Review the [API Reference](API_Reference.md)** for specific interface documentation
3. **Follow the [Coding Style Guide](Coding_Style_Guide.md)** when contributing code
4. **Use the [Legacy Command Migration](LegacyCommandMigration.md)** guide if working with existing code

## Plugin Architecture

The UnrealMCP plugin follows a clean, layered architecture:

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

## Key Features

### Command-Based Architecture
- Individual command classes for each operation
- Standardized parameter validation
- Consistent error handling and response formatting
- Easy extensibility for new commands

### Service Layer Separation
- Business logic separated from request handling
- Mockable interfaces for testing
- Dependency injection support
- Clear separation of concerns

### Factory Pattern Implementation
- Type-safe object creation
- Runtime type registration
- Extensible type system
- Centralized object management

### Comprehensive Error Handling
- Structured error types with context
- Consistent error response format
- Enhanced error tracking with operation contexts
- Detailed logging and debugging support

### Performance Optimizations
- Object pooling for frequently created objects
- Blueprint caching system
- Thread-safe operations
- Memory-efficient resource management

## Core Interfaces

### IUnrealMCPCommand
Base interface for all MCP commands:
```cpp
class IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) = 0;
    virtual FString GetCommandName() const = 0;
    virtual bool ValidateParams(const FString& Parameters) const = 0;
};
```

### Service Interfaces
- **IBlueprintService** - Blueprint creation and management
- **IComponentService** - Component operations
- **IPropertyService** - Property manipulation
- **IDataTableService** - DataTable operations
- **IEditorService** - Editor and viewport control
- **IProjectService** - Project-level operations

### Factory Classes
- **FComponentFactory** - Component type management
- **FWidgetFactory** - UMG widget creation
- **FObjectPoolManager** - Object pool management

## Getting Started

### For Plugin Users

1. **Installation**: The plugin is automatically loaded with the project
2. **Usage**: Commands are executed through the MCP protocol via TCP connection
3. **Configuration**: Server runs on port 55557 by default

### For Plugin Developers

1. **Adding New Commands**:
   ```cpp
   class FMyCommand : public IUnrealMCPCommand
   {
   public:
       virtual FString Execute(const FString& Parameters) override;
       virtual FString GetCommandName() const override;
       virtual bool ValidateParams(const FString& Parameters) const override;
   };
   ```

2. **Creating Services**:
   ```cpp
   class IMyService
   {
   public:
       virtual bool DoSomething(const FMyParams& Params) = 0;
   };
   ```

3. **Using Factories**:
   ```cpp
   UClass* ComponentClass = FComponentFactory::Get().GetComponentClass(TEXT("StaticMeshComponent"));
   ```

## Testing

The plugin includes comprehensive testing support:

### Unit Testing
- Mock implementations for all service interfaces
- Isolated testing of business logic
- Parameter validation testing
- Error handling verification

### Integration Testing
- End-to-end command execution testing
- Service interaction testing
- Performance benchmarking
- Load testing capabilities

## Performance Considerations

### Caching
- Blueprint objects are cached for fast lookup
- Component type information is cached
- Validation rules are pre-compiled

### Object Pooling
- JSON objects are pooled to reduce allocations
- MCP responses are reused
- Parameter validators are pooled
- Configurable pool sizes

### Memory Management
- Smart pointers for automatic memory management
- RAII patterns for resource cleanup
- Minimal allocations in hot paths

## Thread Safety

The plugin is designed with thread safety in mind:
- Command registry uses proper locking
- Factory classes are thread-safe
- Object pools use atomic operations
- Service implementations handle concurrency

## Error Handling

Comprehensive error handling system:
- Structured error types (FMCPError)
- Error severity levels
- Context-aware error reporting
- Consistent error response format

## Logging and Monitoring

Built-in logging and monitoring:
- Structured logging with categories
- Performance metrics collection
- Operation context tracking
- Debug information capture

## Extension Points

The plugin is designed for extensibility:

### Adding New Command Types
1. Implement IUnrealMCPCommand interface
2. Register with command registry
3. Add parameter validation rules
4. Implement unit tests

### Adding New Services
1. Define service interface
2. Implement concrete service class
3. Create mock for testing
4. Register with dependency injection

### Adding New Object Types
1. Register with appropriate factory
2. Define creation parameters
3. Add validation rules
4. Update service layer

## Best Practices

### Command Implementation
- Keep commands focused on request/response handling
- Delegate business logic to services
- Use structured error responses
- Implement comprehensive validation

### Service Implementation
- Design for testability with interfaces
- Use dependency injection
- Implement proper error handling
- Consider performance implications

### Testing
- Write unit tests for all new code
- Use mock implementations for dependencies
- Test error conditions thoroughly
- Include integration tests

## Troubleshooting

### Common Issues

1. **Command Not Found**
   - Verify command is registered in command registry
   - Check command name spelling
   - Ensure module is properly loaded

2. **Parameter Validation Failures**
   - Review parameter validation rules
   - Check JSON parameter format
   - Verify required parameters are provided

3. **Service Initialization Errors**
   - Check service dependencies
   - Verify factory initialization
   - Review module startup sequence

### Debug Information

Enable debug logging for detailed information:
```cpp
UE_LOG(LogUnrealMCP, VeryVerbose, TEXT("Debug message"));
```

## Contributing

When contributing to the plugin:

1. Follow the [Coding Style Guide](Coding_Style_Guide.md)
2. Add comprehensive documentation
3. Include unit tests
4. Update integration tests
5. Follow the established architecture patterns

## Support

For support and questions:
- Review the documentation thoroughly
- Check existing issues and solutions
- Follow the troubleshooting guide
- Ensure you're following best practices

## Version History

### Current Version
- Refactored architecture with layered design
- Comprehensive error handling system
- Performance optimizations with caching and pooling
- Extensive documentation and testing support

### Previous Versions
- Monolithic command handlers (deprecated)
- Basic error handling (replaced)
- Limited extensibility (improved)

This documentation provides a comprehensive guide to understanding, using, and extending the UnrealMCP plugin. For specific implementation details, refer to the individual documentation files listed above.