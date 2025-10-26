# Dialogue System Implementation - Issues & Problems Log

**Date:** October 26, 2025
**Task:** Create dialogue functionality for Third Person Character

## Implementation Steps & Issues

### Step 1: Creating Dialogue Widget
**Status:** ✅ Completed
**Description:** Creating UMG widget for displaying dialogue text
- Created WBP_DialogueWidget successfully
- Added DialogueBackground (Border), SpeakerNameText (TextBlock), DialogueText (TextBlock)

### Step 2: Creating Dialogue Structure and DataTable
**Status:** ✅ Completed
**Description:** Creating DialogueStruct and DT_DialogueData with example dialogues
- Created DialogueStruct with fields: SpeakerName, DialogueText, ResponseOptions
- Created DT_DialogueData DataTable
- Added 3 example dialogue rows in Ukrainian

### Step 3: Creating Dialogue NPC Blueprint
**Status:** ✅ Completed
**Description:** Creating BP_DialogueNPC with interaction components
- Created BP_DialogueNPC (Character)
- Added InteractionWidget (WidgetComponent) and InteractionSphere (SphereComponent)
- Created WBP_InteractionPrompt widget with prompt text
- Set WidgetClass on InteractionWidget to WBP_InteractionPrompt
- Added variables: DialogueRowName, bCanInteract
- Created SetInteractionVisibility function with full logic

### Step 4: Modifying Third Person Character
**Status:** ✅ Completed (with manual steps required)
**Description:** Adding dialogue interaction to BP_ThirdPersonCharacter
- ✅ Added variables: CurrentNearbyNPC, InteractionRadius, DialogueWidgetRef
- ✅ Created CheckForNearbyNPC function with GetAllActorsOfClass node
- ✅ Created StartDialogue function with Create Widget → Add to Viewport logic
- ✅ Found existing IA_Interact Input Action already bound to 'E' key
- ✅ Created Event Tick → CheckForNearbyNPC connection
- ✅ Created OnInteractPressed Custom Event → StartDialogue connection
- ✅ Both blueprints compiled successfully

### Step 5: Widget Components and Layout
**Status:** ✅ Completed
**Description:** Created all necessary UI widgets
- ✅ WBP_DialogueWidget: Main dialogue display with background, speaker name, dialogue text
- ✅ WBP_InteractionPrompt: Popup widget for interaction hint (shows "Натисніть [E] для діалогу")
- ✅ Configured InteractionWidget component on BP_DialogueNPC

**Issue #1: Creating Enhanced Input Event Node**
- Attempted to create EnhancedInputAction event node for IA_Interact
- `search_blueprint_actions` didn't find the specific action
- Created CustomEvent instead, but need proper Input Action event binding
- **Solution:** Created CustomEvent and connected to StartDialogue function call
- ⚠️ **Manual step needed:** User needs to manually bind IA_Interact to OnInteractPressed in Blueprint editor

**Issue #2: ❌ CRITICAL - Unreal Editor Crash**
- **Error:** `Attempted to create a package with name containing double slashes`
- **Full Path:** `/Game/Widgets//Game/Dialogue/BP_DialogueNPC`
- **Root Cause:** `set_node_pin_value` with ActorClass value caused path concatenation bug in C++ plugin
- **Stack Trace:** `FSetNodePinValueCommand::Execute()` → `FUnrealMCPCommonUtils::FindWidgetClass()`
- **Location:** `UnrealMCPCommonUtils.cpp:1493`
- **Action Taken:** 
  - Attempted to set ActorClass pin to `/Game/Dialogue/BP_DialogueNPC.BP_DialogueNPC_C`
  - Plugin incorrectly prepended `/Game/Widgets/` path
  - Resulted in double-slash path: `/Game/Widgets//Game/Dialogue/BP_DialogueNPC`
- **Impact:** Unreal Editor crashed with Fatal Error
- **Bug Location:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Utils/UnrealMCPCommonUtils.cpp` line 1493
- **Recommendation:** Fix path handling in `FindWidgetClass()` to avoid prepending when full path is provided

**🐛 Plugin Bug Found:**
The `FSetNodePinValueCommand` appears to have a bug where it assumes class paths need `/Game/Widgets/` prefix, even when a full path is already provided. This causes invalid package names with double slashes.

**⚠️ Workaround:** Cannot set ActorClass pin via MCP tools due to plugin bug. 

---

### Manual Setup Required (Due to Plugin Limitations)

**Step 1: Bind Input Action to Custom Event**
- Open `BP_ThirdPersonCharacter` in Blueprint Editor
- Find the `OnInteractPressed` Custom Event node
- Add an `IA_Interact` Enhanced Input Action event
- Connect `IA_Interact` (Triggered) → `OnInteractPressed` event call

**Step 2: Set ActorClass in CheckForNearbyNPC Function**
- Open `BP_ThirdPersonCharacter` → Functions → `CheckForNearbyNPC`
- Find the `Get All Actors Of Class` node
- Set `ActorClass` pin to `BP_DialogueNPC` class

**Step 3: Test the Dialogue System**
- Place a `BP_DialogueNPC` in the level
- Set its `DialogueRowName` to one of: `Greeting_1`, `Quest_Offer`, or `Shop_Welcome`
- Play and approach the NPC - interaction prompt should appear
- Press E to start dialogue

---

