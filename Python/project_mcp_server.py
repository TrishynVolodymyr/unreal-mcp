from mcp.server.fastmcp import FastMCP
from project_tools.project_tools import register_project_tools

mcp = FastMCP(
    "projectMCP",
    description="Project tools for Unreal via MCP"
)

register_project_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')

"""
## Project Tools

- **create_input_mapping(action_name, key, input_type="Action")**
  
  Create an input mapping for the project.
  
  Args:
    - action_name (str): Name of the input action
    - key (str): Key to bind (SpaceBar, LeftMouseButton, etc.)
    - input_type (str): Type of input mapping (Action or Axis)
  
  Returns: Response indicating success or failure.
  
  Example:
    create_input_mapping(action_name="Jump", key="SpaceBar")

- **create_folder(folder_path)**
  
  Create a new folder in the Unreal project. Can create both content folders (for the content browser) and regular project folders.
  
  Args:
    - folder_path (str): Path to create, relative to project root. Use "Content/..." prefix for content browser folders.
  
  Returns: Dictionary with the creation status and folder path.
  
  Example:
    create_folder(folder_path="Content/MyGameContent")
    create_folder(folder_path="Intermediate/MyTools")

- **create_struct(struct_name, properties, path="/Game/Blueprints", description="")**
  
  Create a new Unreal struct.
  
  Args:
    - struct_name (str): Name of the struct to create
    - properties (list): List of property dictionaries, each containing:
      - name: Property name
      - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
      - description: (Optional) Property description
    - path (str): Path where to create the struct
    - description (str): Optional description for the struct
  
  Returns: Dictionary with the creation status and struct path.
  
  Example:
    create_struct(
        struct_name="Item",
        properties=[
            {"name": "Name", "type": "String"},
            {"name": "Value", "type": "Integer"},
            {"name": "IsRare", "type": "Boolean"}
        ],
        path="/Game/DataStructures"
    )

- **update_struct(struct_name, properties, path="/Game/Blueprints", description="")**
  
  Update an existing Unreal struct.
  
  Args:
    - struct_name (str): Name of the struct to update
    - properties (list): List of property dictionaries, each containing:
      - name: Property name
      - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
      - description: (Optional) Property description
    - path (str): Path where the struct exists
    - description (str): Optional description for the struct
  
  Returns: Dictionary with the update status and struct path.

- **show_struct_variables(struct_name, path="/Game/Blueprints")**
  
  Show variables and types of a struct in Unreal Engine.
  
  Args:
    - struct_name (str): Name of the struct to inspect
    - path (str): Path where the struct exists (default: /Game/Blueprints)
  
  Returns: Dictionary with struct variable info.

- **list_folder_contents(folder_path)**
  
  List the contents of a folder in the Unreal project (content or regular folder).
  
  Args:
    - folder_path (str): Path to the folder (e.g., "/Game/Blueprints" or "Content/MyFolder" or "Intermediate/MyTools")
  
  Returns: Dictionary with arrays of subfolders and files/assets.
""" 