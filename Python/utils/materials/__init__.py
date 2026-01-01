"""Materials utilities package."""
from .material_operations import (
    create_material,
    create_material_instance,
    get_material_metadata,
    set_material_parameter,
    get_material_parameter,
    apply_material_to_actor
)

__all__ = [
    "create_material",
    "create_material_instance",
    "get_material_metadata",
    "set_material_parameter",
    "get_material_parameter",
    "apply_material_to_actor"
]
