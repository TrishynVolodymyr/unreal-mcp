# MCP Tool Consolidation Analysis

Focus: Query/metadata tools that retrieve information (not setters/creators)

---

## COMPLETED CONSOLIDATIONS

### ✅ UMG_TOOLS - `get_widget_blueprint_metadata` (IMPLEMENTED)

**Status: COMPLETE**

Created consolidated tool: `get_widget_blueprint_metadata`
- Supports fields: `components`, `layout`, `dimensions`, `hierarchy`, `bindings`, `events`, `variables`, `functions`, `*`

**Removed tools:**
- ~~`check_widget_component_exists`~~ → Use `fields=["components"]` then check component name
- ~~`get_widget_container_component_dimensions`~~ → Use `fields=["dimensions"]`
- ~~`get_widget_component_layout`~~ → Use `fields=["layout"]`

---

## CONSOLIDATION OPPORTUNITIES BY MCP SERVER

### 1. BLUEPRINT_TOOLS (blueprint_mcp_server.py)

| Tool | Type | Recommendation |
|------|------|----------------|
| `get_blueprint_metadata` | **QUERY** | ✅ KEEP - Central metadata tool |
| `list_blueprint_components` | **QUERY** | ❌ **REMOVE** - Already in `get_blueprint_metadata` with `fields=["components"]` |
| `create_blueprint` | setter | keep |
| `add_blueprint_variable` | setter | keep |
| `add_component_to_blueprint` | setter | keep |
| `set_static_mesh_properties` | setter | keep |
| `set_component_property` | setter | keep |
| `set_physics_properties` | setter | keep |
| `compile_blueprint` | setter | keep |
| `set_blueprint_property` | setter | keep |
| `set_pawn_properties` | setter | keep |
| `call_blueprint_function` | setter | keep |
| `add_interface_to_blueprint` | setter | keep |
| `create_blueprint_interface` | setter | keep |
| `create_custom_blueprint_function` | setter | keep |

**Action Items:**
1. **Remove `list_blueprint_components`** - 100% redundant with `get_blueprint_metadata(fields=["components"])`

---

### 2. NODE_TOOLS (node_mcp_server.py)

| Tool | Type | Recommendation |
|------|------|----------------|
| `find_blueprint_nodes` | **QUERY** | ⚠️ Consider merge → `get_blueprint_metadata(fields=["nodes"])` |
| `get_variable_info` | **QUERY** | ⚠️ Consider merge → Enhanced `variables` field with detail param |
| `connect_blueprint_nodes` | setter | keep |
| `disconnect_node` | setter | keep |
| `delete_node` | setter | keep |
| `replace_node` | setter | keep |
| `set_node_pin_value` | setter | keep |

**Action Items:**
1. **Consider adding `nodes` field to `get_blueprint_metadata`** - Would replace `find_blueprint_nodes`
2. **Consider enhancing `variables` field** - Could include detailed type info from `get_variable_info`

**Trade-offs:**
- `find_blueprint_nodes` has complex filtering (node_type, event_type) - may be worth keeping separate
- `get_variable_info` is simple enough to merge into `variables` field

---

### 3. BLUEPRINT_ACTION_TOOLS (blueprint_action_mcp_server.py)

| Tool | Type | Recommendation |
|------|------|----------------|
| `get_actions_for_pin` | **QUERY** | ⚠️ Consider merge |
| `get_actions_for_class` | **QUERY** | ⚠️ Consider merge |
| `get_actions_for_class_hierarchy` | **QUERY** | ⚠️ Consider merge |
| `search_blueprint_actions` | **QUERY** | ⚠️ Consider merge |
| `inspect_node_pin_connection` | **QUERY** | ✅ KEEP - Specific use case |
| `create_node_by_action_name` | setter | keep |

**Analysis:**
These 4 query tools could potentially consolidate into `search_blueprint_actions` with extra parameters:
```python
search_blueprint_actions(
    keyword: str = None,           # Current search
    pin_type: str = None,          # Replaces get_actions_for_pin
    class_name: str = None,        # Replaces get_actions_for_class
    include_parent_classes: bool = False  # Replaces get_actions_for_class_hierarchy
)
```

**Trade-offs:**
- These are discovery tools for node creation workflow
- Keeping them separate provides clearer intent
- **Recommendation: KEEP SEPARATE** - They serve distinct UX purposes during node discovery

---

### 4. EDITOR_TOOLS (editor_mcp_server.py)

| Tool | Type | Recommendation |
|------|------|----------------|
| `get_actors_in_level` | **QUERY** | ⚠️ Consider merge → `get_level_metadata(fields=["actors"])` |
| `find_actors_by_name` | **QUERY** | ⚠️ Consider merge → `get_level_metadata(filter=...)` |
| `get_actor_properties` | **QUERY** | ✅ KEEP - Actor-specific, needs actor name |
| `spawn_actor` | setter | keep |
| `delete_actor` | setter | keep |
| `set_actor_transform` | setter | keep |
| `set_actor_property` | setter | keep |
| `set_light_property` | setter | keep |
| `focus_viewport` | setter | keep (buggy) |
| `spawn_blueprint_actor` | setter | keep |

**Action Items:**
1. **Create `get_level_metadata`** - Consolidate `get_actors_in_level` + `find_actors_by_name`
   ```python
   get_level_metadata(
       fields: List[str] = ["actors"],  # Future: "world_settings", "lighting", etc.
       actor_filter: str = None          # Pattern for name matching
   )
   ```

**Trade-offs:**
- Minor consolidation (2 → 1 tool)
- Worth doing for API consistency

---

### 5. PROJECT_TOOLS (project_mcp_server.py)

| Tool | Type | Recommendation |
|------|------|----------------|
| `list_input_actions` | **QUERY** | ⚠️ Consider merge |
| `list_input_mapping_contexts` | **QUERY** | ⚠️ Consider merge |
| `show_struct_variables` | **QUERY** | ⚠️ Consider merge |
| `list_folder_contents` | **QUERY** | ⚠️ Consider merge |
| `create_input_mapping` | setter | keep |
| `create_enhanced_input_action` | setter | keep |
| `create_input_mapping_context` | setter | keep |
| `add_mapping_to_context` | setter | keep |
| `create_folder` | setter | keep |
| `create_struct` | setter | keep |
| `update_struct` | setter | keep |

**Action Items:**
1. **Create `get_project_metadata`** - Consolidates 4 query tools
   ```python
   get_project_metadata(
       fields: List[str] = ["*"],  # "input_actions", "input_contexts", "structs", "folders"
       folder_path: str = "/Game",  # For folder listing
       struct_name: str = None      # For specific struct info
   )
   ```

**Trade-offs:**
- Good consolidation (4 → 1 tool)
- Different query targets but all are "project-level" metadata

---

### 6. DATATABLE_TOOLS (datatable_mcp_server.py)

| Tool | Type | Recommendation |
|------|------|----------------|
| `get_datatable_rows` | **QUERY** | ✅ KEEP - Core query |
| `get_datatable_row_names` | **QUERY** | ⚠️ Consider merge → `get_datatable_rows(names_only=true)` |
| `create_datatable` | setter | keep |
| `add_rows_to_datatable` | setter | keep |
| `update_rows_in_datatable` | setter | keep |
| `delete_datatable_rows` | setter | keep |

**Action Items:**
1. **Add `names_only` parameter to `get_datatable_rows`** - Eliminates `get_datatable_row_names`
   ```python
   get_datatable_rows(
       datatable_name: str,
       row_names: List[str] = None,
       names_only: bool = False  # NEW: Only return row names, not full data
   )
   ```

---

## PRIORITY ORDER

### 🔴 High Priority (Clear wins, minimal risk)

1. **Remove `list_blueprint_components`** from BLUEPRINT_TOOLS
   - 100% redundant with `get_blueprint_metadata(fields=["components"])`
   - Risk: Low - just deprecate and remove
   - Effort: ~30 min

### 🟡 Medium Priority (Good consolidation, moderate effort)

2. **Create `get_project_metadata`** in PROJECT_TOOLS
   - Consolidates 4 query tools
   - Risk: Low - new tool, deprecate old ones
   - Effort: ~2-3 hours

3. **Add `names_only` to `get_datatable_rows`** in DATATABLE_TOOLS
   - Eliminates `get_datatable_row_names`
   - Risk: Very low
   - Effort: ~1 hour

4. **Create `get_level_metadata`** in EDITOR_TOOLS
   - Consolidates 2 query tools
   - Risk: Low
   - Effort: ~2 hours

### 🟢 Lower Priority (Consider but not essential)

5. **Add `nodes` field to `get_blueprint_metadata`** in BLUEPRINT_TOOLS
   - Potentially replaces `find_blueprint_nodes` from NODE_TOOLS
   - Risk: Medium - cross-server change
   - Effort: ~3-4 hours

6. **Enhance `variables` field in `get_blueprint_metadata`**
   - Could replace `get_variable_info` from NODE_TOOLS
   - Risk: Medium - need to maintain backwards compatibility
   - Effort: ~2 hours

---

## SUMMARY TABLE

| Server | Current Query Tools | After Consolidation | Tools Removed |
|--------|---------------------|---------------------|---------------|
| UMG_TOOLS | 1 | 1 | 3 (DONE ✅) |
| BLUEPRINT_TOOLS | 2 | 1 | 1 |
| NODE_TOOLS | 2 | 0-2 | 0-2 (optional) |
| BLUEPRINT_ACTION_TOOLS | 5 | 5 | 0 (keep separate) |
| EDITOR_TOOLS | 3 | 2 | 1 |
| PROJECT_TOOLS | 4 | 1 | 3 |
| DATATABLE_TOOLS | 2 | 1 | 1 |
| **TOTAL** | **19** | **11-13** | **9-11** |

---

## NEXT STEPS

1. Start with **High Priority** items - quick wins
2. Then tackle **Medium Priority** for significant API cleanup
3. **Lower Priority** can wait for future refactoring cycles
