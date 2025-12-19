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
    get_blueprint_metadata as get_blueprint_metadata_impl
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
        fields: List[str] = None
    ) -> Dict[str, Any]:
        """
        Get comprehensive metadata about a Blueprint with selective field querying.

        Args:
            blueprint_name: Name or path of the Blueprint
            fields: List of metadata fields to retrieve. If None or ["*"], returns all.
                    Options: "parent_class", "interfaces", "variables", "functions",
                            "components", "graphs", "status", "metadata",
                            "timelines", "asset_info"

        Returns:
            Dictionary containing requested metadata fields

        Examples:
            # Get all metadata
            get_blueprint_metadata(ctx, blueprint_name="BP_MyActor")

            # Get only parent class and interfaces
            get_blueprint_metadata(
                ctx,
                blueprint_name="BP_MyActor",
                fields=["parent_class", "interfaces"]
            )

            # Get interface implementation status
            result = get_blueprint_metadata(
                ctx,
                blueprint_name="BP_InteractableActor",
                fields=["interfaces"]
            )
            # Result will include interface functions and implementation status
        """
        return get_blueprint_metadata_impl(ctx, blueprint_name, fields)

    logger.info("Blueprint tools registered successfully")
