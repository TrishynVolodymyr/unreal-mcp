# Blueprint Math Search Fix Analysis

## Problem Identified
- **Creating math nodes works**: When we specify exact function names like `Subtract_VectorVector`, `Less_DoubleDouble`, it creates nodes successfully
- **Search doesn't find math operations**: `search_blueprint_actions` returns 0 results for "subtract", "add", "+", "-" etc.

## Root Cause Analysis
The issue is in `SearchBlueprintActions` function in `UnrealMCPBlueprintActionCommands.cpp` line ~1280-1340.

### Current Search Logic:
```cpp
// Go through all actions in the database
for (auto Iterator(ActionRegistry.CreateConstIterator()); Iterator; ++Iterator)
{
    const FBlueprintActionDatabase::FActionList& ActionList = Iterator.Value();
    for (UBlueprintNodeSpawner* NodeSpawner : ActionList)
    {
        if (!NodeSpawner)
        {
            continue;
        }
    
    UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();  // <-- PROBLEM: This returns null for KismetMathLibrary functions!
    if (!TemplateNode)
    {
        continue;  // <-- Math functions get skipped here!
    }
```

### Why This Fails:
1. KismetMathLibrary functions are UBlueprintFunctionLibrary static functions
2. These don't have TemplateNode instances like regular Blueprint nodes
3. Our code skips all actions where GetTemplateNode() returns null
4. This excludes ALL KismetMathLibrary functions from search results

## Solution Required:
Modify the search logic to handle UBlueprintFunctionLibrary functions differently:

1. Check if NodeSpawner is for a function call
2. Extract function info from the spawner itself, not from TemplateNode
3. Use FunctionReference to get display names, categories, and metadata
4. Add proper handling for BlueprintPure functions from KismetMathLibrary

## Files to Fix:
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.cpp`
- Function: `SearchBlueprintActions` (line ~1280)

## Expected Result:
After fix:
- `search_blueprint_actions("subtract")` should find `Subtract_VectorVector`, `Subtract_Vector2DVector2D`, etc.
- `search_blueprint_actions("add")` should find `Add_VectorVector`, etc.
- `search_blueprint_actions("<")` should find `Less_DoubleDouble`, `Less_IntInt`, etc.

## Test Cases After Fix:
```python
# Should return math functions
search_blueprint_actions("subtract")  # Should find Subtract_VectorVector
search_blueprint_actions("add")       # Should find Add_VectorVector  
search_blueprint_actions("multiply")  # Should find Multiply_VectorFloat
search_blueprint_actions("<")         # Should find Less_DoubleDouble
search_blueprint_actions("distance")  # Should find VSize, GetDistanceTo
```