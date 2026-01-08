"""
Blueprint Tools for Unreal MCP.

This module provides tools for creating and manipulating Blueprint assets in Unreal Engine.
"""

import logging
from typing import Dict, List, Any, Union
from mcp.server.fastmcp import FastMCP, Context
from utils.blueprints.blueprint_operations import (
    create_blueprint as create_blueprint_impl,
    add_component_to_blueprint as add_component_to_blueprint_impl,
    set_static_mesh_properties as set_static_mesh_properties_impl,
    set_component_property as set_component_property_impl,
    set_physics_properties as set_physics_properties_impl,
    compile_blueprint as compile_blueprint_impl,
    set_blueprint_property as set_blueprint_property_impl,
    set_pawn_properties as set_pawn_properties_impl,

    add_blueprint_variable as add_blueprint_variable_impl,
    add_interface_to_blueprint as add_interface_to_blueprint_impl,
    create_blueprint_interface as create_blueprint_interface_impl,
    create_custom_blueprint_function as create_custom_blueprint_function_impl,
    call_blueprint_function as call_blueprint_function_impl,
    get_blueprint_metadata as get_blueprint_metadata_impl,
    modify_blueprint_function_properties as modify_blueprint_function_properties_impl,
    capture_blueprint_graph_screenshot as capture_blueprint_graph_screenshot_impl,

    # GAS Pipeline Tools
    create_gameplay_ability as create_gameplay_ability_impl,
    create_gameplay_effect as create_gameplay_effect_impl,
    create_gameplay_cue as create_gameplay_cue_impl
)

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_tools(mcp: FastMCP):
    """Register Blueprint tools with the MCP server."""
    
    @mcp.tool()
    def create_blueprint(
        ctx: Context,
        name: str,
        parent_class: str,
        folder_path: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Blueprint class with support for both native C++ and Blueprint parents.

        Args:
            name: Name of the new Blueprint class (can include path with "/")
            parent_class: Parent class for the Blueprint. Supports:
                         - Native C++ classes: "Actor", "Pawn", "Character", etc.
                         - Blueprint classes: "BP_MyBaseClass" or "/Game/Blueprints/BP_MyBaseClass"
            folder_path: Optional folder path where the blueprint should be created
                         If name contains a path (e.g. "System/Blueprints/my_bp"),
                         folder_path is ignored unless explicitly specified

        Returns:
            Dictionary containing information about the created Blueprint including path and success status

        Examples:
            # Create blueprint with native C++ parent
            create_blueprint(name="MyBlueprint", parent_class="Actor")

            # Create blueprint extending another Blueprint (by name)
            create_blueprint(name="BP_EnemyMelee", parent_class="BP_EnemyBase")

            # Create blueprint extending another Blueprint (by path)
            create_blueprint(
                name="BP_EnemyMelee",
                parent_class="/Game/Blueprints/BP_EnemyBase"
            )

            # Create blueprint with path in name (creates in Content/System/Blueprints)
            create_blueprint(name="System/Blueprints/my_bp", parent_class="Actor")

            # Create blueprint in Content/Success folder
            create_blueprint(name="MyBlueprint", parent_class="Actor", folder_path="Success")
        """
        return create_blueprint_impl(ctx, name, parent_class, folder_path)
    
    @mcp.tool()
    def add_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        is_exposed: bool = False
    ) -> Dict[str, Any]:
        """
        Add a variable to a Blueprint.
        Supports built-in, user-defined struct, delegate types, and class reference types.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type of the variable (Boolean, Integer, Float, Vector, StructName, StructName[], Delegate, Class&lt;ClassName&gt;, etc.)
            is_exposed: Whether to expose the variable to the editor

        Returns:
            Response indicating success or failure

        Examples:
            # Add a basic integer variable
            add_blueprint_variable(
                ctx,
                blueprint_name="PlayerBlueprint",
                variable_name="Score",
                variable_type="Integer",
                is_exposed=True
            )

            # Add a string variable
            add_blueprint_variable(
                ctx,
                blueprint_name="PlayerBlueprint",
                variable_name="PlayerName",
                variable_type="String",
                is_exposed=True
            )

            # Add a vector variable
            add_blueprint_variable(
                ctx,
                blueprint_name="PlayerBlueprint",
                variable_name="Position",
                variable_type="Vector",
                is_exposed=True
            )

            # Add an array variable
            add_blueprint_variable(
                ctx,
                blueprint_name="PlayerBlueprint",
                variable_name="Inventory",
                variable_type="String[]",
                is_exposed=True
            )

            # Add a custom struct variable (using full path)
            add_blueprint_variable(
                ctx,
                blueprint_name="PlayerBlueprint",
                variable_name="Stats",
                variable_type="/Game/DataStructures/PlayerStats",
                is_exposed=True
            )

            # Add a widget blueprint reference
            add_blueprint_variable(
                ctx,
                blueprint_name="BP_HUDController",
                variable_name="MainMenuWidget",
                variable_type="Game/Widgets/WBP_MainMenu",
                is_exposed=True
            )

            # Add a blueprint class reference
            add_blueprint_variable(
                ctx,
                blueprint_name="BP_GameMode",
                variable_name="PlayerPawnClass",
                variable_type="Game/Blueprints/BP_PlayerPawn",
                is_exposed=True
            )

            # Add a class reference variable for UserWidget (can hold any widget blueprint class)
            add_blueprint_variable(
                ctx,
                blueprint_name="BP_UIController",
                variable_name="DialogWidgetClass",
                variable_type="Class&lt;UserWidget&gt;",
                is_exposed=True
            )
        """
        return add_blueprint_variable_impl(ctx, blueprint_name, variable_name, variable_type, is_instance_editable=is_exposed)
    
    @mcp.tool()
    def add_component_to_blueprint(
        ctx: Context,
        blueprint_name: str,
        component_type: str,
        component_name: str,
        location: List[float] = None,
        rotation: List[float] = None,
        scale: List[float] = None,
        component_properties: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Add a component to a Blueprint.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_type: Type of component to add (use component class name without U prefix)
            component_name: Name for the new component
            location: [X, Y, Z] coordinates for component's position
            rotation: [Pitch, Yaw, Roll] values for component's rotation
            scale: [X, Y, Z] values for component's scale
            component_properties: Additional properties to set on the component
        
        Returns:
            Information about the added component
        """
        return add_component_to_blueprint_impl(
            ctx, 
            blueprint_name, 
            component_type, 
            component_name, 
            location, 
            rotation, 
            scale, 
            component_properties
        )
    
    @mcp.tool()
    def set_static_mesh_properties(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        static_mesh: str = "/Engine/BasicShapes/Cube.Cube"
    ) -> Dict[str, Any]:
        """
        Set static mesh properties on a StaticMeshComponent.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the StaticMeshComponent
            static_mesh: Path to the static mesh asset (e.g., "/Engine/BasicShapes/Cube.Cube")
            
        Returns:
            Response indicating success or failure
        """
        return set_static_mesh_properties_impl(ctx, blueprint_name, component_name, static_mesh)
    
    @mcp.tool()
    def set_component_property(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        **kwargs
    ) -> Dict[str, Any]:
        """
        Set one or more properties on a specific component within a UMG Widget Blueprint.

        Parameters:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component to modify
            kwargs: Properties to set as a JSON string. Must be formatted as a valid JSON string.
                   Example format: '{"PropertyName": value}'
                
                Light Component Properties:
                - Intensity: Float value for brightness
                - AttenuationRadius: Float value for light reach
                - SourceRadius: Float value for light source size
                - SoftSourceRadius: Float value for soft light border
                - CastShadows: Boolean for shadow casting

                Transform Properties:
                - RelativeLocation: [x, y, z] float values for position
                - RelativeScale3D: [x, y, z] float values for scale
                
                Collision Properties:
                - CollisionEnabled: String enum value, one of:
                    - "ECollisionEnabled::NoCollision"
                    - "ECollisionEnabled::QueryOnly"
                    - "ECollisionEnabled::PhysicsOnly"
                    - "ECollisionEnabled::QueryAndPhysics"
                - CollisionProfileName: String name of collision profile (e.g. "BlockAll")
                
                Sphere Collision Properties (for SphereComponent):
                - SphereRadius: Float value for collision sphere radius

                For Static Mesh Components:
                - StaticMesh: String path to mesh asset (e.g. "/Game/StarterContent/Shapes/Shape_Cube")

        Returns:
            Dict containing:
                - success: Boolean indicating overall success
                - success_properties: List of property names successfully set
                - failed_properties: List of property names that failed to set, with error messages
                
        Examples:
            # Set light properties
            set_component_property(
                blueprint_name="MyBlueprint",
                component_name="PointLight1",
                kwargs='{"Intensity": 5000.0, "AttenuationRadius": 1000.0, "SourceRadius": 10.0, "SoftSourceRadius": 20.0, "CastShadows": true}'
            )
            
            # Set transform
            set_component_property(
                blueprint_name="MyBlueprint",
                component_name="Mesh1",
                kwargs='{"RelativeLocation": [100.0, 200.0, 50.0], "RelativeScale3D": [1.0, 1.0, 1.0]}'
            )
            
            # Set collision
            set_component_property(
                blueprint_name="MyBlueprint",
                component_name="Mesh1",
                kwargs='{"CollisionEnabled": "ECollisionEnabled::QueryAndPhysics", "CollisionProfileName": "BlockAll"}'
            )

            # Set sphere collision radius
            set_component_property(
                blueprint_name="MyBlueprint",
                component_name="SphereCollision",
                kwargs='{"SphereRadius": 100.0}'
            )

            # Example of incorrect usage (this will not work):
            # set_component_property(
            #     kwargs=SphereRadius=100.0,  # Wrong! Don't use Python kwargs
            #     blueprint_name="BP_DialogueNPC",
            #     component_name="InteractionSphere"
            # )

            # Correct usage with JSON string:
            set_component_property(
                blueprint_name="BP_DialogueNPC",
                component_name="InteractionSphere",
                kwargs='{"SphereRadius": 100.0}'  # Correct! Use JSON string
            )
        """
        return set_component_property_impl(ctx, blueprint_name, component_name, **kwargs)
    
    @mcp.tool()
    def set_physics_properties(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        simulate_physics: bool = True,
        gravity_enabled: bool = True,
        mass: float = 1.0,
        linear_damping: float = 0.01,
        angular_damping: float = 0.0
    ) -> Dict[str, Any]:
        """
        Set physics properties on a component.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component
            simulate_physics: Whether to simulate physics on the component
            gravity_enabled: Whether gravity affects the component
            mass: Mass of the component in kg
            linear_damping: Linear damping factor
            angular_damping: Angular damping factor
            
        Returns:
            Response indicating success or failure
        """
        return set_physics_properties_impl(
            ctx, 
            blueprint_name, 
            component_name, 
            simulate_physics,
            gravity_enabled,
            mass,
            linear_damping,
            angular_damping
        )
    
    @mcp.tool()
    def compile_blueprint(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Compile a Blueprint.
        
        Args:
            blueprint_name: Name of the target Blueprint
            
        Returns:
            Response indicating success or failure with detailed compilation messages
        """
        result = compile_blueprint_impl(ctx, blueprint_name)
        return result

    @mcp.tool()
    def set_blueprint_property(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        property_value: Any
    ) -> Dict[str, Any]:
        """
        Set a property on a Blueprint class default object.
        
        Args:
            blueprint_name: Name of the target Blueprint
            property_name: Name of the property to set
            property_value: Value to set the property to
            
        Returns:
            Response indicating success or failure
        """
        return set_blueprint_property_impl(ctx, blueprint_name, property_name, property_value)

    @mcp.tool()
    def set_pawn_properties(
        ctx: Context,
        blueprint_name: str,
        auto_possess_player: str = "",
        use_controller_rotation_yaw: bool = None,
        use_controller_rotation_pitch: bool = None,
        use_controller_rotation_roll: bool = None,
        can_be_damaged: bool = None
    ) -> Dict[str, Any]:
        """
        Set common Pawn properties on a Blueprint.
        This is a utility function that sets multiple pawn-related properties at once.
        
        Args:
            blueprint_name: Name of the target Blueprint (must be a Pawn or Character)
            auto_possess_player: Auto possess player setting (None, "Disabled", "Player0", "Player1", etc.)
            use_controller_rotation_yaw: Whether the pawn should use the controller's yaw rotation
            use_controller_rotation_pitch: Whether the pawn should use the controller's pitch rotation
            use_controller_rotation_roll: Whether the pawn should use the controller's roll rotation
            can_be_damaged: Whether the pawn can be damaged
            
        Returns:
            Response indicating success or failure with detailed results for each property
        """
        return set_pawn_properties_impl(
            ctx, 
            blueprint_name, 
            auto_possess_player,
            use_controller_rotation_yaw,
            use_controller_rotation_pitch,
            use_controller_rotation_roll,
            can_be_damaged
        )
    

    
    @mcp.tool()
    def call_blueprint_function(
        ctx: Context,
        target_name: str,
        function_name: str,
        string_params: list = None
    ) -> dict:
        """
        Call a BlueprintCallable function by name on the specified target.
        Only supports FString parameters for now.
        """
        return call_blueprint_function_impl(ctx, target_name, function_name, string_params)
    
    @mcp.tool()
    def add_interface_to_blueprint(
        ctx: Context,
        blueprint_name: str,
        interface_name: str
    ) -> Dict[str, Any]:
        """
        Add an interface to a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint (e.g., "BP_MyActor")
            interface_name: Name or path of the interface to add (e.g., "/Game/Blueprints/BPI_MyInterface")

        Returns:
            Response indicating success or failure

        Example:
            add_interface_to_blueprint(
                ctx,
                blueprint_name="BP_MyActor",
                interface_name="/Game/Blueprints/BPI_MyInterface"
            )
        """
        return add_interface_to_blueprint_impl(ctx, blueprint_name, interface_name)
    
    @mcp.tool()
    def create_blueprint_interface(
        ctx: Context,
        name: str,
        folder_path: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Blueprint Interface asset.

        Args:
            name: Name of the new Blueprint Interface (can include path with "/")
            folder_path: Optional folder path where the interface should be created
                         If name contains a path, folder_path is ignored unless explicitly specified

        Returns:
            Dictionary containing information about the created Blueprint Interface including path and success status

        Example:
            create_blueprint_interface(name="MyInterface", folder_path="Blueprints")
        """
        return create_blueprint_interface_impl(ctx, name, folder_path)
    
    @mcp.tool()
    def create_custom_blueprint_function(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        inputs: List[Dict[str, str]] = None,
        outputs: List[Dict[str, str]] = None,
        is_pure: bool = False,
        is_const: bool = False,
        access_specifier: str = "Public",
        category: str = "Default"
    ) -> Dict[str, Any]:
        """
        Create a custom user-defined function in a Blueprint.
        This will create a new function that appears in the Functions section of the Blueprint editor.
        
        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name of the custom function to create
            inputs: List of input parameters, each with 'name' and 'type' keys
            outputs: List of output parameters, each with 'name' and 'type' keys  
            is_pure: Whether the function is pure (no execution pins)
            is_const: Whether the function is const
            access_specifier: Access level ("Public", "Protected", "Private")
            category: Category for organization in the functions list
            
        Returns:
            Dictionary containing success status and function information
            
        Examples:
            # Create a simple function with no parameters
            create_custom_blueprint_function(
                ctx,
                blueprint_name="BP_LoopTest",
                function_name="TestLoopFunction"
            )
            
            # Create a function with input and output parameters
            create_custom_blueprint_function(
                ctx,
                blueprint_name="BP_LoopTest", 
                function_name="ProcessArray",
                inputs=[{"name": "InputArray", "type": "String[]"}],
                outputs=[{"name": "ProcessedCount", "type": "Integer"}]
            )
        """
        return create_custom_blueprint_function_impl(
            ctx,
            blueprint_name,
            function_name,
            inputs,
            outputs,
            is_pure,
            is_const,
            access_specifier,
            category
        )

    @mcp.tool()
    def get_blueprint_metadata(
        ctx: Context,
        blueprint_name: str,
        fields: List[str],
        graph_name: str = None,
        node_type: str = None,
        event_type: str = None,
        detail_level: str = None
    ) -> Dict[str, Any]:
        """
        Get comprehensive metadata about a Blueprint with selective field querying.

        IMPORTANT: At least one field must be specified. When using "graph_nodes",
        the graph_name parameter is REQUIRED to limit response size.

        Args:
            blueprint_name: Name or path of the Blueprint
            fields: REQUIRED list of metadata fields to retrieve. At least one must be specified.
                    Options:
                    - "parent_class": Parent class name and path
                    - "interfaces": Implemented interfaces and their functions
                    - "variables": Blueprint variables with types and default values
                    - "functions": Custom Blueprint functions
                    - "components": All components with names and types
                    - "graphs": Event graphs and function graphs (names and node counts only)
                    - "status": Compilation status and error state
                    - "metadata": Asset metadata and tags
                    - "timelines": Timeline components
                    - "asset_info": Asset path, size, and modification date
                    - "orphaned_nodes": Detects disconnected nodes in all graphs
                    - "graph_nodes": Detailed node info (REQUIRES graph_name parameter)
            graph_name: REQUIRED when using "graph_nodes" field. Specifies which graph to query.
                       Use fields=["graphs"] first to discover available graph names.
            node_type: Optional filter for "graph_nodes" field.
                      Options: "Event", "Function", "Variable", "Comment", or any class name.
            event_type: Optional filter for "graph_nodes" field when node_type="Event".
                       Options: "BeginPlay", "Tick", "EndPlay", "Destroyed", "Construct".
            detail_level: Optional detail level for "graph_nodes" field. Options:
                         - "summary": Node IDs and titles only (minimal output)
                         - "flow": Node IDs, titles, and exec pin connections only (DEFAULT)
                         - "full": Everything including all data pin connections and default values

        Returns:
            Dictionary containing requested metadata fields

        Examples:
            # Get components
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["components"]
            )

            # Get parent class and interfaces
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["parent_class", "interfaces"]
            )

            # First discover available graphs
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["graphs"]
            )

            # Then get nodes from a specific graph (default detail_level="flow")
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["graph_nodes"],
                graph_name="EventGraph"
            )

            # Get only node IDs and titles (minimal output)
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["graph_nodes"],
                graph_name="EventGraph",
                detail_level="summary"
            )

            # Get full details including all data pins and defaults
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["graph_nodes"],
                graph_name="EventGraph",
                detail_level="full"
            )

            # Find Event nodes in EventGraph
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["graph_nodes"],
                graph_name="EventGraph",
                node_type="Event"
            )

            # Find BeginPlay event specifically
            get_blueprint_metadata(
                blueprint_name="BP_MyActor",
                fields=["graph_nodes"],
                graph_name="EventGraph",
                node_type="Event",
                event_type="BeginPlay"
            )
        """
        return get_blueprint_metadata_impl(ctx, blueprint_name, fields, graph_name, node_type, event_type, detail_level)

    @mcp.tool()
    def modify_blueprint_function_properties(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        is_pure: bool = None,
        is_const: bool = None,
        access_specifier: str = None,
        category: str = None
    ) -> Dict[str, Any]:
        """
        Modify properties of an existing Blueprint function.

        Use this to change function attributes after creation, such as converting
        a function from impure to pure, changing access level, or updating category.

        Args:
            blueprint_name: Name or path of the Blueprint
            function_name: Name of the function to modify
            is_pure: If set, changes whether the function is pure (no side effects, no exec pins)
            is_const: If set, changes whether the function is const
            access_specifier: If set, changes access level ("Public", "Protected", "Private")
            category: If set, changes the function's category for organization

        Returns:
            Dictionary containing success status and modified properties

        Examples:
            # Convert a function to pure
            modify_blueprint_function_properties(
                ctx,
                blueprint_name="BP_InventoryComponent",
                function_name="GetTotalItemCount",
                is_pure=True
            )

            # Change access level to private
            modify_blueprint_function_properties(
                ctx,
                blueprint_name="BP_Character",
                function_name="InternalUpdate",
                access_specifier="Private"
            )

            # Change multiple properties at once
            modify_blueprint_function_properties(
                ctx,
                blueprint_name="BP_Utils",
                function_name="CalculateDistance",
                is_pure=True,
                is_const=True,
                category="Math"
            )
        """
        return modify_blueprint_function_properties_impl(
            ctx,
            blueprint_name,
            function_name,
            is_pure,
            is_const,
            access_specifier,
            category
        )

    # =========================================================================
    # GAS (Gameplay Ability System) Pipeline Tools
    # =========================================================================

    @mcp.tool()
    def create_gameplay_ability(
        ctx: Context,
        name: str,
        parent_class: str = "BaseGameplayAbility",
        folder_path: str = "/Game/Abilities",
        ability_type: str = "instant",
        ability_tags: List[str] = None,
        cancel_abilities_with_tag: List[str] = None,
        block_abilities_with_tag: List[str] = None,
        activation_owned_tags: List[str] = None,
        activation_required_tags: List[str] = None,
        activation_blocked_tags: List[str] = None,
        source_required_tags: List[str] = None,
        source_blocked_tags: List[str] = None,
        target_required_tags: List[str] = None,
        target_blocked_tags: List[str] = None,
        cost_effect_class: str = None,
        cooldown_effect_class: str = None,
        cooldown_duration: float = None,
        replication_policy: str = "ReplicateYes",
        instancing_policy: str = "InstancedPerActor"
    ) -> Dict[str, Any]:
        """
        Create a GameplayAbility Blueprint with full GAS configuration.

        This high-level scaffolding tool creates an ability Blueprint with proper parent class
        and configures all common GAS properties in a single call.

        Args:
            name: Name of the ability (e.g., "GA_Dash", "GA_FireBolt")
            parent_class: Parent ability class. Options:
                         - "BaseGameplayAbility" (project's base - default)
                         - "BaseDamageGameplayAbility" (damage abilities)
                         - "BaseAOEDamageAbility" (area damage)
                         - "BaseSummonAbility" (summon mechanics)
                         - "BaseWeaponAbility" (weapon-based)
                         - Any custom ability class path
            folder_path: Content folder (default: "/Game/Abilities")
            ability_type: Behavior type: "instant", "duration", "passive", "toggle"
            ability_tags: Tags identifying this ability (e.g., ["Ability.Movement.Dash"])
            cancel_abilities_with_tag: Tags of abilities this cancels
            block_abilities_with_tag: Tags of abilities this blocks
            activation_owned_tags: Tags applied while active
            activation_required_tags: Tags required on owner to activate
            activation_blocked_tags: Tags on owner that prevent activation
            source_required_tags: Tags required on source (caster) to activate
            source_blocked_tags: Tags on source that prevent activation
            target_required_tags: Tags required on target to activate
            target_blocked_tags: Tags on target that prevent activation
            cost_effect_class: Path to GameplayEffect for cost
            cooldown_effect_class: Path to GameplayEffect for cooldown
            cooldown_duration: Cooldown seconds (if using default cooldown)
            replication_policy: "ReplicateNo" or "ReplicateYes" (default)
            instancing_policy: IMPORTANT for stateful abilities:
                              - "NonInstanced": Shared CDO, no per-actor state
                              - "InstancedPerActor": One per actor (default, recommended)
                              - "InstancedPerExecution": New instance per activation

        Returns:
            Dictionary with created ability info and property results

        Examples:
            # Basic instant ability
            create_gameplay_ability(
                name="GA_Dash",
                ability_tags=["Ability.Movement.Dash"],
                cooldown_duration=2.0
            )

            # Damage ability with target requirements
            create_gameplay_ability(
                name="GA_FireBolt",
                parent_class="BaseDamageGameplayAbility",
                ability_tags=["Ability.Damage.Fire"],
                cost_effect_class="/Game/Effects/GE_ManaCost_Small",
                cooldown_effect_class="/Game/Effects/GE_Cooldown_3s",
                activation_blocked_tags=["State.Dead", "State.Stunned"],
                target_blocked_tags=["State.Immune.Fire"]
            )

            # AOE ability that cancels other attacks
            create_gameplay_ability(
                name="GA_Earthquake",
                parent_class="BaseAOEDamageAbility",
                ability_tags=["Ability.Damage.AOE.Earth"],
                cancel_abilities_with_tag=["Ability.Attack"],
                activation_owned_tags=["State.Casting"],
                source_required_tags=["State.Grounded"]
            )
        """
        return create_gameplay_ability_impl(
            ctx, name, parent_class, folder_path, ability_type,
            ability_tags, cancel_abilities_with_tag, block_abilities_with_tag,
            activation_owned_tags, activation_required_tags, activation_blocked_tags,
            source_required_tags, source_blocked_tags, target_required_tags, target_blocked_tags,
            cost_effect_class, cooldown_effect_class, cooldown_duration,
            replication_policy, instancing_policy
        )

    @mcp.tool()
    def create_gameplay_effect(
        ctx: Context,
        name: str,
        folder_path: str = "/Game/Effects",
        effect_type: str = "instant",
        duration_seconds: float = None,
        period_seconds: float = None,
        modifiers: List[Dict[str, Any]] = None,
        executions: List[str] = None,
        components: List[Dict[str, Any]] = None,
        gameplay_cues: List[Dict[str, Any]] = None,
        asset_tags: List[str] = None,
        granted_tags: List[str] = None,
        application_tag_requirements: List[str] = None,
        removal_tag_requirements: List[str] = None,
        ongoing_tag_requirements: List[str] = None,
        removal_tag_immunity: List[str] = None,
        stacking_type: str = "None",
        stack_limit: int = 1,
        stack_duration_refresh: bool = False,
        stack_period_reset: bool = False
    ) -> Dict[str, Any]:
        """
        Create a GameplayEffect Blueprint with duration, modifiers, and stacking.

        GameplayEffects are the backbone of GAS - they modify attributes, apply tags,
        and trigger gameplay cues. This tool scaffolds common effect patterns.

        Args:
            name: Name of the effect (e.g., "GE_DashCooldown", "GE_FireDamage")
            folder_path: Content folder (default: "/Game/Effects")
            effect_type: Duration type:
                        - "instant": Applied immediately (default)
                        - "duration": Has set duration
                        - "infinite": Lasts until removed
                        - "periodic": Applies periodically
            duration_seconds: Duration for "duration" or "periodic" types
            period_seconds: Period for "periodic" type
            modifiers: Attribute modifiers list:
                      [{"attribute": "VitalAttributesSet.Health", "operation": "Add", "magnitude": -50}]
            executions: Execution calculation class names (e.g., ["BaseDamageExecution"])
            components: GameplayEffect components to add. Supported types:
                       - AbilitiesGameplayEffectComponent: Grants abilities while active
                         {"type": "Abilities", "abilities": ["GA_PassiveRegen"]}
                       - AdditionalEffectsGameplayEffectComponent: Applies additional effects
                         {"type": "AdditionalEffects", "effects": ["GE_BonusDamage"], "on_application": true}
                       - RemoveOtherGameplayEffectComponent: Removes effects by tag
                         {"type": "RemoveOther", "remove_query": {"OwningTagQuery": {"TagFilter": ["Debuff"]}}}
                       - TargetTagsGameplayEffectComponent: Dynamic target tag modification
                         {"type": "TargetTags", "add": ["State.Buffed"], "remove": ["State.Debuffed"]}
                       - ChanceToApplyGameplayEffectComponent: Probability-based application
                         {"type": "ChanceToApply", "chance": 0.5}
            gameplay_cues: GameplayCue triggers for VFX/SFX feedback:
                          [{"cue_tag": "GameplayCue.Impact.Fire", "min_level": 1, "max_level": 5}]
            asset_tags: Tags identifying this effect
            granted_tags: Tags applied to target while active
            application_tag_requirements: Tags required on target to apply
            removal_tag_requirements: Tags that cause removal when gained
            ongoing_tag_requirements: Tags required on target while active (effect removed if lost)
            removal_tag_immunity: Tags that make target immune to this effect's removal
            stacking_type: "None", "AggregateBySource", "AggregateByTarget"
            stack_limit: Maximum stacks
            stack_duration_refresh: Refresh duration on new stack
            stack_period_reset: Reset period on new stack

        Returns:
            Dictionary with created effect info

        Examples:
            # Instant damage effect
            create_gameplay_effect(
                name="GE_FireDamage",
                effect_type="instant",
                modifiers=[
                    {"attribute": "VitalAttributesSet.Health", "operation": "Add", "magnitude": -50}
                ],
                asset_tags=["Effect.Damage.Fire"],
                gameplay_cues=[{"cue_tag": "GameplayCue.Impact.Fire"}]
            )

            # Duration buff with stacking
            create_gameplay_effect(
                name="GE_SpeedBoost",
                effect_type="duration",
                duration_seconds=10.0,
                modifiers=[
                    {"attribute": "BaseAttributesSet.MovementSpeed", "operation": "Multiply", "magnitude": 1.25}
                ],
                granted_tags=["Buff.Speed"],
                stacking_type="AggregateBySource",
                stack_limit=3
            )

            # Periodic healing over time
            create_gameplay_effect(
                name="GE_Regeneration",
                effect_type="periodic",
                duration_seconds=10.0,
                period_seconds=1.0,
                modifiers=[
                    {"attribute": "VitalAttributesSet.Health", "operation": "Add", "magnitude": 5}
                ]
            )

            # Effect that grants abilities while active
            create_gameplay_effect(
                name="GE_BerserkerRage",
                effect_type="duration",
                duration_seconds=15.0,
                components=[
                    {"type": "Abilities", "abilities": ["GA_RageAttack", "GA_RageCharge"]}
                ],
                granted_tags=["State.Berserk"]
            )

            # Buff that requires target to stay in combat
            create_gameplay_effect(
                name="GE_CombatFocus",
                effect_type="infinite",
                modifiers=[
                    {"attribute": "BaseAttributesSet.CritChance", "operation": "Add", "magnitude": 0.15}
                ],
                ongoing_tag_requirements=["State.InCombat"]
            )
        """
        return create_gameplay_effect_impl(
            ctx, name, folder_path, effect_type,
            duration_seconds, period_seconds, modifiers, executions,
            components, gameplay_cues,
            asset_tags, granted_tags, application_tag_requirements, removal_tag_requirements,
            ongoing_tag_requirements, removal_tag_immunity,
            stacking_type, stack_limit, stack_duration_refresh, stack_period_reset
        )

    @mcp.tool()
    def create_gameplay_cue(
        ctx: Context,
        name: str,
        cue_tag: str,
        folder_path: str = "/Game/Abilities/Cues",
        cue_type: str = "Notify",
        implement_on_execute: bool = True,
        implement_on_active: bool = False,
        implement_on_remove: bool = False,
        implement_while_active: bool = False
    ) -> Dict[str, Any]:
        """
        Create a GameplayCue Blueprint for VFX/SFX feedback.

        GameplayCues provide visual and audio feedback for GAS events. They're triggered
        by GameplayEffects or directly from abilities via cue tags.

        Args:
            name: Cue name (e.g., "GCN_Impact_Fire", "GCN_Buff_Haste")
            cue_tag: GameplayCue tag (MUST start with "GameplayCue.")
                    e.g., "GameplayCue.Impact.Fire", "GameplayCue.Buff.Haste"
            folder_path: Content folder (default: "/Game/Abilities/Cues")
            cue_type: Cue implementation type:
                     - "Notify": Static, non-instanced (default, most common)
                     - "Actor": Instanced actor for duration effects
                     - "Static": Pure static, no state
            implement_on_execute: Scaffold OnExecute event (instant cues)
            implement_on_active: Scaffold OnActive event (cue becomes active)
            implement_on_remove: Scaffold OnRemove event (cue removed)
            implement_while_active: Scaffold WhileActive event (tick while active)

        Returns:
            Dictionary with created cue info and requested events

        Examples:
            # Impact cue for instant damage
            create_gameplay_cue(
                name="GCN_Impact_Fire",
                cue_tag="GameplayCue.Impact.Fire",
                implement_on_execute=True
            )

            # Buff cue with duration (shows while buff active)
            create_gameplay_cue(
                name="GCN_Buff_Haste",
                cue_tag="GameplayCue.Buff.Haste",
                cue_type="Actor",
                implement_on_active=True,
                implement_on_remove=True
            )

            # Status effect with ongoing visual
            create_gameplay_cue(
                name="GCN_Status_Burning",
                cue_tag="GameplayCue.Status.Burning",
                cue_type="Actor",
                implement_on_active=True,
                implement_while_active=True,
                implement_on_remove=True
            )
        """
        return create_gameplay_cue_impl(
            ctx, name, cue_tag, folder_path, cue_type,
            implement_on_execute, implement_on_active, implement_on_remove, implement_while_active
        )

    logger.info("Blueprint tools registered successfully")
