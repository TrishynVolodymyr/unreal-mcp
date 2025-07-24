# Design Document

## Overview

The issue with non-editable custom functions stems from incomplete function metadata setup during creation. Unreal Engine's Blueprint system requires specific metadata and property configurations to make functions fully editable in the Blueprint graph editor. The current implementation creates the function graph and nodes but doesn't properly configure the function's metadata, access specifiers, and other properties that the Blueprint editor relies on.

## Architecture

The fix involves modifying the C++ implementation of both `HandleCreateCustomBlueprintFunction` and `HandleSetTextBlockBinding` methods to:

1. **Properly configure function metadata** using UE's `FBlueprintMetadata` system
2. **Set function flags and access specifiers** using the correct UFunction flags
3. **Use FBlueprintEditorUtils methods** that mirror the Blueprint editor's "Add Function" workflow
4. **Ensure function signature persistence** by properly configuring the UFunction object

## Components and Interfaces

### Modified Components

1. **UnrealMCPBlueprintCommands.cpp**
   - `HandleCreateCustomBlueprintFunction()` method
   - Enhanced parameter handling for metadata

2. **UnrealMCPUMGCommands.cpp** 
   - `HandleSetTextBlockBinding()` method (for binding functions)
   - Consistent metadata application

3. **Python MCP Tools**
   - Enhanced parameter validation
   - New optional parameters for function metadata

### Key UE API Classes Used

1. **FBlueprintEditorUtils**
   - `AddFunctionGraph()` - Proper function graph creation
   - `MarkBlueprintAsStructurallyModified()` - Ensure editor refresh

2. **UK2Node_FunctionEntry**
   - Function signature configuration
   - Metadata attachment point

3. **FBlueprintMetadata**
   - Function description storage
   - Category organization
   - Custom metadata key-value pairs

4. **UFunction Flags**
   - `FUNC_BlueprintCallable` - Makes function callable from Blueprints
   - `FUNC_Public/Protected/Private` - Access control
   - `FUNC_BlueprintPure` - Pure function designation

## Data Models

### Enhanced Function Creation Parameters

```cpp
struct FCustomFunctionParams {
    FString FunctionName;           // Required
    FString Description;            // Optional - function description
    FString Category;               // Optional - function category  
    EFunctionAccess AccessLevel;    // Optional - Public/Protected/Private
    bool bIsPure;                   // Optional - pure function flag
    bool bIsConst;                  // Optional - const function flag
    bool bCallInEditor;             // Optional - call in editor flag
    TArray<FInputParam> Inputs;     // Optional - input parameters
    TArray<FOutputParam> Outputs;   // Optional - output parameters
};
```

### Function Metadata Structure

```cpp
// Metadata keys used by UE Blueprint system
static const FString MD_FunctionDescription = TEXT("ToolTip");
static const FString MD_FunctionCategory = TEXT("Category"); 
static const FString MD_CallInEditor = TEXT("CallInEditor");
static const FString MD_BlueprintProtected = TEXT("BlueprintProtected");
static const FString MD_BlueprintPrivate = TEXT("BlueprintPrivate");
```

## Error Handling

### Validation Strategy

1. **Parameter Validation**
   - Validate function names for Blueprint compatibility
   - Check for duplicate function names
   - Validate metadata string formats

2. **UE API Error Handling**
   - Check for null Blueprint references
   - Validate function graph creation success
   - Ensure metadata application success

3. **Graceful Degradation**
   - If metadata fails to apply, function should still be created
   - Log warnings for non-critical metadata failures
   - Provide detailed error messages for critical failures

## Testing Strategy

### Unit Testing Approach

1. **Function Creation Tests**
   - Test basic function creation with default parameters
   - Test function creation with full metadata
   - Test function creation with invalid parameters

2. **Metadata Persistence Tests**
   - Verify metadata survives Blueprint compilation
   - Verify metadata survives Blueprint save/load
   - Verify metadata is visible in Blueprint editor

3. **Backward Compatibility Tests**
   - Ensure existing MCP calls continue working
   - Test with various Blueprint types (Actor, Widget, etc.)
   - Test with different UE project configurations

### Integration Testing

1. **Blueprint Editor Integration**
   - Verify functions appear in Functions list
   - Verify right-click context menu works
   - Verify function properties dialog opens and functions

2. **MCP Tool Integration**
   - Test both `create_custom_blueprint_function` and text binding tools
   - Test parameter passing from Python to C++
   - Test error reporting back to MCP clients

## Implementation Details

### Critical UE API Usage Patterns

Based on UE documentation and source code analysis, the key to making functions editable is:

1. **Use FBlueprintEditorUtils::AddFunctionGraph()** instead of manually adding to FunctionGraphs array
2. **Set metadata on the UK2Node_FunctionEntry** using `SetMetaData()` method
3. **Configure UFunction flags** during or immediately after function creation
4. **Call FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified()** to trigger editor refresh

### Metadata Application Pattern

```cpp
// Apply metadata to function entry node
UK2Node_FunctionEntry* EntryNode = GetFunctionEntryNode(FuncGraph);
if (EntryNode && !Description.IsEmpty()) {
    EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_Tooltip, Description);
}
if (EntryNode && !Category.IsEmpty()) {
    EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_FunctionCategory, Category);
}

// Set access level flags
if (AccessLevel == EFunctionAccess::Private) {
    EntryNode->SetExtraFlags(EntryNode->GetExtraFlags() | FUNC_Private);
} else if (AccessLevel == EFunctionAccess::Protected) {
    EntryNode->SetExtraFlags(EntryNode->GetExtraFlags() | FUNC_Protected);
}
```

This design ensures that custom functions created via MCP tools will have the same editing capabilities as functions created manually through the Blueprint editor interface.