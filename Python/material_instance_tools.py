"""Material-instance creation transport helpers."""

from typing import Any, Awaitable, Callable, Dict


SendCommand = Callable[[str, Dict[str, Any]], Awaitable[Dict[str, Any]]]


async def create_material_instance_impl(
    send_command: SendCommand,
    name: str,
    parent_material: str,
    folder_path: str = "",
    scalar_params: dict = None,
    vector_params: dict = None,
    texture_params: dict = None,
) -> Dict[str, Any]:
    """Create a Material Instance Constant with optional initial overrides.

    Args:
        name: Asset name, for example ``MI_Crystal_Red``.
        parent_material: Parent material object path or resolvable asset name.
        folder_path: Optional destination folder such as ``/Game/Materials/Instances``.
        scalar_params: Scalar overrides as ``{"Roughness": 0.2}``.
        vector_params: Vector overrides as ``{"BaseColor": [1, 0, 0, 1]}``.
        texture_params: Texture overrides keyed by parameter name.

    Returns:
        Creation status plus the created asset path and parent path.

    Example:
        create_material_instance(
            name="MI_Crystal_Red",
            parent_material="/Game/Materials/M_Crystal",
            scalar_params={"Roughness": 0.2},
            vector_params={"BaseColor": [1.0, 0.0, 0.0, 1.0]},
        )
    """
    params = {"name": name, "parent_material": parent_material}
    if folder_path:
        params["folder_path"] = folder_path
    if scalar_params:
        params["scalar_params"] = scalar_params
    if vector_params:
        params["vector_params"] = vector_params
    if texture_params:
        params["texture_params"] = texture_params
    return await send_command("create_material_instance", params)


async def duplicate_material_instance_impl(
    send_command: SendCommand,
    source_material_instance: str,
    new_name: str,
    folder_path: str = "",
) -> Dict[str, Any]:
    """Duplicate an existing Material Instance while preserving its overrides.

    Args:
        source_material_instance: Source instance object path or resolvable name.
        new_name: Asset name for the duplicate.
        folder_path: Optional destination folder; defaults to the source folder.

    Returns:
        Duplication status plus the new asset path and parent material path.
    """
    params = {
        "source_material_instance": source_material_instance,
        "new_name": new_name,
    }
    if folder_path:
        params["folder_path"] = folder_path
    return await send_command("duplicate_material_instance", params)


def register_material_instance_tools(app, send_command: SendCommand):
    """Register material-instance creation tools and return their callables."""

    async def create_material_instance(
        name: str,
        parent_material: str,
        folder_path: str = "",
        scalar_params: dict = None,
        vector_params: dict = None,
        texture_params: dict = None,
    ) -> Dict[str, Any]:
        return await create_material_instance_impl(
            send_command,
            name,
            parent_material,
            folder_path,
            scalar_params,
            vector_params,
            texture_params,
        )

    async def duplicate_material_instance(
        source_material_instance: str,
        new_name: str,
        folder_path: str = "",
    ) -> Dict[str, Any]:
        return await duplicate_material_instance_impl(
            send_command,
            source_material_instance,
            new_name,
            folder_path,
        )

    create_material_instance.__doc__ = create_material_instance_impl.__doc__
    duplicate_material_instance.__doc__ = duplicate_material_instance_impl.__doc__
    app.tool()(create_material_instance)
    app.tool()(duplicate_material_instance)
    return create_material_instance, duplicate_material_instance
