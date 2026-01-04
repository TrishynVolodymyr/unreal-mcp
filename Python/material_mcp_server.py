"""
Material MCP Server

Exposes Material-related tools for Unreal Engine via MCP.

## Tools

### Material Asset Management
- create_material(name, path, blend_mode, shading_model)
    Create a new Material asset.
- create_material_instance(name, parent_material, path, is_dynamic)
    Create a Material Instance (static or dynamic).
- get_material_metadata(material_path, fields)
    Get material metadata including parameters.
- set_material_parameter(material_path, parameter_name, parameter_type, value)
    Set a scalar, vector, or texture parameter.
- get_material_parameter(material_path, parameter_name, parameter_type)
    Get a material parameter value.
- apply_material_to_actor(actor_name, material_path, slot_index)
    Apply a material to an actor's mesh component.

### Material Expression/Graph Tools
- add_material_expression(material_path, expression_type, position, properties)
    Add a material expression node (Constant, Multiply, Time, etc.)
- connect_material_expressions(material_path, source_id, output_index, target_id, input_name)
    Connect two expressions together.
- connect_expression_to_material_output(material_path, expression_id, output_index, material_property)
    Connect an expression to a material output (BaseColor, Emissive, etc.)
- get_material_graph_metadata(material_path, fields)
    Get all expressions, connections, outputs, orphans, and flow in the material graph.
    Fields: "expressions", "connections", "material_outputs", "orphans", "flow", "*"
- delete_material_expression(material_path, expression_id)
    Remove an expression from the graph.
- set_material_expression_property(material_path, expression_id, property_name, value)
    Modify a property on an existing expression.
- compile_material(material_path)
    Compile/apply material and return validation info including orphan detection.

See the main server or tool docstrings for argument details and examples.
"""
from mcp.server.fastmcp import FastMCP
from material_tools.material_tools import register_material_tools

mcp = FastMCP(
    "materialMCP",
    description="Material tools for Unreal via MCP"
)

register_material_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')
