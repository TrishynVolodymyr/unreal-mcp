# Blueprint Node Creation Service Refactoring Guide

## Overview
This guide explains how to refactor `BlueprintNodeCreationService.cpp` into smaller, focused files.

---

## File Structure

```
Services/
├── BlueprintNodeCreationService.h          (MAIN SERVICE - keep as orchestrator)
├── BlueprintNodeCreationService.cpp        (MAIN SERVICE - simplified)
└── NodeCreation/
    ├── NodeCreationHelpers.h
    ├── NodeCreationHelpers.cpp
    ├── ArithmeticNodeCreator.h
    ├── ArithmeticNodeCreator.cpp
    ├── NativePropertyNodeCreator.h
    ├── NativePropertyNodeCreator.cpp
    ├── BlueprintActionDatabaseNodeCreator.h
    ├── BlueprintActionDatabaseNodeCreator.cpp
    ├── NodeResultBuilder.h
    └── NodeResultBuilder.cpp
```

---

## Migration Steps

### 1. NodeCreationHelpers.cpp
**Source**: `BlueprintNodeCreationService.cpp`

**Functions to copy**:
- `ConvertPropertyNameToDisplayLocal` (line ~46) → rename to `ConvertPropertyNameToDisplay`
- `ConvertCamelCaseToTitleCase` (line ~70)

**Notes**:
- These are simple utility functions
- Remove `Local` suffix from function name
- Make them part of `NodeCreationHelpers` namespace

---

### 2. ArithmeticNodeCreator.cpp
**Source**: `BlueprintNodeCreationService.cpp`

**Functions to copy**:
- `TryCreateArithmeticOrComparisonNode` (static function, line ~1558)

**Size**: ~200 lines

**Notes**:
- This is a self-contained function
- Handles operators: +, -, *, /, <, >, ==, !=, etc.
- Make it a static member of `FArithmeticNodeCreator`

**Required includes**:
```cpp
#include "K2Node_PromotableOperator.h"
#include "K2Node_CallFunction.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetMathLibrary.h"
```

---

### 3. NativePropertyNodeCreator.cpp
**Source**: `BlueprintNodeCreationService.cpp`

**Functions to copy**:
- `TryCreateNativePropertyNode` (static function, line ~1755)

**Size**: ~150 lines

**Notes**:
- Handles native property getters/setters
- Searches through UE4/UE5 native classes for properties
- Make it a static member of `FNativePropertyNodeCreator`

**Required includes**:
```cpp
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
```

---

### 4. BlueprintActionDatabaseNodeCreator.cpp
**Source**: `BlueprintNodeCreationService.cpp`

**Functions to copy**:
- `FBlueprintNodeCreationService::TryCreateNodeUsingBlueprintActionDatabase` (line ~1127)

**Size**: ~350 lines (VERY LARGE!)

**Notes**:
- **THIS IS THE MOST IMPORTANT FILE FOR DUPLICATE CHECK FEATURE**
- This is where you should add the logic to detect duplicate function names
- Searches through Blueprint Action Database
- Handles Enhanced Input Actions
- Handles operation aliases
- Make it a static member of `FBlueprintActionDatabaseNodeCreator`

**Required includes**:
```cpp
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "NodeCreationHelpers.h"  // For ConvertCamelCaseToTitleCase
```

**CRITICAL - Add duplicate check here**:
After the function fails to find a match, before returning false, add logic to:
1. Search all spawners for functions with matching name
2. Collect all unique class names
3. If count > 1, set `NodeTitle` to error message and `NodeType` to "ERROR"
4. Return false (let caller handle error formatting)

---

### 5. NodeResultBuilder.cpp
**Source**: `BlueprintNodeCreationService.cpp`

**Functions to copy**:
- `FBlueprintNodeCreationService::BuildNodeResult` (line ~1485)

**Size**: ~70 lines

**Notes**:
- Builds JSON response for node creation
- Includes pin information, position, success/error status
- Make it a static member of `FNodeResultBuilder`

**Required includes**:
```cpp
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphPin.h"
```

---

### 6. BlueprintNodeCreationService.cpp (UPDATED)
**Keep these functions** (don't move):
- `FBlueprintNodeCreationService()` constructor
- `CreateNodeByActionName()` - MAIN ORCHESTRATOR METHOD
- `ParseJsonParameters()` 
- `FindTargetClass()` - helper

**Update these sections**:
1. **Add includes** at top:
```cpp
#include "NodeCreation/NodeCreationHelpers.h"
#include "NodeCreation/ArithmeticNodeCreator.h"
#include "NodeCreation/NativePropertyNodeCreator.h"
#include "NodeCreation/BlueprintActionDatabaseNodeCreator.h"
#include "NodeCreation/NodeResultBuilder.h"
```

2. **Replace function calls** in `CreateNodeByActionName()`:
   - `TryCreateArithmeticOrComparisonNode()` → `FArithmeticNodeCreator::TryCreateArithmeticOrComparisonNode()`
   - `TryCreateNodeUsingBlueprintActionDatabase()` → `FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase()`
   - `BuildNodeResult()` → `FNodeResultBuilder::BuildNodeResult()`
   - `ConvertCamelCaseToTitleCase()` → `NodeCreationHelpers::ConvertCamelCaseToTitleCase()`

3. **Remove**:
   - All static function declarations at top
   - All static function implementations
   - Old `BuildNodeResult()` implementation
   - Old `TryCreateNodeUsingBlueprintActionDatabase()` implementation

---

## Duplicate Function Check Implementation

**Where to add**: `BlueprintActionDatabaseNodeCreator.cpp`

**When to check**: At the END of the main search loop in `TryCreateNodeUsingBlueprintActionDatabase()`, when no node was created and ClassName is empty

**Pseudocode**:
```cpp
// At the end of TryCreateNodeUsingBlueprintActionDatabase, before final return false:

if (ClassName.IsEmpty())
{
    // Search for duplicates
    TMap<FString, TArray<FString>> FunctionToClasses;
    
    for (all spawners in ActionDatabase)
    {
        if (function name matches FunctionName)
        {
            Add OwnerClass->GetName() to FunctionToClasses[FunctionName]
        }
    }
    
    if (FunctionToClasses[FunctionName].Num() > 1)
    {
        NodeTitle = FString::Printf(TEXT("ERROR: Multiple '%s' functions"), *FunctionName);
        NodeType = TEXT("ERROR");
        return false;  // Caller will detect ERROR type and format detailed response
    }
}

return false;
```

**Then in CreateNodeByActionName()**, after calling `TryCreateNodeUsingBlueprintActionDatabase()`:
```cpp
if (NodeType == TEXT("ERROR"))
{
    // Search again to collect all options
    // Build JSON error with:
    // - error message
    // - available_options array (function_name, class_name, tooltip)
    // - suggestion with example
    return JSON error string;
}
```

---

## Testing After Refactoring

1. Try creating a node without class_name when duplicates exist → should get detailed error
2. Try creating a node with class_name → should work
3. Try creating operators (+, -, etc.) → should work
4. Try creating Enhanced Input Action nodes → should work
5. Compile and ensure no errors

---

## Benefits

1. **Smaller files** - easier to navigate and understand
2. **Clear separation** - each file has one responsibility
3. **Easier to add duplicate check** - clear location in BlueprintActionDatabaseNodeCreator
4. **Better testing** - can test each creator independently
5. **Less merge conflicts** - different features modify different files
