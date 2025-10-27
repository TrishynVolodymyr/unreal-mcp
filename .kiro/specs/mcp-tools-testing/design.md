# Design Document

## Overview

This design outlines a comprehensive testing framework for verifying the functionality of all 64 Unreal MCP tools after the recent C++ architecture refactoring. The testing system will systematically validate each tool category, identify broken functionality, and provide detailed reporting to guide restoration efforts.

The design follows a structured approach that tests tools in logical dependency order, captures detailed results, and provides actionable feedback for fixing any issues discovered during the refactoring process. All tool implementations must adhere to the new layered architecture defined in the CPP refactoring spec, which includes Command Pattern, Service Layer, Factory Pattern, Validation Framework, Error Handling System, and Caching Strategy.

## Architecture

### Testing Framework Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Test Orchestrator                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Test Scheduler  │  │ Result Collector│  │ Report      │ │
│  │                 │  │                 │  │ Generator   │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Tool Category Testers                    │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Blueprint       │  │ Editor          │  │ UMG         │ │
│  │ Tester          │  │ Tester          │  │ Tester      │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Node            │  │ DataTable       │  │ Project     │ │
│  │ Tester          │  │ Tester          │  │ Tester      │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
│  ┌─────────────────┐                                       │
│  │ Blueprint Action│                                       │
│  │ Tester          │                                       │
│  └─────────────────┘                                       │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Individual Tool Tests                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Parameter       │  │ Execution       │  │ Result      │ │
│  │ Validation      │  │ Testing         │  │ Verification│ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                Architecture Compliance                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Command Pattern │  │ Service Layer   │  │ Factory     │ │
│  │ Compliance      │  │ Compliance      │  │ Pattern     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │ Validation      │  │ Error Handling  │  │ Caching     │ │
│  │ Framework       │  │ System          │  │ Strategy    │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Test Execution Flow

1. **Preparation Phase**: Set up test environment and clean Unreal project state
2. **Dependency Testing**: Test foundational tools that other tools depend on
3. **Category Testing**: Execute tests for each tool category in logical order
4. **Integration Testing**: Verify tools work together correctly
5. **Reporting Phase**: Generate comprehensive test results and recommendations

## Components and Interfaces

### Test Categories and Dependencies

**Testing Order Based on Dependencies:**

1. **Project Tools** (Foundation) - Must work first as other tools depend on project structure
2. **Editor Tools** (Core) - Basic actor and scene management needed for other tests
3. **Blueprint Tools** (Core) - Blueprint creation needed for advanced testing
4. **DataTable Tools** (Independent) - Can be tested independently
5. **Node Tools** (Depends on Blueprints) - Requires working Blueprint system
6. **Blueprint Action Tools** (Depends on Blueprints) - Requires working Blueprint system
7. **UMG Tools** (Depends on Blueprints) - Widget Blueprints depend on Blueprint system

### Individual Tool Test Structure

Each tool test will follow this pattern:

```
Tool Test {
    - Tool Name and Category
    - Prerequisites (dependencies)
    - Test Parameters (input validation)
    - Expected Behavior
    - Actual Execution
    - Result Verification
    - Error Handling
    - Performance Metrics
    - Success/Failure Status
}
```

### Test Data Management

**Test Assets Creation:**
- Temporary test Blueprints, structs, and assets
- Cleanup procedures to maintain clean test environment
- Asset naming conventions to avoid conflicts
- Backup and restore procedures for critical project files

**Test Parameters:**
- Standardized test inputs for consistent results
- Edge case parameters to test robustness
- Invalid parameters to test error handling
- Performance stress test parameters

## Data Models

### Test Result Structure

```json
{
  "testSuite": "MCP Tools Comprehensive Test",
  "timestamp": "2025-01-19T10:30:00Z",
  "totalTools": 64,
  "categories": [
    {
      "name": "Blueprint Tools",
      "toolCount": 13,
      "results": [
        {
          "toolName": "create_blueprint",
          "status": "PASS|FAIL|ERROR",
          "executionTime": 1.23,
          "parameters": {...},
          "expectedResult": "...",
          "actualResult": "...",
          "errorMessage": "...",
          "recommendations": ["..."]
        }
      ]
    }
  ],
  "summary": {
    "totalPassed": 45,
    "totalFailed": 15,
    "totalErrors": 4,
    "successRate": 70.3,
    "criticalIssues": ["..."],
    "recommendations": ["..."]
  }
}
```

### Tool Categorization

**Blueprint Tools (13 tools):**
1. create_blueprint
2. add_blueprint_variable  
3. add_component_to_blueprint
4. set_static_mesh_properties
5. set_component_property
6. set_physics_properties
7. compile_blueprint
8. set_blueprint_property
9. set_pawn_properties
10. call_blueprint_function
11. add_interface_to_blueprint
12. create_blueprint_interface
13. list_blueprint_components
14. create_custom_blueprint_function

**Blueprint Action Tools (6 tools):**
1. get_actions_for_pin
2. get_actions_for_class
3. get_actions_for_class_hierarchy
4. search_blueprint_actions
5. get_node_pin_info
6. create_node_by_action_name

**Editor Tools (11 tools):**
1. get_actors_in_level
2. find_actors_by_name
3. spawn_actor
4. delete_actor
5. set_actor_transform
6. get_actor_properties
7. set_actor_property
8. set_light_property
9. spawn_blueprint_actor
10. create_input_mapping
11. Enhanced Input tools (grouped)

**Node Tools (6 tools):**
1. add_blueprint_input_action_node
2. connect_blueprint_nodes
3. find_blueprint_nodes
4. get_variable_info
5. Function call nodes (grouped)
6. Component reference nodes (grouped)

**UMG Tools (14 tools):**
1. create_umg_widget_blueprint
2. bind_widget_component_event
3. set_text_block_widget_component_binding
4. add_child_widget_component_to_parent
5. create_parent_and_child_widget_components
6. check_widget_component_exists
7. set_widget_component_placement
8. get_widget_container_component_dimensions
9. add_widget_component_to_widget
10. set_widget_component_property
11. get_widget_component_layout
12. Widget hierarchy management (grouped)
13. Widget styling (grouped)
14. Widget event handling (grouped)

**DataTable Tools (6 tools):**
1. create_datatable
2. get_datatable_rows
3. delete_datatable_row
4. get_datatable_row_names
5. add_rows_to_datatable
6. update_rows_in_datatable
7. delete_datatable_rows

**Project Tools (8 tools):**
1. create_input_mapping
2. create_enhanced_input_action
3. create_input_mapping_context
4. add_mapping_to_context
5. list_input_actions
6. list_input_mapping_contexts
7. create_folder
8. create_struct

## Error Handling

### Error Classification System

**Error Categories:**
1. **Connection Errors**: MCP server communication failures
2. **Parameter Errors**: Invalid or missing parameters
3. **Execution Errors**: Tool execution failures in Unreal Engine
4. **Result Errors**: Unexpected or invalid results
5. **Integration Errors**: Tools not working together properly

**Error Handling Strategy:**
- Capture all error types with detailed context
- Provide specific error messages for each failure mode
- Include stack traces and debugging information where available
- Suggest specific remediation steps for common error patterns

### Recovery Procedures

**Test Environment Recovery:**
- Automatic cleanup of test assets after each test
- Project state restoration if tests cause corruption
- MCP server restart procedures if communication fails
- Fallback testing modes for partially working systems

## Testing Strategy

### Phase 1: Foundation Testing (Project Tools)

**Critical Foundation Tools:**
- create_folder: Test project organization capabilities
- create_struct: Test data structure creation
- Enhanced Input system: Test input system setup
- Basic project management functions

**Success Criteria:**
- All project organization tools work correctly
- Input system can be configured properly
- Struct creation and management functions properly
- Project structure can be established for further testing

### Phase 2: Core System Testing (Editor + Blueprint Tools)

**Editor Tools Testing:**
- Actor creation, deletion, and manipulation
- Property getting and setting
- Level management functions
- Blueprint actor spawning

**Blueprint Tools Testing:**
- Blueprint creation with various parent classes
- Component addition and configuration
- Variable and property management
- Compilation and interface handling

**Success Criteria:**
- Basic actor and Blueprint management works
- Property systems function correctly
- Compilation process works without errors
- Foundation exists for advanced testing

### Phase 3: Advanced Logic Testing (Node + Blueprint Action Tools)

**Node Tools Testing:**
- Event node creation and management
- Node connection and graph building
- Variable information retrieval
- Function call node creation

**Blueprint Action Tools Testing:**
- Action discovery by pin type and class
- Node creation from discovered actions
- Pin information retrieval
- Search functionality across action database

**Success Criteria:**
- Blueprint visual scripting can be created programmatically
- Action discovery system works correctly
- Node graphs can be built and connected properly
- Advanced Blueprint logic can be automated

### Phase 4: Data and UI Testing (DataTable + UMG Tools)

**DataTable Tools Testing:**
- Table creation with custom structs
- Row addition, modification, and deletion
- Data querying and structure inspection
- Bulk operations and data management

**UMG Tools Testing:**
- Widget Blueprint creation
- Component addition and hierarchy building
- Event binding and property binding
- Layout management and styling

**Success Criteria:**
- Data management systems work correctly
- UI creation and management functions properly
- Event systems and bindings work as expected
- Complete game systems can be built

### Phase 5: Integration and Stress Testing

**Cross-Category Integration:**
- Tools from different categories working together
- Complex workflows using multiple tool types
- Data flow between different systems
- Performance under load conditions

**Stress Testing:**
- Large numbers of operations
- Complex nested structures
- Memory usage and cleanup
- Error recovery under stress

## Performance Optimizations

### Test Execution Optimization

**Parallel Testing:**
- Independent tools can be tested simultaneously
- Category-level parallelization where dependencies allow
- Resource management to prevent conflicts

**Caching Strategy:**
- Reuse test assets where possible
- Cache MCP server connections
- Optimize test data generation

**Resource Management:**
- Monitor memory usage during testing
- Clean up test assets promptly
- Manage Unreal Engine editor resources efficiently

## Migration and Restoration Strategy

### Issue Identification Process

1. **Categorize Failures**: Group failures by type and severity
2. **Identify Root Causes**: Determine if issues are in C++ refactoring, Python MCP servers, or integration
3. **Prioritize Fixes**: Focus on critical tools that block other functionality
4. **Create Fix Plans**: Develop specific remediation strategies for each issue type
5. **Architecture Compliance**: Ensure all fixes adhere to the new layered architecture

### Restoration Approach

**High Priority Fixes:**
- Foundation tools (Project Tools) that other tools depend on
- Core Blueprint and Editor tools needed for basic functionality
- Critical integration points between C++ and Python layers
- Command Pattern implementation for all tools
- Service Layer extraction for core functionality

**Medium Priority Fixes:**
- Advanced functionality tools
- Performance optimizations
- Error handling improvements
- Factory Pattern implementation
- Validation Framework integration
- Error Handling System standardization

**Low Priority Fixes:**
- Edge case handling
- Documentation updates
- Minor feature enhancements
- Caching Strategy implementation
- Performance monitoring integration
- Memory optimization

### Success Metrics

**Quantitative Metrics:**
- Percentage of tools working correctly (target: 100%)
- Average execution time per tool (performance baseline)
- Error rate under normal conditions (target: <0%)
- Memory usage and resource efficiency

**Qualitative Metrics:**
- User experience quality
- Error message clarity and helpfulness
- Integration smoothness between tool categories
- Overall system reliability and stability

This comprehensive testing design ensures that all 64 MCP tools are thoroughly validated, issues are clearly identified, and a clear path to restoration is established for the post-refactoring system.
## A
rchitecture Compliance Testing

### Command Pattern Compliance

Each tool implementation must follow the Command Pattern as defined in the CPP refactoring spec:

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

**Testing Approach:**
1. Verify each tool has a corresponding command class implementing IUnrealMCPCommand
2. Confirm proper command registration in FUnrealMCPCommandRegistry
3. Test ValidateParams implementation for parameter validation
4. Verify Execute method properly delegates to service layer

### Service Layer Compliance

Business logic should be extracted into service classes following the single responsibility principle:

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

**Testing Approach:**
1. Verify business logic is extracted from command handlers into appropriate services
2. Confirm services follow single responsibility principle
3. Test service interfaces for proper abstraction
4. Verify command handlers delegate to services rather than implementing logic directly

### Factory Pattern Compliance

Component and widget creation should use factory patterns:

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

**Testing Approach:**
1. Verify component creation uses ComponentFactory
2. Confirm widget creation uses WidgetFactory
3. Test proper registration of component and widget types
4. Verify factory methods are used instead of direct creation

### Validation Framework Compliance

Parameter validation should use the validation framework:

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

**Testing Approach:**
1. Verify each command uses FParameterValidator for validation
2. Confirm validation rules are properly defined
3. Test validation error reporting
4. Verify consistent validation approach across all commands

### Error Handling System Compliance

Error handling should use the structured error system:

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

**Testing Approach:**
1. Verify commands use FMCPError for error reporting
2. Confirm proper error categorization
3. Test error message clarity and detail
4. Verify consistent error handling across all commands

### Caching Strategy Compliance

Appropriate caching should be implemented:

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

**Testing Approach:**
1. Verify appropriate caching for Blueprint and component references
2. Confirm proper cache invalidation
3. Test thread safety for cached resources
4. Verify performance improvements from caching