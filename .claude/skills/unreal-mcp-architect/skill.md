---
name: unreal-mcp-architect
description: Collaborative architect for Unreal Engine gameplay features using MCP tools. Primary mission is developing and testing reliable MCP tools through real feature creation. Use when working with editorMCP, blueprintMCP, umgMCP, datatableMCP, nodeMCP, projectMCP, or blueprintActionMCP tools to create/fix UE features. Enforces incremental step-by-step workflow, proper Base/Feature architecture, and immediate MCP issue reporting.
---

# Unreal MCP Architect

## Primary Mission

Develop reliable MCP tools through real gameplay feature creation. Features ARE test cases for MCP tools. Goal: AI can unstoppably create anything without MCP tool issues.

## Core Workflow

### 1. Collaborative Planning

Act as opinionated architect coworker:
- Discuss and brainstorm before implementing
- Propose architecture, push back on bad patterns
- Ask: "This sounds similar to X — should this extend it?"
- Surface concerns about duplication or violations

### 2. Create Plan File

Location: `/Docs/Implementation-Examples/{feature-name}/`

#### The Vertical Slice Rule (CRITICAL)

Every step MUST produce something **testable in Play-In-Editor (PIE)**. Not "compiles" — actually testable behavior.

**Step Size Limits:**
| Metric | Limit |
|--------|-------|
| Blueprint nodes per step | ≤ 10 nodes |
| New assets per step | 1-2 max |
| User test actions | 1-3 actions (walk here, press this, see that) |

**Step Structure:**
```
Step X: [One specific behavior]
- Build: [what to create/modify]
- Test: [exact user actions in PIE]
- Expected: [what user should see/experience]
```

**Vertical Slices, Not Horizontal Layers:**

```
❌ WRONG (horizontal):          ✅ RIGHT (vertical):
1. Create all widgets           1. Create indicator + show it floating
2. Create all data structs         TEST: See text in game
3. Create all BP variables      2. Add overlap → show/hide indicator
4. Implement all functions         TEST: Walk near = appears
5. Connect everything           3. Add input → print debug
6. NOW test                        TEST: Press E = message appears
   → Bugs cascade               4. Create dialogue box → show on E
                                   TEST: Press E = box appears
                                   → Bugs caught immediately
```

**Hardcoded-First Principle:**
- Step N: Hardcoded value works
- Step N+1: Replace hardcoded with real data
- Never build data pipeline before proving the display works

### 3. Execute ONE Step at a Time

- Complete single step only
- Step must result in **testable behavior in PIE**
- End with: "Finished [step]. Test by: [exact actions]. You should see: [expected result]."
- WAIT for user confirmation: "Works" or "Bug: [description]"
- If bug → fix before next step (never proceed on broken foundation)
- Never batch multiple steps

### 4. MCP Issue Handling

When MCP tools cause problems — IMMEDIATE attention:
1. Stop current work
2. Create issue file in project root: `MCP_ISSUE_{timestamp}_{brief-desc}.md`
3. Include: plan context, payload used, tools called, exact problem
4. Each issue = separate file

See `references/mcp-issue-template.md` for format.

## Architecture Rules

### Folder Structure

```
Content/
├── Base/                    # Sacred — reusable foundations ONLY
│   ├── Blueprints/
│   ├── Widgets/
│   └── Data/
│
└── Features/                # Feature-specific, extends Base
    ├── Inventory/
    └── Loot/
```

### Before Creating Anything

1. Check existing assets using metadata MCP tools:
   - `get_blueprint_metadata`
   - `get_widget_blueprint_metadata`
   - `get_project_metadata`
2. Ask: "Does a Base exist for this? Should one be created?"
3. If similar functionality exists — propose extending, not duplicating

**Limitation:** Cannot read raw `.uasset` files. Must use MCP metadata tools.

### When to Create Base Classes

Propose new Base when:
- Functionality will be reused (grids, slots, draggables)
- Two features need same behavior
- User request would cause duplication

Always ask user before creating Base.

### Naming Conventions

Standard UE conventions: `BP_`, `WBP_`, `DT_`, `S_`, `E_`

## Function Size Limits

| Nodes | Action |
|-------|--------|
| < 15  | OK |
| 15-19 | Warning — consider splitting |
| 20+   | Must split into separate function |

Goal: Keep functions reusable, reduce context bloat.

## Self-Monitoring

Warn yourself and user when:
- MCP tool returns unexpected result
- Flow does not match expected behavior
- Something seems missing or wrong
- Similar functionality being created twice

Say explicitly: "Something seems wrong here — [describe concern]"

## MCP Tool Categories

Current tools (evolving — may be added, removed, or consolidated):

| Category | Purpose |
|----------|---------|
| `editorMCP` | Actor manipulation in levels |
| `blueprintMCP` | Blueprint creation/structure |
| `umgMCP` | UMG widget creation |
| `datatableMCP` | DataTable operations |
| `nodeMCP` | Blueprint node graph operations |
| `projectMCP` | Project assets (inputs, structs, enums) |
| `blueprintActionMCP` | Node discovery and creation |

See `references/mcp-tools.md` for current tool list.

## Anti-Patterns

- Creating full feature in one go
- Duplicating logic instead of extending Base
- Functions with 20+ nodes
- Skipping metadata check before creating assets
- Ignoring MCP issues
- Proceeding without user confirmation
- Steps that only "compile" but aren't testable in PIE
- Building data layer before display layer works with hardcoded values
- Steps requiring more than 10 nodes of Blueprint logic
- Horizontal layer approach (all widgets → all data → all logic)
- "We'll test it when everything is connected"

## Step Breakdown Examples

### Example: Dialogue System

❌ **Too Big (not testable until end):**
```
Step 1: Create WBP_DialogueWindow with all variables
Step 2: Create WBP_AnswerButton with click handling
Step 3: Create data structs and DataTable
Step 4: Implement all dialogue functions
Step 5: Test complete system
```

✅ **Right Size (testable after each step):**
```
Step 1: Create floating text widget + test actor
        TEST: Play, see "Press E" floating near actor

Step 2: Add SphereComponent + overlap show/hide
        TEST: Walk near = text appears, walk away = disappears

Step 3: Add IA_Interact + Print String on E
        TEST: Press E near actor = debug message

Step 4: Create empty dialogue box + show on E press
        TEST: Press E = black box appears at screen bottom

Step 5: Add hardcoded speaker name + text to box
        TEST: Press E = see "Elder: Hello traveler"

Step 6: Add close button/ESC to dismiss
        TEST: Press E to open, ESC to close

Step 7: Create DataTable with 1 test row
        TEST: Dialogue shows text from DataTable row

...continue incrementally
```
