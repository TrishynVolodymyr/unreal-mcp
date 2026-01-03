"""
Niagara MCP Server

Exposes Niagara VFX-related tools for Unreal Engine via MCP.

## Tools

### Core Asset Management (Feature 1)
- create_niagara_system(name, path, template)
    Create a new Niagara System asset.
- create_niagara_emitter(name, path, template)
    Create a new standalone Niagara Emitter asset.
- add_emitter_to_system(system_path, emitter_path, emitter_name)
    Add an emitter to an existing Niagara System.
- get_niagara_metadata(asset_path, fields)
    Get metadata about a Niagara System or Emitter.
- compile_niagara_asset(asset_path)
    Compile a Niagara System or Emitter.

### Module System (Feature 2)
- add_module_to_emitter(system_path, emitter_name, module_path, stage, index)
    Add a module to a specific stage of an emitter.
- search_niagara_modules(search_query, stage_filter, max_results)
    Search available Niagara modules in the asset registry.
- set_module_input(system_path, emitter_name, module_name, stage, input_name, value, value_type)
    Set an input value on a module.

### Parameters (Feature 3)
- add_niagara_parameter(system_path, parameter_name, parameter_type, default_value, scope)
    Add a parameter to a Niagara System.
- set_niagara_parameter(system_path, parameter_name, value)
    Set the default value of a Niagara parameter.

### Data Interfaces (Feature 4)
- add_data_interface(system_path, emitter_name, interface_type, interface_name)
    Add a Data Interface to an emitter.
- set_data_interface_property(system_path, emitter_name, interface_name, property_name, property_value)
    Set a property on a Data Interface.

### Renderers (Feature 5)
- add_renderer(system_path, emitter_name, renderer_type, renderer_name)
    Add a renderer to an emitter.
- set_renderer_property(system_path, emitter_name, renderer_name, property_name, property_value)
    Set a property on a renderer.

### Level Integration (Feature 6)
- spawn_niagara_actor(system_path, actor_name, location, rotation, auto_activate)
    Spawn a Niagara System actor in the level.

See the main server or tool docstrings for argument details and examples.
"""
from mcp.server.fastmcp import FastMCP
from niagara_tools.niagara_tools import register_niagara_tools

mcp = FastMCP(
    "niagaraMCP",
    description="Niagara VFX tools for Unreal via MCP"
)

register_niagara_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')
