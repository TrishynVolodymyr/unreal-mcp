# Blueprint Action Commands Refactoring Guide

## Overview
This refactoring splits the monolithic `UnrealMCPBlueprintActionCommands.cpp` (1780+ lines) into modular services for better maintainability.

## File Structure

```
Services/BlueprintAction/
├── BlueprintActionDiscoveryService.h/cpp      # Base service with shared utilities
├── BlueprintPinSearchService.h/cpp            # Pin-type based action search
├── BlueprintClassSearchService.h/cpp          # Class-based action search
├── BlueprintActionSearchService.h/cpp         # General keyword-based search
└── BlueprintNodePinInfoService.h/cpp          # Node pin information lookup
```

## Migration Plan

### Phase 1: Move Shared Utilities to BlueprintActionDiscoveryService

**Functions to move:**
1. `ProcessFunctionNodeSpawner()` - Extract FunctionNodeSpawner metadata (lines ~1574-1610)
2. `ProcessEventNodeSpawner()` - Extract EventNodeSpawner metadata (lines ~1617-1640)
3. `BuildActionResult()` - Common JSON result building pattern
4. `DetectDuplicateFunctions()` - Find duplicate function names across classes
5. `AddDuplicateWarning()` - Build duplicate warning message

**Location in original file:** Lines 1574-1640 (spawner processing), scattered throughout for JSON building

---

### Phase 2: Move BlueprintPinSearchService::GetActionsForPin

**Source:** `UnrealMCPBlueprintActionCommands.cpp` lines 357-690

**Main function:** `GetActionsForPin(PinType, PinSubCategory, SearchFilter, MaxResults)`

**Helper functions to move:**
1. `ResolveShortClassName()` - Maps "PlayerController" to "/Script/Engine.PlayerController" (lines ~367-373)
2. `IsRelevantForMathOperations()` - Check if node is math-related (lines ~455-491)
3. `IsRelevantForObjectType()` - Check class compatibility (lines ~494-507)
4. `AddNativePropertyNodes()` - Add property getter/setter nodes (lines ~617-690)

**IMPORTANT:** Keep the TryFindTypeSlow optimization we added!
- Line ~382-395: TargetClass resolved ONCE before loop (not inside loop)

**Key sections:**
1. Lines 365-375: Pin subcategory resolution
2. Lines 382-395: Target class resolution (OPTIMIZED)
3. Lines 397-615: Main action registry iteration
4. Lines 617-690: Native property nodes addition

---

### Phase 3: Move BlueprintClassSearchService Functions

#### GetActionsForClass
**Source:** Lines 693-910

**Main function:** `GetActionsForClass(ClassName, SearchFilter, MaxResults)`

**Helper:** `ResolveClass()` - Class resolution with fallback strategies (lines ~699-723)

**Key sections:**
1. Lines 699-723: Class resolution (direct, /Script/Engine, /Game/Blueprints)
2. Lines 725-850: Action database iteration and filtering
3. Lines 852-910: JSON result building

#### GetActionsForClassHierarchy
**Source:** Lines 912-1179

**Main function:** `GetActionsForClassHierarchy(ClassName, SearchFilter, MaxResults)`

**Helpers:**
1. `BuildClassHierarchy()` - Walk parent class chain (lines ~947-960)
2. `CountActionsByCategory()` - Group and count by category (lines ~1122-1140)

**Key sections:**
1. Lines 920-945: Class resolution
2. Lines 947-960: Build class hierarchy using GetSuperClass()
3. Lines 962-1120: Collect actions from all classes in hierarchy
4. Lines 1122-1140: Category counting
5. Lines 1142-1179: JSON result with hierarchy info

---

### Phase 4: Move BlueprintNodePinInfoService::GetNodePinInfo

**Source:** Lines 1181-1309

**Main function:** `GetNodePinInfo(NodeName, PinName)`

**Helpers:**
1. `GetKnownPinInfo()` - Lookup from hardcoded database (lines ~1190-1280)
2. `GetAvailablePinsForNode()` - List known pins for node (lines ~1297-1305)
3. `BuildPinInfoResult()` - JSON result building

**Key sections:**
1. Lines 1190-1280: Hardcoded pin info database for known nodes
   - Create Widget node
   - Get Controller node
   - Cast nodes
2. Lines 1282-1295: Pin lookup logic
3. Lines 1297-1309: Error handling and available pins listing

---

### Phase 5: Move BlueprintActionSearchService::SearchBlueprintActions

**Source:** Lines 1311-1783 (LARGEST FUNCTION!)

**Main function:** `SearchBlueprintActions(SearchQuery, Category, MaxResults, BlueprintName)`

**Helpers:**
1. `MatchesSearchQuery()` - Check if action matches search (lines ~1450-1470)
2. `MatchesCategoryFilter()` - Check category filter (lines ~1472-1480)
3. `DiscoverLocalVariables()` - Find Blueprint variables (lines ~1327-1390)

**Key sections:**
1. Lines 1318-1325: Blueprint lookup for local variables
2. Lines 1327-1390: Local variable discovery (Get/Set nodes)
3. Lines 1392-1700: Main action database iteration
4. Lines 1450-1470: Search filtering (case-insensitive across name/category/tooltip/keywords)
5. Lines 1472-1480: Category filtering
6. Lines 1702-1783: JSON building with duplicate detection

---

### Phase 6: Update UnrealMCPBlueprintActionCommands.cpp

After moving all implementations, this file should only contain:

```cpp
FString UUnrealMCPBlueprintActionCommands::GetActionsForPin(...)
{
    FBlueprintPinSearchService Service;
    return Service.GetActionsForPin(PinType, PinSubCategory, SearchFilter, MaxResults);
}

FString UUnrealMCPBlueprintActionCommands::GetActionsForClass(...)
{
    FBlueprintClassSearchService Service;
    return Service.GetActionsForClass(ClassName, SearchFilter, MaxResults);
}

// ... similar wrappers for other functions ...

FString UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(...)
{
    // This already delegates to UnrealMCPNodeCreators - no change needed
    return UnrealMCPNodeCreators::CreateNodeByActionName(BlueprintName, FunctionName, ClassName, NodePosition, JsonParams);
}
```

## Testing Checklist

After migration, verify:
- [ ] `get_actions_for_pin` works and doesn't hang (TryFindTypeSlow optimization preserved)
- [ ] `get_actions_for_class` returns correct functions
- [ ] `get_actions_for_class_hierarchy` includes parent class functions
- [ ] `search_blueprint_actions` finds actions correctly
- [ ] `inspect_node_pin_connection` returns pin info
- [ ] Duplicate function warnings still appear
- [ ] CustomEvent ("Add Custom Event...") appears in searches
- [ ] No compilation errors

## Important Notes

1. **Preserve TryFindTypeSlow Optimization**: In `GetActionsForPin`, we moved `UClass::TryFindTypeSlow()` OUTSIDE the main loop to prevent O(n²) complexity
2. **Duplicate Detection**: Make sure the duplicate warning logic is properly called from all search functions
3. **JSON Building**: Each service should use the base class `BuildActionResult()` for consistency
4. **Error Handling**: Preserve all UE_LOG statements for debugging

## Compilation Dependencies

Each service needs these includes:
- `BlueprintActionDatabase.h`
- `BlueprintFunctionNodeSpawner.h`
- `BlueprintEventNodeSpawner.h`
- `K2Node_CallFunction.h`
- `Kismet/KismetMathLibrary.h`
- `Kismet/KismetSystemLibrary.h`
- `Dom/JsonObject.h`
- `Serialization/JsonSerializer.h`
