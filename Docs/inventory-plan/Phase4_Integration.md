# Phase 4: Integration Layer

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Connect the inventory system to the player character and input system:
1. Create Enhanced Input Action for toggling inventory
2. Add BP_InventoryManager component to player character
3. Implement ToggleInventory function
4. Bind input to toggle function

## Dependencies

- Phase 1-3 completed
- Required assets:
  - BP_InventoryManager component
  - WBP_InventoryGrid widget
  - Player character blueprint (e.g., BP_ThirdPersonCharacter)

---

## Step 4.1: Create Enhanced Input Action

### AIM
Create an input action asset for toggling the inventory UI using the Enhanced Input System (UE 5.5+).

### USAGE EXAMPLES
```python
# Player presses I key
# IA_ToggleInventory triggers
# ToggleInventory function called
```

### WORKFLOW
1. Create Enhanced Input Action asset
2. Configure as Digital (on/off) type
3. Will be mapped to I key

### MCP TOOL CALL

```python
create_enhanced_input_action(
    action_name="IA_ToggleInventory",
    path="/Game/Input/Actions",
    value_type="Digital",
    description="Toggle inventory UI visibility"
)
```

### VERIFICATION
- [ ] IA_ToggleInventory created at /Game/Input/Actions/IA_ToggleInventory
- [ ] Value type is Digital (Boolean)

---

## Step 4.2: Create Input Mapping Context (if needed)

### AIM
Create or update the input mapping context to include the inventory toggle action.

### WORKFLOW
1. Check if IMC_Default exists
2. Create if not present
3. Add mapping for IA_ToggleInventory -> I key

### MCP TOOL CALLS

```python
# Create input mapping context (if not exists)
create_input_mapping_context(
    context_name="IMC_Default",
    path="/Game/Input",
    description="Default input mapping context for player actions"
)

# Add mapping: I key -> IA_ToggleInventory
add_mapping_to_context(
    context_path="/Game/Input/IMC_Default",
    action_path="/Game/Input/Actions/IA_ToggleInventory",
    key="I",
    trigger_type="Pressed"
)
```

### VERIFICATION
- [ ] IMC_Default exists
- [ ] I key mapped to IA_ToggleInventory
- [ ] Trigger on key press (not hold)

---

## Step 4.3: Add Inventory Manager to Player Character

### AIM
Add the BP_InventoryManager component to the player character so they have inventory functionality.

### USAGE EXAMPLES
```python
# Player character now has:
# - InventoryManager component
# - Access to AddItem, RemoveItem, etc.
```

### WORKFLOW
1. Identify player character blueprint (e.g., BP_ThirdPersonCharacter)
2. Add BP_InventoryManager as component
3. Component auto-initializes on BeginPlay

### MCP TOOL CALL

```python
add_component_to_blueprint(
    blueprint_name="BP_ThirdPersonCharacter",  # Adjust to actual player BP name
    component_type="/Game/Inventory/Blueprints/BP_InventoryManager",
    component_name="InventoryManager"
)
```

### VERIFICATION
- [ ] InventoryManager component added to player
- [ ] Component visible in Components panel

---

## Step 4.4: Add Player UI Variables

### AIM
Add variables to the player character for managing the inventory UI state.

### MCP TOOL CALLS

```python
# Reference to the inventory widget (when open)
add_blueprint_variable(
    blueprint_name="BP_ThirdPersonCharacter",
    variable_name="InventoryWidget",
    variable_type="/Game/Inventory/UI/WBP_InventoryGrid",
    is_exposed=False
)

# Track if inventory is currently open
add_blueprint_variable(
    blueprint_name="BP_ThirdPersonCharacter",
    variable_name="IsInventoryOpen",
    variable_type="Boolean",
    is_exposed=False
)

# Reference to the InventoryManager component
add_blueprint_variable(
    blueprint_name="BP_ThirdPersonCharacter",
    variable_name="InventoryManagerRef",
    variable_type="/Game/Inventory/Blueprints/BP_InventoryManager",
    is_exposed=False
)
```

### VERIFICATION
- [ ] InventoryWidget variable added
- [ ] IsInventoryOpen variable added
- [ ] InventoryManagerRef variable added

---

## Step 4.5: Create ToggleInventory Function

### AIM
Create a function that opens or closes the inventory UI based on current state.

### USAGE EXAMPLES
```python
# Press I (inventory closed)
ToggleInventory()
# Result: Inventory opens, mouse cursor shown

# Press I (inventory open)
ToggleInventory()
# Result: Inventory closes, mouse cursor hidden
```

### WORKFLOW
1. Check IsInventoryOpen state
2. If open (closing):
   - Remove InventoryWidget from viewport
   - Set InventoryWidget to null
   - Set IsInventoryOpen to false
   - Set input mode to Game Only
   - Hide mouse cursor
3. If closed (opening):
   - Create WBP_InventoryGrid widget
   - Call InitializeGrid with InventoryManager
   - Add to viewport
   - Store in InventoryWidget
   - Set IsInventoryOpen to true
   - Set input mode to Game and UI
   - Show mouse cursor

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="ToggleInventory",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Inventory"
)

# Implementation nodes:

# Branch on IsInventoryOpen
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Branch",
    target_graph="ToggleInventory",
    node_position=[200, 0]
)

# === TRUE BRANCH (Closing) ===

# Remove from Parent
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Remove from Parent",
    target_graph="ToggleInventory",
    node_position=[400, -100]
)

# Set IsInventoryOpen = false
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set IsInventoryOpen",
    target_graph="ToggleInventory",
    node_position=[600, -100]
)

# Set Input Mode Game Only
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set Input Mode Game Only",
    class_name="WidgetBlueprintLibrary",
    target_graph="ToggleInventory",
    node_position=[800, -100]
)

# Set Show Mouse Cursor = false
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set Show Mouse Cursor",
    target_graph="ToggleInventory",
    node_position=[1000, -100]
)

# === FALSE BRANCH (Opening) ===

# Create Widget (WBP_InventoryGrid)
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Create Widget",
    target_graph="ToggleInventory",
    node_position=[400, 100]
)

# Call InitializeGrid
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="InitializeGrid",
    target_graph="ToggleInventory",
    node_position=[600, 100]
)

# Add to Viewport
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Add to Viewport",
    target_graph="ToggleInventory",
    node_position=[800, 100]
)

# Set InventoryWidget
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set InventoryWidget",
    target_graph="ToggleInventory",
    node_position=[1000, 100]
)

# Set IsInventoryOpen = true
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set IsInventoryOpen",
    target_graph="ToggleInventory",
    node_position=[1200, 100]
)

# Set Input Mode Game and UI
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set Input Mode Game And UI",
    class_name="WidgetBlueprintLibrary",
    target_graph="ToggleInventory",
    node_position=[1400, 100]
)

# Set Show Mouse Cursor = true
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set Show Mouse Cursor",
    target_graph="ToggleInventory",
    node_position=[1600, 100]
)

# Connect all nodes appropriately
connect_blueprint_nodes(
    blueprint_name="BP_ThirdPersonCharacter",
    connections=[
        # Connect execution flow for both branches
        # Connect data pins for widget references
        # ...
    ],
    target_graph="ToggleInventory"
)

auto_arrange_nodes(
    blueprint_name="BP_ThirdPersonCharacter",
    graph_name="ToggleInventory"
)
```

### VERIFICATION
- [ ] Function created
- [ ] Both branches implemented (open/close)
- [ ] Widget created and initialized on open
- [ ] Widget removed on close
- [ ] Input mode changes appropriately
- [ ] Mouse cursor visibility toggles

---

## Step 4.6: Bind Input Action to Toggle Function

### AIM
Connect the IA_ToggleInventory input action to the ToggleInventory function.

### WORKFLOW
1. Create Enhanced Input Action event in Event Graph
2. Connect to ToggleInventory function call

### MCP TOOL CALLS

```python
# Create Enhanced Input Action event
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="IA_ToggleInventory",
    class_name="EnhancedInputAction",
    target_graph="EventGraph",
    node_position=[0, 500]
)

# Create function call node
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="ToggleInventory",
    target_graph="EventGraph",
    node_position=[300, 500]
)

# Connect action to function
connect_blueprint_nodes(
    blueprint_name="BP_ThirdPersonCharacter",
    connections=[
        {
            "source_node_id": "<IA_ToggleInventory_node_id>",
            "source_pin": "Triggered",
            "target_node_id": "<ToggleInventory_node_id>",
            "target_pin": "Execute"
        }
    ],
    target_graph="EventGraph"
)
```

### VERIFICATION
- [ ] Input action event in Event Graph
- [ ] Connected to ToggleInventory call
- [ ] Pressing I triggers the function

---

## Step 4.7: Initialize Component Reference in BeginPlay

### AIM
Store a reference to the InventoryManager component for easy access.

### WORKFLOW
1. In BeginPlay, get InventoryManager component reference
2. Store in InventoryManagerRef variable

### MCP TOOL CALLS

```python
# Get component by class
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Get Component by Class",
    target_graph="EventGraph",
    node_position=[200, 0]
)

# Set InventoryManagerRef
create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="Set InventoryManagerRef",
    target_graph="EventGraph",
    node_position=[400, 0]
)

# Connect to existing BeginPlay flow
connect_blueprint_nodes(
    blueprint_name="BP_ThirdPersonCharacter",
    connections=[
        # Connect after other BeginPlay logic
    ],
    target_graph="EventGraph"
)
```

### VERIFICATION
- [ ] InventoryManagerRef populated on BeginPlay
- [ ] Can access inventory functions via reference

---

## Step 4.8: Compile Player Character

```python
compile_blueprint(blueprint_name="BP_ThirdPersonCharacter")
```

### VERIFICATION
- [ ] Blueprint compiles without errors
- [ ] All new functionality accessible

---

## Step 4.9: Test Item Pickup (Optional)

### AIM
Add test functionality to give items to the player for testing.

### USAGE EXAMPLES
```python
# Debug command or trigger
AddTestItems()
# Adds sample items to inventory for testing
```

### WORKFLOW
Create a debug function that adds sample items:
1. Get InventoryManager reference
2. Call AddItem for each test item

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="AddTestItems",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Debug"
)

# Implementation:
# InventoryManagerRef.AddItem("HealthPotion", 5)
# InventoryManagerRef.AddItem("IronSword", 1)
# InventoryManagerRef.AddItem("IronOre", 25)
# InventoryManagerRef.AddItem("AncientKey", 1)
```

### VERIFICATION
- [ ] AddTestItems function created
- [ ] Calling it populates inventory

---

## Phase 4 Completion Checklist

### Input System
- [ ] IA_ToggleInventory input action created
- [ ] I key mapped to action
- [ ] Input mapping context configured

### Player Character
- [ ] BP_InventoryManager component added
- [ ] InventoryWidget variable added
- [ ] IsInventoryOpen variable added
- [ ] InventoryManagerRef variable added
- [ ] ToggleInventory function implemented
- [ ] Input action bound to toggle function
- [ ] BeginPlay initializes references

### Functionality
- [ ] Press I opens inventory
- [ ] Press I again closes inventory
- [ ] Mouse cursor shows when inventory open
- [ ] Mouse cursor hides when inventory closed
- [ ] Input mode switches appropriately

### All Done Via MCP Tools
- [ ] `create_enhanced_input_action` used
- [ ] `create_input_mapping_context` used
- [ ] `add_mapping_to_context` used
- [ ] `add_component_to_blueprint` used
- [ ] `add_blueprint_variable` used
- [ ] `create_custom_blueprint_function` used
- [ ] `create_node_by_action_name` used
- [ ] `connect_blueprint_nodes` used
- [ ] `compile_blueprint` successful

### Ready for Phase 5
- [ ] Integration complete and functional
- [ ] Proceed to Phase 5: Testing

---

## Next Phase

Continue to [Phase5_Testing.md](Phase5_Testing.md) for comprehensive testing and verification.
