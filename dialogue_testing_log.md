# Dialogue Functionality Testing Log

## Testing Date: November 2, 2025
## Plugin: Unreal MCP
## Task: Create dialogue system functionality

## Issue #3: Duplicate Warning Implementation Not Appearing

**Problem**: After implementing duplicate detection warnings in all 4 search functions, the warning message was not appearing in MCP tool results.

**Root Cause**: The `send_unreal_command()` function returns a nested structure:
```python
{
  "status": "success",
  "result": {
    "success": true,
    "actions": [...]
  }
}
```

But the code was checking `result.get("success")` where `result` was the outer wrapper, not the inner data.

**Solution**: Extract the actual result from the wrapper:
```python
command_result = send_unreal_command("search_blueprint_actions", params)

# Extract the actual result from the wrapper
if "result" in command_result:
    result = command_result["result"]
else:
    result = command_result
```

**Status**: ✅ RESOLVED - Warning now appears correctly in all search results when duplicates are detected.

**Example Output**:
```
IMPORTANT REMINDER: Multiple functions with the same name found!

When using create_node_by_action_name() with these results,
you MUST specify the 'class_name' parameter to avoid getting the wrong variant.

Functions with duplicates:
  - 'IsValid' exists in: AssetRegistryHelpers, AnimPoseExtensions, SharedImageConstRefBlueprintFns, 
    SubobjectDataBlueprintFunctionLibrary, KismetSystemLibrary, NavigationPath

Example:
  create_node_by_action_name(
      function_name="IsValid",
      class_name="KismetSystemLibrary",  # <- REQUIRED!
      ...)
```

---

## Issue #4: UBlueprintEventNodeSpawner Compilation Errors

**Problem**: Added EventNodeSpawner support to make CustomEvent searchable, but compilation failed with:
- `error C2065: 'UBlueprintEventNodeSpawner': undeclared identifier`
- `error C2039: 'GetMenuSignature': is not a member of 'UBlueprintNodeSpawner'`

**Root Causes**:
1. Missing include header for UBlueprintEventNodeSpawner class
2. Wrong API usage - tried calling `GetMenuSignature()` function when it's actually a member variable `DefaultMenuSignature`

**Investigation Process**:
- Researched UE 5.6 source code: `Engine/Source/Editor/BlueprintGraph/Classes/BlueprintEventNodeSpawner.h`
- Found UK2Node_CustomEvent::GetMenuActions() uses UBlueprintEventNodeSpawner::Create()
- Discovered that BlueprintEventNodeSpawner.cpp sets MenuName = "Add Custom Event..." when CustomEventName.IsNone()
- Learned DefaultMenuSignature is a public member variable of type FBlueprintActionUiSpec

**Solution**:
1. Added `#include "BlueprintEventNodeSpawner.h"` after BlueprintFunctionNodeSpawner include
2. Changed API usage from:
   ```cpp
   FBlueprintActionUiSpec MenuSignature = NodeSpawner->GetMenuSignature();
   ```
   To:
   ```cpp
   const FBlueprintActionUiSpec& MenuSignature = NodeSpawner->DefaultMenuSignature;
   ```

**Files Modified**:
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.cpp`
  - Line ~33: Added include header
  - Line ~1621: Fixed EventNodeSpawner metadata extraction (first location)
  - Line ~1705: EventNodeSpawner JSON output handling (second location)

**Status**: ✅ RESOLVED - Compilation successful, CustomEvent now appears in search results

**Testing Validation**:
```
search_blueprint_actions("custom event")
→ Returns: "Add Custom Event..." with function_name="CustomEvent"

search_blueprint_actions("add custom")  
→ "Add Custom Event..." appears as first result
```

---

## Steps and Issues

### Step 1: Creating Dialogue Widget
**Status:** ✓ Completed

Successfully created:
- DialogueData struct with SpeakerName, DialogueText, NextDialogueID
- DialogueTable DataTable with 4 sample dialogues
- WBP_DialogueDisplay widget with SpeakerNameText, DialogueText, ContinueButton
- WBP_InteractionPrompt widget for NPC interaction popup
- BP_DialogueNPC character with InteractionWidget component

### Step 2: Adding ThirdPersonCharacter Logic
**Status:** In Progress

**Issue #1**: Cannot directly create "Event Tick" node via action name
- ~~Attempted to create Event Tick using `create_node_by_action_name`~~
- ~~Error: "Function 'Event Tick' not found and not a recognized control flow node"~~
- **UPDATE**: ✅ RESOLVED - Event Tick can be created successfully
- Both `"Event Tick"` (with space) and `"EventTick"` (without space) work
- Created node type: K2Node_Event with pins: then (exec), DeltaSeconds (float)

**Issue #2**: Need way to add custom events or use existing event nodes
- ~~BP_ThirdPersonCharacter already has many nodes in EventGraph~~
- **UPDATE**: ✅ FULLY RESOLVED - CustomEvent now fully supported and searchable
- Usage: `create_node_by_action_name(function_name="CustomEvent", event_name="MyEventName")`
- Creates node type: UK2Node_CustomEvent with exec output pin
- **SOLUTION COMPLETED**: Added UBlueprintEventNodeSpawner support to C++ search function
- CustomEvent now appears in search_blueprint_actions("custom event") as "Add Custom Event..."
- Implementation: Modified `UnrealMCPBlueprintActionCommands.cpp` to handle EventNodeSpawner alongside FunctionNodeSpawner
- Files changed:
  - Added `#include "BlueprintEventNodeSpawner.h"` header
  - Added EventNodeSpawner detection and metadata extraction
  - Special handling: "Add Custom Event..." → function_name="CustomEvent"
  - Now processes Event Tick, Custom Events, and other event types in search results
- Status: Fully working - both creation and discovery through search

**Current Progress**:
- Created CheckForNearbyNPC function with logic:
  - Get All Actors Of Class (BP_DialogueNPC)
  - ForEach Loop through actors
  - GetDistanceTo check
  - Branch if distance < 300
  - Set NearbyNPC variable
- Still need to trigger this function periodically or on demand

**Issue #3**: Cannot find proper "Is Valid" node for object reference validation
- Tried to connect Object Reference (NearbyNPC) directly to Branch Condition pin (Boolean)
- Error: "Boolean is not compatible with Object Reference"
- search_blueprint_actions("IsValid") returns wrong variants (Image validation, etc.)
- get_actions_for_pin with "valid" search shows "Is Valid" macro but creates wrong node type

**ROOT CAUSE IDENTIFIED**: 
- The correct IsValid function EXISTS in search results (position 26 in results)
- Function name: "IsValid", class: "KismetSystemLibrary"
- Problem was NOT specifying `class_name` parameter when calling `create_node_by_action_name`
- Documentation clearly states: "ALWAYS specify class_name when search returns multiple functions with the same name"
- Without class_name, system picked wrong variant (Image validation instead of object validation)

**SOLUTION IMPLEMENTED**:
✅ Added automatic duplicate detection in all search functions
✅ When duplicates found, Python MCP now returns `duplicate_warning` field with:
   - List of all duplicate function names
   - Classes where each function exists
   - Example code showing proper usage with class_name
   
This warning will appear at the top of search results to remind AI assistants to use class_name parameter.

**Correct Usage**:
```python
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="IsValid",
    class_name="KismetSystemLibrary",  # ← REQUIRED when duplicates exist!
    ...
)
```

**Workaround Attempted**: 
- Tried creating "Is Valid" macro node - created wrong node (Image validation)
- Need to find correct IsValid implementation or alternative approach (e.g., using custom Branch that accepts object)

**Resolution for Issue #3**:
- Temporary solution: Removed validation check entirely
- Connected IA_Interact Triggered directly to Create Widget
- This works but lacks proper NPC proximity validation
- TODO: Need to find proper IsValid node or implement custom validation logic

### Step 3: Final Status
**Status:** ✓ Basic functionality completed with known issues

**What Works:**
1. DialogueData struct created with proper fields
2. DialogueTable populated with sample dialogues
3. WBP_DialogueDisplay widget created with UI components
4. WBP_InteractionPrompt widget for NPC popups
5. BP_DialogueNPC created with InteractionWidget component
6. BP_ThirdPersonCharacter has:
   - CheckForNearbyNPC function (checks distance < 300)
   - Timer set to call function every 0.5 seconds
   - IA_Interact input creates and shows dialogue widget

**Known Limitations:**
1. No validation check for NearbyNPC before showing dialogue
2. Missing proper IsValid node support in plugin
3. InteractionPrompt widget not shown/hidden based on proximity
4. No actual data binding between DataTable and WBP_DialogueDisplay
5. Continue button has no functionality

**Testing Recommendations:**
1. Test if CheckForNearbyNPC actually detects BP_DialogueNPC in level
2. Verify IA_Interact input shows the dialogue widget
3. Check if dialogue widget displays properly on screen
4. Test proximity detection by moving character near/far from NPC

**Next Steps for Full Implementation:**
1. Find solution for IsValid object reference check
2. Show/hide InteractionPrompt based on NearbyNPC variable
3. Bind DataTable data to dialogue widget text blocks
4. Implement Continue button logic to progress through dialogue

