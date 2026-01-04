"""Niagara Tools for Unreal MCP."""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context
from utils.unreal_connection_utils import send_unreal_command

logger = logging.getLogger("UnrealMCP")


def register_niagara_tools(mcp: FastMCP):
    """Register Niagara tools with the MCP server."""

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
            name: System name (e.g., "NS_FireEffect")
            path: Content path for the system
            template: Optional template system path to copy from
        """
        params = {"name": name, "path": path}
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
            name: Emitter name (e.g., "NE_Sparks")
            path: Content path for the emitter
            template: Optional template emitter path to copy from
        """
        params = {"name": name, "path": path}
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
            system_path: Path to the target system
            emitter_path: Path to the emitter to add
            emitter_name: Optional display name in system
        """
        params = {"system_path": system_path, "emitter_path": emitter_path}
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
            fields: Filter: "emitters"|"modules"|"parameters"|"renderers"|"status"|"*"

        Returns:
            For Systems:
                - emitters[]: List of emitter handles (name, id, enabled, emitter_path)
                - modules_by_emitter[]: Modules per emitter (spawn_modules, update_modules)
                - renderers_by_emitter[]: Renderers per emitter (name, type, enabled, renderer_count)
                - parameters[]: Exposed system parameters
                - compile_status: "Valid" or "Invalid"
            For Emitters:
                - renderers[]: Direct renderer list
                - version: Emitter version GUID

        Example:
            get_niagara_metadata("/Game/VFX/NS_Fire", ["renderers"])
        """
        params = {"asset_path": asset_path}
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
            asset_path: Path to the Niagara asset
        """
        params = {"asset_path": asset_path}
        logger.info(f"Compiling Niagara asset '{asset_path}'")
        return send_unreal_command("compile_niagara_asset", params)

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
            emitter_name: Emitter name within the system
            module_path: Path to the module script asset
            stage: "Spawn"|"Update"|"Event"
            index: Position in stack (-1 = end)
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
            search_query: Text to search in module names
            stage_filter: "Spawn"|"Update"|"Event"|"" (all)
            max_results: Maximum results to return

        Returns:
            Dict with: modules[] (path, name, description), count

        Example:
            search_niagara_modules("Location", stage_filter="Spawn")
        """
        params = {
            "search_query": search_query,
            "stage_filter": stage_filter,
            "max_results": max_results
        }
        logger.info(f"Searching Niagara modules query='{search_query}', stage='{stage_filter}'")
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
            emitter_name: Emitter name
            module_name: Module name
            stage: Stage containing the module
            input_name: Input parameter name
            value: Value to set (parsed by C++ side)
            value_type: "float"|"vector"|"int"|"bool"|"auto"
        """
        params = {
            "system_path": system_path,
            "emitter_name": emitter_name,
            "module_name": module_name,
            "stage": stage,
            "input_name": input_name,
            "value": str(value),
            "value_type": value_type
        }
        logger.info(f"Setting module input '{input_name}' on '{module_name}' to '{value}'")
        return send_unreal_command("set_module_input", params)

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
            parameter_name: Parameter name (e.g., "User.SpawnRate")
            parameter_type: "Float"|"Int"|"Bool"|"Vector"|"LinearColor"
            default_value: Optional default value as string
            scope: "user"|"system"|"emitter"
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
            parameter_name: Parameter name
            value: New default value (parsed by C++ side)
        """
        params = {
            "system_path": system_path,
            "parameter_name": parameter_name,
            "value": str(value)
        }
        logger.info(f"Setting parameter '{parameter_name}' to '{value}' on '{system_path}'")
        return send_unreal_command("set_niagara_parameter", params)

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
            emitter_name: Emitter name
            interface_type: "StaticMesh"|"SkeletalMesh"|"Spline"|"Audio"|"Curve"|"Texture"|"RenderTarget"|"Grid2D"|"Grid3D"
            interface_name: Optional custom name
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
            emitter_name: Emitter name
            interface_name: Data interface name
            property_name: Property to set (e.g., "Mesh", "Source")
            property_value: Value (asset path for references, or numeric/bool)
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
            emitter_name: Emitter name
            renderer_type: "Sprite"|"Mesh"|"Ribbon"|"Light"|"Decal"|"Component"
            renderer_name: Optional custom name
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
            emitter_name: Emitter name
            renderer_name: Renderer name or index ("Renderer_0")
            property_name: Property (Sprite: "Material", Mesh: "ParticleMesh", Ribbon: "RibbonWidth", etc.)
            property_value: Value (asset path, numeric, or boolean)
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
            location: [X, Y, Z] world location
            rotation: [Pitch, Yaw, Roll] in degrees
            auto_activate: Whether to auto-activate on spawn
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

    logger.info("Niagara tools registered successfully")
