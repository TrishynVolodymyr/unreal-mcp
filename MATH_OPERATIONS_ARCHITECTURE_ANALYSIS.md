# Mathematical Operations Architecture Analysis in Unreal Engine 5.6

## Problem Statement
The MCP plugin's `search_blueprint_actions` function successfully finds regular Blueprint functions but fails to locate mathematical operations like Add, Subtract, Multiply, etc., despite the fact that `create_node_by_action_name` can successfully create these nodes.

## Root Cause Analysis

### Key Discovery
Mathematical operations in Unreal Engine 5.6 use a **completely different architecture** from regular Blueprint functions. They are not stored in the standard `FBlueprintActionDatabase` but in a separate system called **TypePromotion**.

## Architecture Details

### 1. Regular Blueprint Functions
- Stored in: `FBlueprintActionDatabase`
- Created via: `UBlueprintFunctionNodeSpawner`
- Registration: Added to action database through `GetNodeSpecificActions`
- Search: Found by standard BlueprintActionDatabase queries

### 2. Mathematical Operations (Add, Subtract, etc.)
- Stored in: `FTypePromotion::OperatorNodeSpawnerMap`
- Created via: `UK2Node_PromotableOperator`
- Registration: `FTypePromotion::RegisterOperatorSpawner(OpName, NodeSpawner)`
- Search: **NOT included in standard BlueprintActionDatabase queries**

## Technical Implementation

### Node Types Involved
1. **UK2Node_PromotableOperator** - Base class for dynamic mathematical operations
2. **UK2Node_CommutativeAssociativeBinaryOperator** - Specialized for associative operations
3. **UBlueprintFunctionNodeSpawner** - Standard spawner (also used for math ops)

### Key Files and Functions

#### TypePromotion System
- **File**: `ues/UnrealEngine-5.6/Engine/Source/Editor/BlueprintGraph/Private/BlueprintTypePromotion.cpp`
- **Key Functions**:
  - `FTypePromotion::RegisterOperatorSpawner(FName OpName, UBlueprintFunctionNodeSpawner* Spawner)`
  - `FTypePromotion::GetOperatorSpawner(FName OpName)`
  - `FTypePromotion::IsOperatorSpawnerRegistered(UFunction const* const Func)`

#### Registration Logic
- **File**: `ues/UnrealEngine-5.6/Engine/Source/Editor/BlueprintGraph/Private/BlueprintFunctionNodeSpawner.cpp`
- **Lines**: 295-302
```cpp
if(bIsPromotableFunction)
{
    MenuSignature.MenuName = FTypePromotion::GetUserFacingOperatorName(OpName);
    MenuSignature.Category = LOCTEXT("UtilityOperatorCategory", "Utilities|Operators");
    MenuSignature.Tooltip = FTypePromotion::GetUserFacingOperatorName(OpName);
    MenuSignature.Keywords = FTypePromotion::GetKeywordsForOperator(OpName);
    FTypePromotion::RegisterOperatorSpawner(OpName, NodeSpawner); // ← KEY LINE
}
```

#### Creation Logic
- **File**: `ues/UnrealEngine-5.6/Engine/Source/Editor/BlueprintEditorLibrary/Private/BlueprintEditorLibrary.cpp`
- **Function**: `CreateOpNode(const FName OpName, UEdGraph* Graph, const int32 AdditionalPins)`
- **Key Line**: `UBlueprintFunctionNodeSpawner* Spawner = FTypePromotion::GetOperatorSpawner(OpName)`

### Data Storage Structure

#### Standard Actions Database
```cpp
// Regular functions stored here
FBlueprintActionDatabase::ActionDatabase
├── UBlueprintFunctionNodeSpawner (regular functions)
├── UBlueprintComponentNodeSpawner
├── UBlueprintEventNodeSpawner
└── ...
```

#### TypePromotion System
```cpp
// Mathematical operations stored here (SEPARATE!)
FTypePromotion::Instance->OperatorNodeSpawnerMap
├── "Add" → UBlueprintFunctionNodeSpawner
├── "Subtract" → UBlueprintFunctionNodeSpawner
├── "Multiply" → UBlueprintFunctionNodeSpawner
├── "Divide" → UBlueprintFunctionNodeSpawner
└── ...
```

## Why This Architecture Exists

### Dynamic Type Promotion
Mathematical operations in UE5 support **dynamic type promotion**, meaning:
- An "Add" node can work with Int + Int = Int
- The same "Add" node can work with Float + Float = Float  
- It can promote Int + Float = Float automatically
- The actual function called is determined at compile time based on connected pin types

### Regular Functions vs Promotable Operations
- **Regular Functions**: Fixed signature, specific types (e.g., `Add_IntInt`, `Add_FloatFloat`)
- **Promotable Operations**: Dynamic signature, type-agnostic until connected (e.g., `Add`)

## Current MCP Plugin Limitation

### What Works
- `create_node_by_action_name("Add")` ✅ - Uses `FTypePromotion::GetOperatorSpawner`
- `get_actions_for_class("KismetMathLibrary")` ✅ - Finds specific functions like `Add_IntInt`

### What Doesn't Work  
- `search_blueprint_actions("Add")` ❌ - Only searches standard ActionDatabase

### SearchBlueprintActions Function Issue
**File**: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.cpp`

**Current Implementation**: Only searches `FBlueprintActionDatabase`
**Missing**: Search in `FTypePromotion::OperatorNodeSpawnerMap`

## Solution Requirements

### What Needs to Be Modified
1. **SearchBlueprintActions function** in UnrealMCPBlueprintActionCommands.cpp
2. Add search logic for TypePromotion operator spawners
3. Include results from `FTypePromotion::OperatorNodeSpawnerMap`

### Implementation Strategy
```cpp
// Pseudo-code for fix
void SearchBlueprintActions(/* params */) {
    // 1. Search regular actions (current implementation)
    SearchRegularBlueprintActions();
    
    // 2. Search TypePromotion operators (NEW)
    SearchOperatorSpawners(); 
    
    // 3. Merge and return results
    CombineAndReturnResults();
}
```

### Required Access Points
- `FTypePromotion::GetOperatorSpawner()` - to iterate through operators
- `FTypePromotion::OperatorNodeSpawnerMap` - direct access to the map
- `FTypePromotion::GetUserFacingOperatorName()` - for display names

## Impact Assessment

### Plugin Functionality Status
- ✅ **Node Creation**: Fully working - uses correct TypePromotion system
- ❌ **Node Search**: Partially working - missing mathematical operations
- ✅ **Class Actions**: Working - finds specific math functions in KismetMathLibrary
- ✅ **Pin Actions**: Working - finds compatible operations for pin types

### User Experience Impact
- Users can create math nodes via direct name: `create_node_by_action_name("Add")`
- Users cannot discover math operations via search: `search_blueprint_actions("add")`
- This creates inconsistent discoverability vs creation capabilities

## Next Steps

1. **Modify SearchBlueprintActions** to include TypePromotion operators
2. **Test mathematical operation search** (Add, Subtract, Multiply, etc.)
3. **Validate integration** with existing search results
4. **Update documentation** to reflect complete search capabilities

## Files to Modify

### Primary Target
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.cpp`

### Key Includes Needed
```cpp
#include "BlueprintTypePromotion.h" // For FTypePromotion access
```

### Key API Calls to Add
```cpp
FTypePromotion::GetOperatorSpawner(OpName)
FTypePromotion::GetUserFacingOperatorName(OpName)  
FTypePromotion::GetKeywordsForOperator(OpName)
```

---

**Analysis Date**: September 21, 2025
**UE Version**: 5.6
**Plugin Status**: Search functionality incomplete - architecture mismatch identified
**Resolution**: Requires integration of TypePromotion system into search logic