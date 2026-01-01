---
name: unreal-mcp-debugger
description: >
  Systematic debugger for Unreal Engine gameplay logic using MCP inspection tools.
  ALWAYS trigger on: Blueprint not working as expected, user reports bugs or broken features,
  "nothing happens", wrong variable values, events not firing, DataTable lookup failures,
  widget behavior issues, or phrases like "doesn't work", "broken", "bug", "wrong",
  "not working", "fix", "debug", "why isn't", "investigate".
  Uses get_blueprint_metadata, get_datatable_rows, find_in_blueprints for structured investigation.
---

# Unreal MCP Debugger

Systematic approach to finding and fixing gameplay logic bugs using MCP inspection tools.

## Debugging Workflow

### Step 1: Reproduce & Scope

Ask user:
- What SHOULD happen?
- What ACTUALLY happens?
- Which Blueprint/Widget/DataTable is involved?

### Step 2: Investigate with Tools

Follow this diagnostic order:

```
1. Check orphaned nodes (disconnected logic)
2. Check execution flow (is logic reachable?)
3. Check pin connections (are values passed correctly?)
4. Check variable values (are defaults correct?)
5. Check DataTable content (is data present?)
6. Check cross-blueprint usage (find_in_blueprints)
```

### Step 3: Report Findings

Before fixing, explain:
- What the bug is
- Why it causes the problem
- Proposed fix

### Step 4: Fix ONE Thing

Make single targeted fix, then confirm with user.

---

## MCP Debugging Tools Reference

### Blueprint Inspection

| Command | Usage | When to Use |
|---------|-------|-------------|
| `get_blueprint_metadata` | `fields=["orphaned_nodes"]` | First check — find disconnected logic |
| `get_blueprint_metadata` | `fields=["graph_nodes"], detail_level="full"` | See ALL node pins, values, connections |
| `get_blueprint_metadata` | `fields=["graph_nodes"], detail_level="flow"` | Trace execution path only |
| `get_blueprint_metadata` | `fields=["variables"]` | Check variable types and defaults |
| `get_blueprint_metadata` | `fields=["components"]` | Verify component setup |

### Variable & Pin Inspection

| Command | Usage | When to Use |
|---------|-------|-------------|
| `get_variable_info` | `blueprint_name, variable_name` | Detailed variable type info |
| `inspect_node_pin_connection` | `node_name, pin_name` | Pin type and connection details |

### DataTable Inspection

| Command | Usage | When to Use |
|---------|-------|-------------|
| `get_datatable_row_names` | `datatable_path` | List rows + field names |
| `get_datatable_rows` | `datatable_path, row_names` | Actual row data |

### Widget Inspection

| Command | Usage | When to Use |
|---------|-------|-------------|
| `get_widget_blueprint_metadata` | `fields=["layout", "components"]` | Widget hierarchy and bindings |

### Cross-Blueprint Search

| Command | Usage | When to Use |
|---------|-------|-------------|
| `find_in_blueprints` | `search_query, search_type` | Find function/variable usage across project |

### Level Inspection

| Command | Usage | When to Use |
|---------|-------|-------------|
| `get_level_metadata` | `actor_filter="*Pattern*"` | Find actors in level |

---

## Common Bug Patterns

### "Nothing Happens"
1. Check orphaned nodes — logic may be disconnected
2. Check execution flow — is the function called?
3. Check event bindings — is event actually bound?

### "Wrong Value"
1. Get full node details — check pin connections
2. Check variable defaults
3. Trace data flow backward from problem node

### "DataTable Lookup Fails"
1. Get row names — verify row exists
2. Get row data — check field values
3. Check row name spelling (case-sensitive)

### "Widget Not Updating"
1. Check widget bindings
2. Find where update function is called
3. Check visibility/collapse state

### "Function Not Found"
1. Use find_in_blueprints to locate usage
2. Check if function exists in expected Blueprint
3. Verify function is public if called externally

---

## Anti-Patterns

- Guessing without inspecting first
- Fixing multiple things at once
- Not explaining root cause to user
- Skipping orphaned node check
