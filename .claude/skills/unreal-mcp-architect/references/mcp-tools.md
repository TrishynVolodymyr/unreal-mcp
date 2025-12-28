# MCP Tools Reference

Current tool inventory. This list evolves as tools are added, removed, or consolidated.

## editorMCP — Level Actor Manipulation

| Tool | Purpose |
|------|---------|
| `spawn_actor` | Spawn actor in level |
| `delete_actor` | Delete actor from level |
| `set_actor_transform` | Set actor position/rotation/scale |
| `get_actor_properties` | Get actor property values |
| `set_actor_property` | Set actor property |
| `set_light_property` | Set light-specific properties |
| `spawn_blueprint_actor` | Spawn BP actor in level |
| `get_level_metadata` | Get level information |

## blueprintMCP — Blueprint Structure

| Tool | Purpose |
|------|---------|
| `create_blueprint` | Create new blueprint |
| `compile_blueprint` | Compile blueprint |
| `add_blueprint_variable` | Add variable to BP |
| `add_component_to_blueprint` | Add component |
| `modify_blueprint_component_properties` | Modify component props |
| `create_custom_blueprint_function` | Create custom function |
| `set_blueprint_variable_value` | Set variable default |
| `get_blueprint_metadata` | Get BP structure info |
| `add_interface_to_blueprint` | Add interface |
| `create_blueprint_interface` | Create new interface |

## umgMCP — UMG Widgets

| Tool | Purpose |
|------|---------|
| `create_umg_widget_blueprint` | Create widget BP |
| `bind_widget_component_event` | Bind widget event |
| `set_text_block_widget_component_binding` | Bind text block |
| `add_child_widget_component_to_parent` | Add child to parent |
| `create_parent_and_child_widget_components` | Create hierarchy |
| `set_widget_component_placement` | Set widget placement |
| `add_widget_component_to_widget` | Add component |
| `set_widget_component_property` | Set widget property |
| `get_widget_blueprint_metadata` | Get widget BP info |
| `capture_widget_screenshot` | Screenshot widget |
| `create_widget_input_handler` | Create input handler |
| `remove_widget_function_graph` | Remove function |

## datatableMCP — DataTable Operations

| Tool | Purpose |
|------|---------|
| `create_datatable` | Create datatable |
| `get_datatable_rows` | Get row data |
| `get_datatable_row_names` | Get row names |
| `add_rows_to_datatable` | Add rows |
| `update_rows_in_datatable` | Update rows |
| `delete_datatable_rows` | Delete rows |

## nodeMCP — Blueprint Node Graph

| Tool | Purpose |
|------|---------|
| `connect_blueprint_nodes` | Connect nodes |
| `get_variable_info` | Get variable info |
| `disconnect_node` | Disconnect node |
| `delete_node` | Delete node |
| `replace_node` | Replace node |
| `set_node_pin_value` | Set pin value |
| `cleanup_blueprint_graph` | Cleanup graph |
| `auto_arrange_nodes` | Arrange nodes |

## projectMCP — Project Assets

| Tool | Purpose |
|------|---------|
| `create_input_mapping` | Create input mapping |
| `create_enhanced_input_action` | Create input action |
| `create_input_mapping_context` | Create mapping context |
| `add_mapping_to_context` | Add mapping to context |
| `create_folder` | Create content folder |
| `create_struct` | Create struct |
| `update_struct` | Update struct |
| `create_enum` | Create enum |
| `get_project_metadata` | Get project info |
| `get_struct_pin_names` | Get struct pins |
| `duplicate_asset` | Duplicate asset |

## blueprintActionMCP — Node Discovery & Creation

| Tool | Purpose |
|------|---------|
| `get_actions_for_pin` | Get actions for pin type |
| `get_actions_for_class` | Get class actions |
| `get_actions_for_class_hierarchy` | Get hierarchy actions |
| `search_blueprint_actions` | Search available actions |
| `inspect_node_pin_connection` | Inspect pin connection |
| `create_node_by_action_name` | Create node by action |
| `find_in_blueprints` | Search in blueprints |
