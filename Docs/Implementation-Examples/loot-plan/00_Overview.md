# Looting System - Overview

> **STATUS: 📋 PLANNED**

> **REMINDER**: ALL functionality in this looting system MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted. This serves as a comprehensive test of AI-driven Blueprint development.

## ⚠️ TEST-FIRST PHASE STRUCTURE

This plan follows the **UI-First, Data-Second, Logic-Last** methodology (see CLAUDE.md).

**Each phase ends with a USER TEST checkpoint** - something the user can launch Unreal and verify works before proceeding.

---

## Implementation Status

| Phase | Status | Testable Deliverable |
|-------|--------|---------------------|
| Phase 1: Loot Window Skeleton | 📋 PLANNED | Visual loot window with hardcoded slots |
| Phase 2: Drag Widget | 📋 PLANNED | Drag widget follows cursor |
| Phase 3: Inventory Drag Support | 📋 PLANNED | Drag from inventory shows overlay |
| Phase 4: Lootable Actor + Data | 📋 PLANNED | Loot window shows real items from chest |
| Phase 5: E-Key Integration | 📋 PLANNED | E key opens loot or dialogue contextually |

---

## User Requirements

- **E key interaction** - Same key as dialogue, context-aware (NPC = dialogue, Loot = loot window)
- **Side-by-side layout** - Inventory on right, loot on left
- **Auto-find slot on drop** - Drop anywhere on container, item goes to first available slot
- **Grey overlay** - Dragged slot shows grey overlay (overlay on top, not modify original)
- **Full stack transfer** - For now, transfer entire stack on drag-drop
- **Class inheritance** - Base classes for shared slot/container functionality

---

## Architecture

```
INTERFACE
BPI_Interactable - Shared by NPCs and lootables
├── CanInteract(Interactor) → bool
├── GetInteractionPrompt() → String
└── OnInteract(Interactor) → void

BASE WIDGET CLASSES (created in Phase 1)
WBP_ItemSlotBase (common slot)
├── SlotBackground, ItemIcon, StackCountText, DragOverlay
├── SetSlotData(), SetDragOverlay(), GetSlotData()
└── OnSlotClicked (virtual), OnSlotDragStart (virtual)

WBP_ItemContainerBase (common container)
├── ContainerBackground, TitleText, SlotsContainer
├── InitializeContainer(), RefreshAllSlots()
└── HandleItemDrop(), FindEmptySlot()

LOOT (new)
WBP_LootSlot extends WBP_ItemSlotBase
WBP_LootWindow extends WBP_ItemContainerBase
BP_LootableBase (Actor, implements BPI_Interactable)

INVENTORY (modified)
WBP_InventorySlot - Add drag support
WBP_InventoryGrid - Add drop handling

DRAG SYSTEM
WBP_DraggedItem (follows cursor, shows item being dragged)
```

---

## Development Phases (TEST-FIRST ORDER)

### Phase 1: Loot Window Skeleton
**Goal:** User sees a loot window with hardcoded items, can visually verify layout.

- Create base widget classes (WBP_ItemSlotBase, WBP_ItemContainerBase)
- Create WBP_LootSlot with hardcoded item display
- Create WBP_LootWindow with grid of slots, close button
- **USER TEST:** Add loot window to viewport, verify it looks correct, close button works

### Phase 2: Drag Widget
**Goal:** User can see a widget follow their cursor.

- Create WBP_DraggedItem widget
- Test by spawning and having it follow mouse
- **USER TEST:** Drag widget follows cursor smoothly

### Phase 3: Inventory Drag Support
**Goal:** User can drag from existing inventory, see grey overlay.

- Add DragOverlay to WBP_InventorySlot
- Add StartDrag/CancelDrag to WBP_InventoryGrid
- Connect left-click to start drag
- **USER TEST:** Click inventory slot → grey overlay appears, drag widget follows cursor

### Phase 4: Lootable Actor + Data Connection
**Goal:** Real loot containers with actual item data.

- Create BPI_Interactable interface
- Create BP_LootableBase actor with LootSlots array
- Connect loot window to lootable data
- Implement item transfer (loot → inventory)
- **USER TEST:** Place chest in level, open loot window, drag item to inventory

### Phase 5: E-Key Integration
**Goal:** Complete flow - E key works for both NPCs and loot.

- Modify BP_ThirdPersonCharacter E-key handler
- Add BPI_Interactable to BP_DialogueNPC
- Add OpenLootWindow/CloseLootWindow to player
- Side-by-side positioning
- **USER TEST:** E near NPC = dialogue, E near chest = loot window

---

## Phase Documents

| Phase | Document | Focus |
|-------|----------|-------|
| 1 | [Phase1_LootWindowSkeleton.md](Phase1_LootWindowSkeleton.md) | Visual widgets with hardcoded data |
| 2 | [Phase2_DragWidget.md](Phase2_DragWidget.md) | Cursor-following drag visual |
| 3 | [Phase3_InventoryDragSupport.md](Phase3_InventoryDragSupport.md) | Drag overlay, start/cancel drag |
| 4 | [Phase4_LootableActorAndData.md](Phase4_LootableActorAndData.md) | Actor, interface, real data |
| 5 | [Phase5_EKeyIntegration.md](Phase5_EKeyIntegration.md) | E-key handler, final integration |

---

## Asset Structure

```
/Game/Blueprints/
└── Interfaces/
    └── BPI_Interactable.uasset         (Blueprint Interface)

/Game/Inventory/
├── Data/                               (existing - shared with loot)
│   ├── E_ItemType.uasset
│   ├── S_ItemDefinition.uasset
│   ├── S_InventorySlot.uasset
│   └── DT_Items.uasset
├── Blueprints/
│   └── BP_InventoryManager.uasset      (existing)
└── UI/
    ├── Base/                           (NEW)
    │   ├── WBP_ItemSlotBase.uasset
    │   └── WBP_ItemContainerBase.uasset
    ├── WBP_InventoryGrid.uasset        (modified for drag)
    ├── WBP_InventorySlot.uasset        (modified for drag)
    ├── WBP_ItemContextMenu.uasset      (existing)
    ├── WBP_ItemTooltip.uasset          (existing)
    └── WBP_DraggedItem.uasset          (NEW)

/Game/Loot/                             (NEW)
├── Blueprints/
│   └── BP_LootableBase.uasset          (Actor, implements BPI_Interactable)
└── UI/
    ├── WBP_LootSlot.uasset             (extends WBP_ItemSlotBase)
    └── WBP_LootWindow.uasset           (extends WBP_ItemContainerBase)
```

---

## Success Criteria

- [ ] E key opens dialogue for NPCs, loot window for lootables
- [ ] Inventory and loot windows appear side-by-side
- [ ] Left-click and drag from loot slot shows grey overlay
- [ ] Dragged item visual follows cursor
- [ ] Drop on inventory transfers full stack
- [ ] Source slot clears, target slot shows item
- [ ] Both containers refresh after transfer
- [ ] Close button on loot window works
- [ ] Empty loot containers can optionally auto-close

---

## Related Systems

- [Inventory System](../inventory-plan/00_Overview.md) - Core inventory functionality
- Dialogue System - Shares E-key interaction via BPI_Interactable
