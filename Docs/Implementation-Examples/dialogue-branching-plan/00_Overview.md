# Branching Dialogue System Implementation Plan

## Overview

A branching dialogue system for Unreal Engine 5.7 using MCP tools. Players press E near NPCs to open a classic RPG-style dialogue box with typewriter text effect, speaker name, and clickable answer buttons that lead to different conversation paths.

## Design Reference

**All UI implementation MUST follow the designs in `/design/` folder:**
- `dialogue-system-spec.md` - Complete visual specifications (colors, sizes, typography)
- `00_full_dialogue_in_context.png` - Full dialogue window in game context
- `01_dialogue_window_complete.png` - Complete window layout reference
- `02_interaction_indicator.png` - "Press E to talk" indicator
- `03_answer_button_states.png` - Button normal/hover/pressed/disabled states
- `04_speaker_name_badge.png` - Speaker name styling
- `05_dialogue_text_area.png` - Text area with typewriter effect
- `06_dialogue_frame.png` - Main container frame with decorations
- `07_continue_indicators.png` - Continue prompt styles
- `08_dialogue_dividers.png` - Section dividers

## What Already Exists

### UI Widgets (visual complete, graphs empty)
- `WBP_InteractionIndicator` - "Press E to talk" floating indicator
- `WBP_DialogueWindow` - Main dialogue box with speaker name, text area, answers container
- `WBP_AnswerButton` - Clickable answer option with number badge

### Data Layer
- `S_DialogueAnswer` - Answer struct (AnswerText, NextDialogueID)
- `S_DialogueRow` - Dialogue row struct (DialogueID, SpeakerName, DialogueText, Answers[], SoundCue)
- `DT_DialogueSample` - Sample dialogue DataTable

### Folder Structure
```
/Game/Dialogue/
  ├── Data/
  │   ├── S_DialogueAnswer
  │   ├── S_DialogueRow
  │   └── DT_DialogueSample
  ├── Blueprints/
  │   └── (empty - to be created)
  └── UI/
      ├── WBP_InteractionIndicator
      ├── WBP_DialogueWindow
      └── WBP_AnswerButton
```

---

## Implementation Steps (Vertical Slices)

Each step produces **testable behavior in PIE**. No step requires more than 10 Blueprint nodes.

---

### Step 1: Show Interaction Indicator in World

**Build:**
- Create `BP_DialogueNPC` Actor at `/Game/Dialogue/Blueprints/`
- Add `WidgetComponent` named "InteractionWidget"
- Set WidgetComponent's Widget Class to `WBP_InteractionIndicator`
- Set Widget to Screen Space, draw size 200x50

**Test:** Place BP_DialogueNPC in level, Play. See "Press E to talk" floating above actor.

**Expected:** Indicator visible in world space near the NPC actor.

---

### Step 2: Show/Hide Indicator on Player Proximity

**Build:**
- Add `SphereComponent` named "InteractionSphere" (radius 300) to BP_DialogueNPC
- Add `bPlayerInRange` Boolean variable
- BeginPlay: Hide InteractionWidget
- OnBeginOverlap: If Player → Show InteractionWidget, set bPlayerInRange=true
- OnEndOverlap: Hide InteractionWidget, set bPlayerInRange=false

**Test:** Play. Walk toward NPC → indicator appears. Walk away → indicator disappears.

**Expected:** Indicator toggles based on player distance.

---

### Step 3: Detect E Press Near NPC

**Build:**
- `IA_Interact` already exists at `/Game/ThirdPerson/Input/Actions/IA_Interact`
- In `BP_ThirdPersonCharacter`: Add IA_Interact event node → Print String "E pressed!"

**Test:** Play. Press E anywhere → see debug message.

**Expected:** "E pressed!" appears in viewport.

---

### Step 4: Find NPC When E Pressed

**Build:**
- In BP_ThirdPersonCharacter IA_Interact handler:
  - SphereOverlapActors at player location (radius 300)
  - ForEach: Cast to BP_DialogueNPC
  - If cast succeeds → Print String "Found NPC!"

**Test:** Play. Press E away from NPC → nothing. Press E near NPC → "Found NPC!"

**Expected:** Only detects NPC when player is close.

---

### Step 5: Show Dialogue Window (Hardcoded)

**Build:**
- Add `DialogueWindowClass` variable to BP_DialogueNPC (Class type, default WBP_DialogueWindow)
- Add `DialogueWindowRef` variable (Object type)
- Add `StartDialogue` function to BP_DialogueNPC:
  - Create Widget (DialogueWindowClass)
  - Add to Viewport
  - Store in DialogueWindowRef
- In BP_ThirdPersonCharacter: When NPC found → Call NPC.StartDialogue

**Test:** Play. Walk to NPC, press E → black dialogue box appears at screen bottom.

**Expected:** Dialogue window visible with existing UI layout (hardcoded text from widget design).

---

### Step 6: Close Dialogue with ESC

**Build:**
- Create `IA_CloseDialogue` input action, map to Escape key
- Add `bDialogueActive` Boolean to BP_DialogueNPC
- Add `EndDialogue` function to BP_DialogueNPC:
  - Remove DialogueWindowRef from parent
  - Set bDialogueActive = false
  - Set Input Mode Game Only
- In StartDialogue: Set bDialogueActive = true, Set Input Mode UI Only
- In BP_ThirdPersonCharacter: IA_CloseDialogue → Find active NPC → Call EndDialogue

**Test:** Play. Press E → dialogue opens, can't move. Press ESC → dialogue closes, can move.

**Expected:** Clean open/close cycle with proper input mode switching.

---

### Step 7: Display Speaker Name from Variable

**Build:**
- Add `SpeakerName` (String) variable to BP_DialogueNPC, exposed, default "Village Elder"
- Add `NPCRef` variable to WBP_DialogueWindow (Object type)
- Add `InitializeDialogue(NPC)` function to WBP_DialogueWindow:
  - Store NPC in NPCRef
  - Cast to BP_DialogueNPC
  - Get SpeakerName → Set SpeakerNameText.Text
- In BP_DialogueNPC.StartDialogue: Call DialogueWindow.InitializeDialogue(self)

**Test:** Play. Press E near NPC → dialogue shows "Village Elder" as speaker name.

**Expected:** Speaker name displays from NPC variable, not hardcoded.

---

### Step 8: Display Dialogue Text from DataTable

**Build:**
- Add `DialogueTable` (DataTable) variable to BP_DialogueNPC, exposed
- Add `StartingDialogueID` (Name) variable, exposed, default "Greeting"
- Add `CurrentDialogueID` (Name) variable
- Add `GetDialogueRow(ID) → S_DialogueRow` function to BP_DialogueNPC
- Modify InitializeDialogue: Get starting row → Set DialogueText.Text
- Set DialogueTable default to DT_DialogueSample on placed NPC

**Test:** Play. Press E → dialogue shows text from DataTable "Greeting" row.

**Expected:** Dialogue text comes from DataTable, not hardcoded.

---

### Step 9: Create Answer Buttons from DataTable

**Build:**
- Add `AnswerButtonClass` (Class) variable to WBP_DialogueWindow, default WBP_AnswerButton
- Add `PopulateAnswers(Answers[])` function to WBP_DialogueWindow:
  - Clear AnswersContainer children
  - ForEach Answer: Create AnswerButton widget, add to AnswersContainer
- Add `SetAnswerText(Text, Index)` function to WBP_AnswerButton:
  - Set AnswerText.Text, Set NumberText.Text to Index+1
- Call PopulateAnswers after displaying dialogue text

**Test:** Play. Press E → see answer buttons with text from DataTable.

**Expected:** Answer buttons appear with correct text and numbering.

---

### Step 10: Answer Button Click Handler

**Build:**
- Add `AnswerIndex` (Integer), `NextDialogueID` (Name) variables to WBP_AnswerButton
- Add `ParentWindow` (Object) variable to WBP_AnswerButton
- Modify SetAnswerText to also set AnswerIndex, NextDialogueID, ParentWindow
- Bind MainButton.OnClicked → Call parent window's OnAnswerSelected

**Test:** Play. Click answer button → Print String showing which answer was clicked.

**Expected:** Button clicks are detected and routed to parent.

---

### Step 11: Branch to Next Dialogue

**Build:**
- Add `ShowNextDialogue(NextID)` function to BP_DialogueNPC:
  - If NextID empty → EndDialogue
  - Else → Get row, update CurrentDialogueID, call DisplayDialogue
- Add `DisplayDialogue(Row)` function to WBP_DialogueWindow:
  - Set speaker name, dialogue text, populate answers
- Wire OnAnswerSelected to call NPC.ShowNextDialogue

**Test:** Play. Click answer → dialogue updates to next entry. Click final answer → dialogue closes.

**Expected:** Full branching dialogue navigation works.

---

### Step 12: Typewriter Effect

**Build:**
- Add `FullDialogueText` (String), `CurrentCharIndex` (Integer) to WBP_DialogueWindow
- Add `TypewriterSpeed` (Float) variable, default 0.03
- Add `StartTypewriter(Text)` function: Store text, reset index, set timer
- Add `TypewriterTick()` function: Reveal next char, check if complete
- Replace direct text set with StartTypewriter call

**Test:** Play. Press E → text types out character by character.

**Expected:** Typewriter animation at configured speed.

---

### Step 13: Skip Typewriter on Click

**Build:**
- Add click handler to dialogue text area
- On click: If typewriter running → show full text immediately

**Test:** Play. During typewriter animation, click → text completes instantly.

**Expected:** Players can skip the animation.

---

## Future Steps (Optional Polish)

- Step 14: Answer button hover effects
- Step 15: Sound effects (dialogue appear, button click, typewriter)
- Step 16: Fade transitions
- Step 17: Portrait/avatar display

---

## Known Considerations

- Cast nodes must have execution pins connected (impure nodes)
- Always specify `class_name` when creating function nodes to avoid wrong variants
- Compile after completing each function, not after every node
- GUID-based field names for DataTable - must fetch before adding rows
