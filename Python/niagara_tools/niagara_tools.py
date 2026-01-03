"""
Niagara Tools for Unreal MCP.

This module provides tools for creating and manipulating Unreal Engine Niagara VFX systems.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_niagara_tools(mcp: FastMCP):
    """Register Niagara tools with the MCP server."""

    # =========================================================================
    # Feature 1: Core Asset Management
    # =========================================================================

    @mcp.tool()
    def create_niagara_system(
        ctx: Context,
        name: str,
        path: str = "/Game/Niagara",
        template: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Niagara System asset.

        Args:
            name: Name of the system (e.g., "NS_FireEffect")
            path: Content path where the system should be created
            template: Optional template system path to copy from

        Returns:
            Dict containing:
                - success: Whether the system was created
                - system_path: Full path to the created system
                - message: Status message

        Examples:
            # Create a basic Niagara system
            create_niagara_system(name="NS_Sparks")

            # Create in a specific folder
            create_niagara_system(
                name="NS_Fire",
                path="/Game/VFX/Fire"
            )

            # Create from a template
            create_niagara_system(
                name="NS_CustomFire",
                template="/Game/VFX/Templates/NS_FireTemplate"
            )
        """
        params = {
            "name": name,
            "path": path
        }
        if template:
            params["template"] = template

        logger.info(f"Creating Niagara system '{name}' at '{path}'")
        return send_unreal_command("create_niagara_system", params)

    @mcp.tool()
    def create_niagara_emitter(
        ctx: Context,
        name: str,
        path: str = "/Game/Niagara",
        template: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new standalone Niagara Emitter asset.

        Args:
            name: Name of the emitter (e.g., "NE_Sparks")
            path: Content path where the emitter should be created
            template: Optional template emitter path to copy from

        Returns:
            Dict containing:
                - success: Whether the emitter was created
                - emitter_path: Full path to the created emitter
                - message: Status message

        Examples:
            # Create a basic emitter
            create_niagara_emitter(name="NE_Smoke")

            # Create from a template
            create_niagara_emitter(
                name="NE_CustomSmoke",
                template="/Game/VFX/Templates/NE_SmokeTemplate"
            )
        """
        params = {
            "name": name,
            "path": path
        }
        if template:
            params["template"] = template

        logger.info(f"Creating Niagara emitter '{name}' at '{path}'")
        return send_unreal_command("create_niagara_emitter", params)

    @mcp.tool()
    def add_emitter_to_system(
        ctx: Context,
        system_path: str,
        emitter_path: str,
        emitter_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add an emitter to an existing Niagara System.

        Args:
            system_path: Path to the target system (e.g., "/Game/Niagara/NS_Fire")
            emitter_path: Path to the emitter to add
            emitter_name: Optional display name for the emitter in system

        Returns:
            Dict containing:
                - success: Whether the emitter was added
                - emitter_handle_id: GUID handle for the added emitter
                - message: Status message

        Examples:
            # Add an emitter to a system
            add_emitter_to_system(
                system_path="/Game/VFX/NS_Explosion",
                emitter_path="/Game/VFX/Emitters/NE_Debris"
            )

            # Add with custom name
            add_emitter_to_system(
                system_path="/Game/VFX/NS_Explosion",
                emitter_path="/Game/VFX/Emitters/NE_Sparks",
                emitter_name="MainSparks"
            )
        """
        params = {
            "system_path": system_path,
            "emitter_path": emitter_path
        }
        if emitter_name:
            params["emitter_name"] = emitter_name

        logger.info(f"Adding emitter '{emitter_path}' to system '{system_path}'")
        return send_unreal_command("add_emitter_to_system", params)

    @mcp.tool()
    def get_niagara_metadata(
        ctx: Context,
        asset_path: str,
        fields: List[str] = None
    ) -> Dict[str, Any]:
        """
        Get metadata about a Niagara System or Emitter.

        Args:
            asset_path: Path to the Niagara asset
            fields: Optional list of fields to retrieve:
                - "emitters": List of emitters (systems only)
                - "modules": Modules per emitter stage
                - "parameters": User-exposed parameters
                - "renderers": Renderer configurations
                - "data_interfaces": Data interfaces in use
                - "*": All fields (default)

        Returns:
            Dict containing:
                - success: Whether the asset was found
                - asset_type: "System" or "Emitter"
                - (requested fields)
                - message: Status message

        Examples:
            # Get all metadata
            get_niagara_metadata(asset_path="/Game/VFX/NS_Fire")

            # Get only emitters and parameters
            get_niagara_metadata(
                asset_path="/Game/VFX/NS_Fire",
                fields=["emitters", "parameters"]
            )
        """
        params = {
            "asset_path": asset_path
        }
        if fields is not None:
            params["fields"] = fields

        logger.info(f"Getting metadata for Niagara asset '{asset_path}'")
        return send_unreal_command("get_niagara_metadata", params)

    @mcp.tool()
    def compile_niagara_asset(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Compile a Niagara System or Emitter.

        Args:
            asset_path: Path to the Niagara asset to compile

        Returns:
            Dict containing:
                - success: Whether compilation succeeded
                - compilation_time_seconds: Time taken to compile
                - warnings: List of compilation warnings
                - errors: List of compilation errors (if failed)
                - message: Status message

        Examples:
            # Compile a system
            compile_niagara_asset(asset_path="/Game/VFX/NS_Fire")
        """
        params = {
            "asset_path": asset_path
        }

        logger.info(f"Compiling Niagara asset '{asset_path}'")
        return send_unreal_command("compile_niagara_asset", params)

    # =========================================================================
    # Feature 2: Module System
    # =========================================================================

    @mcp.tool()
    def add_module_to_emitter(
        ctx: Context,
        system_path: str,
        emitter_name: str,
        module_path: str,
        stage: str,
        index: int = -1
    ) -> Dict[str, Any]:
        """
        Add a module to a specific stage of an emitter.

        Args:
            system_path: Path to the Niagara System
            emitter_name: Name of the emitter within the system
            module_path: Path to the module script asset
            stage: Stage to add to ("Spawn", "Update", "Event")
            index: Position in stack (-1 = end)

        Returns:
            Dict containing:
                - success: Whether the module was added
                - node_id: Identifier for the added module node
                - message: Status message

        Examples:
            # Add a spawn module
            add_module_to_emitter(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                module_path="/Niagara/Modules/Location/SphereLocation",
                stage="Spawn"
            )

            # Add to specific position
            add_module_to_emitter(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                module_path="/Niagara/Modules/Update/Gravity",
                stage="Update",
                index=0
            )
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "module_path": module_path,
            "stage": stage,
            "index": index
        }

        logger.info(f"Adding module '{module_path}' to emitter '{emitter_name}' stage '{stage}'")
        return send_unreal_command("add_module_to_emitter", params)

    @mcp.tool()
    def search_niagara_modules(
        ctx: Context,
        search_query: str = "",
        stage_filter: str = "",
        max_results: int = 50
    ) -> Dict[str, Any]:
        """
        Search available Niagara modules in the asset registry.

        Args:
            search_query: Text to search for in module names
            stage_filter: Filter by compatible stage ("Spawn", "Update", "Event", "")
            max_results: Maximum number of results

        Returns:
            Dict containing:
                - success: Whether the search completed
                - modules: List of matching modules with path, name, description
                - count: Number of modules found
                - message: Status message

        Examples:
            # Search for location modules
            search_niagara_modules(search_query="Location")

            # Search for spawn-compatible modules
            search_niagara_modules(
                search_query="",
                stage_filter="Spawn"
            )
        """
        params = {
            "search_query": search_query,
            "stage_filter": stage_filter,
            "max_results": max_results
        }

        logger.info(f"Searching Niagara modules with query='{search_query}', stage='{stage_filter}'")
        return send_unreal_command("search_niagara_modules", params)

    @mcp.tool()
    def set_module_input(
        ctx: Context,
        system_path: str,
        emitter_name: str,
        module_name: str,
        stage: str,
        input_name: str,
        value: str | int | float,
        value_type: str = "auto"
    ) -> Dict[str, Any]:
        """
        Set an input value on a module.

        Args:
            system_path: Path to the Niagara System
            emitter_name: Name of the emitter
            module_name: Name of the module
            stage: Stage containing the module
            input_name: Name of the input parameter
            value: Value to set (as string, will be parsed)
            value_type: Type hint ("float", "vector", "int", "bool", "auto")

        Returns:
            Dict containing:
                - success: Whether the input was set
                - previous_value: Previous value
                - new_value: New value that was set
                - message: Status message

        Examples:
            # Set a float input
            set_module_input(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                module_name="Sphere Location",
                stage="Spawn",
                input_name="Radius",
                value="100.0"
            )

            # Set a vector input
            set_module_input(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                module_name="Add Velocity",
                stage="Spawn",
                input_name="Velocity",
                value="0,0,500",
                value_type="vector"
            )
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "module_name": module_name,
            "stage": stage,
            "input_name": input_name,
            "value": str(value),  # Convert to string for C++ side
            "value_type": value_type
        }

        logger.info(f"Setting module input '{input_name}' on '{module_name}' to '{value}'")
        return send_unreal_command("set_module_input", params)

    # =========================================================================
    # Feature 3: Parameters
    # =========================================================================

    @mcp.tool()
    def add_niagara_parameter(
        ctx: Context,
        system_path: str,
        parameter_name: str,
        parameter_type: str,
        default_value: str = "",
        scope: str = "user"
    ) -> Dict[str, Any]:
        """
        Add a parameter to a Niagara System.

        Args:
            system_path: Path to the Niagara System
            parameter_name: Name of the parameter (e.g., "User.SpawnRate")
            parameter_type: Type ("Float", "Int", "Bool", "Vector", "LinearColor")
            default_value: Optional default value (as string)
            scope: Parameter scope ("user", "system", "emitter")

        Returns:
            Dict containing:
                - success: Whether the parameter was added
                - parameter_info: Details about the added parameter
                - message: Status message

        Examples:
            # Add a float parameter
            add_niagara_parameter(
                system_path="/Game/VFX/NS_Fire",
                parameter_name="User.Intensity",
                parameter_type="Float",
                default_value="1.0"
            )

            # Add a color parameter
            add_niagara_parameter(
                system_path="/Game/VFX/NS_Fire",
                parameter_name="User.FlameColor",
                parameter_type="LinearColor",
                default_value="1.0,0.5,0.0,1.0"
            )
        """
        params = {
            "system_path": system_path,
            "parameter_name": parameter_name,
            "parameter_type": parameter_type,
            "scope": scope
        }
        if default_value:
            params["default_value"] = default_value

        logger.info(f"Adding parameter '{parameter_name}' ({parameter_type}) to '{system_path}'")
        return send_unreal_command("add_niagara_parameter", params)

    @mcp.tool()
    def set_niagara_parameter(
        ctx: Context,
        system_path: str,
        parameter_name: str,
        value: str | int | float
    ) -> Dict[str, Any]:
        """
        Set the default value of a Niagara parameter.

        Args:
            system_path: Path to the Niagara System
            parameter_name: Name of the parameter
            value: New default value (as string, will be parsed by C++ side)

        Returns:
            Dict containing:
                - success: Whether the parameter was set
                - previous_value: Previous value
                - new_value: New value that was set
                - message: Status message

        Examples:
            # Set a float parameter
            set_niagara_parameter(
                system_path="/Game/VFX/NS_Fire",
                parameter_name="User.Intensity",
                value=2.5
            )
        """
        params = {
            "system_path": system_path,
            "parameter_name": parameter_name,
            "value": str(value)
        }

        logger.info(f"Setting parameter '{parameter_name}' to '{value}' on '{system_path}'")
        return send_unreal_command("set_niagara_parameter", params)

    # =========================================================================
    # Feature 4: Data Interfaces
    # =========================================================================

    @mcp.tool()
    def add_data_interface(
        ctx: Context,
        system_path: str,
        emitter_name: str,
        interface_type: str,
        interface_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Data Interface to an emitter.

        Args:
            system_path: Path to the Niagara System
            emitter_name: Name of the emitter
            interface_type: Type of data interface:
                - "StaticMesh"
                - "SkeletalMesh"
                - "Spline"
                - "Audio"
                - "Curve"
                - "Texture"
                - "RenderTarget"
                - "Grid2D"
                - "Grid3D"
            interface_name: Optional custom name for the interface

        Returns:
            Dict containing:
                - success: Whether the data interface was added
                - interface_id: Identifier for the added interface
                - available_properties: Properties that can be configured
                - message: Status message

        Examples:
            # Add a static mesh data interface
            add_data_interface(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                interface_type="StaticMesh",
                interface_name="FireMesh"
            )
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "interface_type": interface_type
        }
        if interface_name:
            params["interface_name"] = interface_name

        logger.info(f"Adding data interface '{interface_type}' to emitter '{emitter_name}'")
        return send_unreal_command("add_data_interface", params)

    @mcp.tool()
    def set_data_interface_property(
        ctx: Context,
        system_path: str,
        emitter_name: str,
        interface_name: str,
        property_name: str,
        property_value: str | int | float | bool
    ) -> Dict[str, Any]:
        """
        Set a property on a Data Interface.

        Args:
            system_path: Path to the Niagara System
            emitter_name: Name of the emitter
            interface_name: Name of the data interface
            property_name: Property to set (e.g., "Mesh", "Source", "SamplingRegion")
            property_value: Value to set (asset path for references, or numeric/bool values)

        Returns:
            Dict containing:
                - success: Whether the property was set
                - message: Status message

        Examples:
            # Set the mesh for a StaticMesh data interface
            set_data_interface_property(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                interface_name="FireMesh",
                property_name="Mesh",
                property_value="/Game/Meshes/SM_Flame"
            )
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "interface_name": interface_name,
            "property_name": property_name,
            "property_value": str(property_value)
        }

        logger.info(f"Setting data interface property '{property_name}' to '{property_value}'")
        return send_unreal_command("set_data_interface_property", params)

    # =========================================================================
    # Feature 5: Renderers
    # =========================================================================

    @mcp.tool()
    def add_renderer(
        ctx: Context,
        system_path: str,
        emitter_name: str,
        renderer_type: str,
        renderer_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a renderer to an emitter.

        Args:
            system_path: Path to the Niagara System
            emitter_name: Name of the emitter
            renderer_type: Type of renderer:
                - "Sprite" (UNiagaraSpriteRendererProperties)
                - "Mesh" (UNiagaraMeshRendererProperties)
                - "Ribbon" (UNiagaraRibbonRendererProperties)
                - "Light" (UNiagaraLightRendererProperties)
                - "Decal" (UNiagaraDecalRendererProperties)
                - "Component" (UNiagaraComponentRendererProperties)
            renderer_name: Optional custom name

        Returns:
            Dict containing:
                - success: Whether the renderer was added
                - renderer_id: Identifier for the added renderer
                - available_properties: Properties that can be configured
                - message: Status message

        Examples:
            # Add a sprite renderer
            add_renderer(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                renderer_type="Sprite"
            )

            # Add a mesh renderer
            add_renderer(
                system_path="/Game/VFX/NS_Debris",
                emitter_name="Rocks",
                renderer_type="Mesh",
                renderer_name="RockMeshRenderer"
            )
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "renderer_type": renderer_type
        }
        if renderer_name:
            params["renderer_name"] = renderer_name

        logger.info(f"Adding renderer '{renderer_type}' to emitter '{emitter_name}'")
        return send_unreal_command("add_renderer", params)

    @mcp.tool()
    def set_renderer_property(
        ctx: Context,
        system_path: str,
        emitter_name: str,
        renderer_name: str,
        property_name: str,
        property_value: str | int | float | bool
    ) -> Dict[str, Any]:
        """
        Set a property on a renderer.

        Args:
            system_path: Path to the Niagara System
            emitter_name: Name of the emitter
            renderer_name: Name of the renderer (or index like "Renderer_0")
            property_name: Property to set. Common properties:
                - Sprite: "Material", "SubImageSize", "FacingMode", "Alignment"
                - Mesh: "ParticleMesh", "OverrideMaterials"
                - Ribbon: "Material", "FacingMode", "RibbonWidth"
                - Light: "LightExponent", "Radius", "ColorAdd"
            property_value: Value to set (asset path, numeric, or boolean)

        Returns:
            Dict containing:
                - success: Whether the property was set
                - message: Status message

        Examples:
            # Set material on a sprite renderer
            set_renderer_property(
                system_path="/Game/VFX/NS_Fire",
                emitter_name="Flames",
                renderer_name="SpriteRenderer",
                property_name="Material",
                property_value="/Game/Materials/M_Fire"
            )

            # Set mesh on a mesh renderer
            set_renderer_property(
                system_path="/Game/VFX/NS_Debris",
                emitter_name="Rocks",
                renderer_name="RockMeshRenderer",
                property_name="ParticleMesh",
                property_value="/Game/Meshes/SM_Rock"
            )
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "renderer_name": renderer_name,
            "property_name": property_name,
            "property_value": str(property_value)
        }

        logger.info(f"Setting renderer property '{property_name}' to '{property_value}'")
        return send_unreal_command("set_renderer_property", params)

    # =========================================================================
    # Feature 6: Level Integration
    # =========================================================================

    @mcp.tool()
    def spawn_niagara_actor(
        ctx: Context,
        system_path: str,
        actor_name: str,
        location: List[float] = None,
        rotation: List[float] = None,
        auto_activate: bool = True
    ) -> Dict[str, Any]:
        """
        Spawn a Niagara System actor in the level.

        Args:
            system_path: Path to the Niagara System asset
            actor_name: Name for the spawned actor
            location: [X, Y, Z] world location (default: [0, 0, 0])
            rotation: [Pitch, Yaw, Roll] rotation in degrees (default: [0, 0, 0])
            auto_activate: Whether to auto-activate on spawn

        Returns:
            Dict containing:
                - success: Whether the actor was spawned
                - actor_name: Name of the spawned actor
                - component_info: Information about the Niagara component
                - message: Status message

        Examples:
            # Spawn at origin
            spawn_niagara_actor(
                system_path="/Game/VFX/NS_Fire",
                actor_name="FireEffect_1"
            )

            # Spawn at specific location
            spawn_niagara_actor(
                system_path="/Game/VFX/NS_Fire",
                actor_name="FireEffect_2",
                location=[100.0, 200.0, 50.0],
                rotation=[0.0, 45.0, 0.0]
            )

            # Spawn inactive (for later activation)
            spawn_niagara_actor(
                system_path="/Game/VFX/NS_Explosion",
                actor_name="Explosion_Preset",
                location=[0.0, 0.0, 100.0],
                auto_activate=False
            )
        """
        params = {
            "system_path": system_path,
            "actor_name": actor_name,
            "auto_activate": auto_activate
        }
        if location is not None:
            params["location"] = location
        if rotation is not None:
            params["rotation"] = rotation

        logger.info(f"Spawning Niagara actor '{actor_name}' with system '{system_path}'")
        return send_unreal_command("spawn_niagara_actor", params)
