# Command Coverage Audit: Legacy vs New Architecture

## Legacy Command Handlers Analysis

### Blueprint Commands (FUnrealMCPBlueprintCommands)
**Legacy Handle Methods:**
1. `HandleCreateBlueprint`
2. `HandleAddComponentToBlueprint`
3. `HandleSetComponentProperty`
4. `HandleSetPhysicsProperties`
5. `HandleCompileBlueprint`
6. `HandleSpawnBlueprintActor`
7. `HandleSetBlueprintProperty`
8. `HandleSetStaticMeshProperties`
9. `HandleSetPawnProperties`
10. `HandleCallFunctionByName`
11. `HandleAddInterfaceToBlueprint`
12. `HandleCreateBlueprintInterface`
13. `HandleListBlueprintComponents`
14. `HandleCreateCustomBlueprintFunction`

### UMG Commands (FUnrealMCPUMGCommands)
**Legacy Handle Methods:**
1. `HandleCreateUMGWidgetBlueprint`
2. `HandleBindWidgetComponentEvent`
3. `HandleSetTextBlockBinding`
4. `HandleAddWidgetSwitcherToWidget`
5. `HandleAddThrobberToWidget`
6. `HandleAddExpandableAreaToWidget`
7. `HandleAddMenuAnchorToWidget`
8. `HandleAddRichTextBlockToWidget`
9. `HandleAddSafeZoneToWidget`
10. `HandleAddInvalidationBoxToWidget`
11. `HandleAddInputKeySelectorToWidget`
12. `HandleAddMultiLineEditableTextToWidget`
13. `HandleAddSizeBoxToWidget`
14. `HandleGetWidgetContainerDimensions`
15. `HandleAddImageToWidget`
16. `HandleAddCheckBoxToWidget`
17. `HandleAddSliderToWidget`
18. `HandleAddProgressBarToWidget`
19. `HandleAddBorderToWidget`
20. `HandleAddScrollBoxToWidget`
21. `HandleAddSpacerToWidget`
22. `HandleCheckComponentExists`
23. `HandleSetWidgetPlacement`
24. `HandleAddWidgetAsChild`
25. `HandleCreateWidgetComponentWithChild`
26. `HandleAddWidgetComponent`
27. `HandleSetWidgetComponentProperty`
28. `HandleGetWidgetComponentLayout`

### Editor Commands (FUnrealMCPEditorCommands)
**Legacy Handle Methods:**
1. `HandleGetActorsInLevel`
2. `HandleFindActorsByName`
3. `HandleSpawnActor`
4. `HandleDeleteActor`
5. `HandleSetActorTransform`
6. `HandleGetActorProperties`
7. `HandleSetActorProperty`
8. `HandleSetLightProperty`
9. `HandleSpawnBlueprintActor`
10. `HandleFocusViewport`
11. `HandleTakeScreenshot`
12. `HandleFindAssetsByType`
13. `HandleFindAssetsByName`
14. `HandleFindWidgetBlueprints`
15. `HandleFindBlueprints`
16. `HandleFindDataTables`

### DataTable Commands (FUnrealMCPDataTableCommands)
**Legacy Handle Methods:**
1. `HandleCreateDataTable`
2. `HandleAddRowsToDataTable`
3. `HandleGetDataTableRows`
4. `HandleDeleteDataTableRows`
5. `HandleGetDataTablePropertyMap`
6. `HandleGetDataTableRowNames`
7. `HandleUpdateRowsInDataTable`

### Project Commands (FUnrealMCPProjectCommands)
**Legacy Handle Methods:**
1. `HandleUpdateStruct`
2. `HandleShowStructVariables`
3. `HandleListFolderContents`
4. `HandleCreateEnhancedInputAction`
5. `HandleCreateInputMappingContext`
6. `HandleAddMappingToContext`
7. `HandleListInputActions`
8. `HandleListInputMappingContexts`

### Blueprint Node Commands (FUnrealMCPBlueprintNodeCommands)
**Legacy Handle Methods:**
1. `HandleConnectBlueprintNodes`
2. `HandleAddBlueprintEvent`
3. `HandleAddBlueprintFunctionCall`
4. `HandleAddBlueprintVariable`
5. `HandleAddBlueprintInputActionNode`
6. `HandleFindBlueprintNodes`
7. `HandleAddBlueprintCustomEventNode`
8. `HandleGetVariableInfo`
9. `HandleAddEnhancedInputActionNode`

## New Architecture Command Registration Analysis

### Currently Registered Commands

#### Blueprint Commands (BlueprintCommandRegistration.cpp)
**Registered Commands:**
1. ✅ `create_blueprint` (FCreateBlueprintCommand)
2. ✅ `add_component_to_blueprint` (FAddComponentToBlueprintCommand)
3. ✅ `set_component_property` (FSetComponentPropertyCommand)
4. ✅ `compile_blueprint` (FCompileBlueprintCommand)
5. ✅ `set_physics_properties` (FSetPhysicsPropertiesCommand)
6. ✅ `set_blueprint_property` (FSetBlueprintPropertyCommand)
7. ✅ `list_blueprint_components` (FListBlueprintComponentsCommand)
8. ✅ `set_static_mesh_properties` (FSetStaticMeshPropertiesCommand)
9. ✅ `set_pawn_properties` (FSetPawnPropertiesCommand)

#### DataTable Commands (DataTableCommandRegistration.cpp)
**Registered Commands:**
1. ✅ `create_datatable` (FCreateDataTableCommand)
2. ✅ `add_rows_to_datatable` (FAddRowsToDataTableCommand)
3. ✅ `get_datatable_rows` (FGetDataTableRowsCommand)
4. ✅ `update_rows_in_datatable` (FUpdateRowsInDataTableCommand)
5. ✅ `delete_datatable_rows` (FDeleteDataTableRowsCommand)
6. ✅ `get_datatable_row_names` (FGetDataTableRowNamesCommand)
7. ✅ `get_datatable_property_map` (FGetDataTablePropertyMapCommand)

#### Editor Commands (EditorCommandRegistration.cpp)
**Registered Commands:**
1. ✅ `get_actors_in_level` (FGetActorsInLevelCommand)
2. ✅ `find_actors_by_name` (FFindActorsByNameCommand)
3. ✅ `spawn_actor` (FSpawnActorCommand)
4. ✅ `delete_actor` (FDeleteActorCommand)
5. ✅ `spawn_blueprint_actor` (FSpawnBlueprintActorCommand)

#### Project Commands (ProjectCommandRegistration.cpp)
**Registered Commands:**
1. ✅ `create_input_mapping` (FCreateInputMappingCommand)
2. ✅ `create_folder` (FCreateFolderCommand)
3. ✅ `create_struct` (FCreateStructCommand)
4. ✅ `get_project_dir` (FGetProjectDirCommand)

## Missing Commands Analysis

### Blueprint Commands - MISSING:
- ❌ `spawn_blueprint_actor` (handled by Editor commands)
- ❌ `call_blueprint_function` (HandleCallFunctionByName)
- ❌ `add_interface_to_blueprint` (HandleAddInterfaceToBlueprint)
- ❌ `create_blueprint_interface` (HandleCreateBlueprintInterface)
- ❌ `create_custom_blueprint_function` (HandleCreateCustomBlueprintFunction)

### UMG Commands - MISSING ALL 28 COMMANDS:
- ❌ `create_umg_widget_blueprint`
- ❌ `bind_widget_component_event`
- ❌ `set_text_block_widget_component_binding`
- ❌ `add_widget_switcher_to_widget`
- ❌ `add_throbber_to_widget`
- ❌ `add_expandable_area_to_widget`
- ❌ `add_menu_anchor_to_widget`
- ❌ `add_rich_text_block_to_widget`
- ❌ `add_safe_zone_to_widget`
- ❌ `add_invalidation_box_to_widget`
- ❌ `add_input_key_selector_to_widget`
- ❌ `add_multi_line_editable_text_to_widget`
- ❌ `add_size_box_to_widget`
- ❌ `get_widget_container_component_dimensions`
- ❌ `add_image_to_widget`
- ❌ `add_checkbox_to_widget`
- ❌ `add_slider_to_widget`
- ❌ `add_progress_bar_to_widget`
- ❌ `add_border_to_widget`
- ❌ `add_scroll_box_to_widget`
- ❌ `add_spacer_to_widget`
- ❌ `check_widget_component_exists`
- ❌ `set_widget_component_placement`
- ❌ `add_child_widget_component_to_parent`
- ❌ `create_parent_and_child_widget_components`
- ❌ `add_widget_component_to_widget`
- ❌ `set_widget_component_property`
- ❌ `get_widget_component_layout`

### Editor Commands - MISSING:
- ❌ `set_actor_transform`
- ❌ `get_actor_properties`
- ❌ `set_actor_property`
- ❌ `set_light_property`
- ❌ `focus_viewport`
- ❌ `take_screenshot`
- ❌ `find_assets_by_type`
- ❌ `find_assets_by_name`
- ❌ `find_widget_blueprints`
- ❌ `find_blueprints`
- ❌ `find_data_tables`

### Project Commands - MISSING:
- ❌ `update_struct`
- ❌ `show_struct_variables`
- ❌ `list_folder_contents`
- ❌ `create_enhanced_input_action`
- ❌ `create_input_mapping_context`
- ❌ `add_mapping_to_context`
- ❌ `list_input_actions`
- ❌ `list_input_mapping_contexts`

### Blueprint Node Commands - MISSING ALL 9 COMMANDS:
- ❌ `connect_blueprint_nodes`
- ❌ `add_blueprint_input_action_node`
- ❌ `find_blueprint_nodes`
- ❌ `add_blueprint_variable`
- ❌ `get_variable_info`
- ❌ All other Blueprint node commands

## Summary

**Total Legacy Commands:** 76
**Total Registered Commands:** 25
**Missing Commands:** 51

**Coverage by Category:**
- Blueprint Commands: 9/14 (64% coverage)
- UMG Commands: 0/28 (0% coverage) ⚠️ CRITICAL
- Editor Commands: 5/16 (31% coverage)
- DataTable Commands: 7/7 (100% coverage) ✅
- Project Commands: 4/8 (50% coverage)
- Blueprint Node Commands: 0/9 (0% coverage) ⚠️ CRITICAL

## Priority Actions Required

1. **CRITICAL:** Implement UMG command registration (28 missing commands)
2. **CRITICAL:** Implement Blueprint Node command registration (9 missing commands)
3. **HIGH:** Complete Editor command registration (11 missing commands)
4. **HIGH:** Complete Blueprint command registration (5 missing commands)
5. **MEDIUM:** Complete Project command registration (4 missing commands)