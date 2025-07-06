from mcp.server.fastmcp import FastMCP
from datatable_tools.datatable_tools import register_datatable_tools

mcp = FastMCP(
    "datatableMCP",
    description="Datatable tools for Unreal via MCP"
)

register_datatable_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')

"""
## DataTable Tools

- **create_datatable(datatable_name, row_struct_name, path="/Game/Data", description="")**
  
  Create a new DataTable asset.
  
  Args:
    - datatable_name (str): Name of the DataTable to create
    - row_struct_name (str): Name or path of the struct to use for rows. Can be a full path (recommended) or a simple name.
    - path (str): Path where to create the DataTable (defaults to "/Game/Data")
    - description (str): Optional description for the DataTable
  
  Returns: Dict containing success status and created asset info.
  
  Example:
    create_datatable(
        datatable_name="ItemTable",
        row_struct_name="/Game/Data/ItemStruct",
        path="/Game/Data"
    )
    create_datatable(
        datatable_name="ItemTable",
        row_struct_name="ItemStruct"
    )
    create_datatable(
        datatable_name="ItemTable",
        row_struct_name="/Game/Data/ItemStruct",
        description="Table containing all game items"
    )

- **get_datatable_rows(datatable_name, row_names=None)**
  
  Get rows from a DataTable.
  
  Args:
    - datatable_name (str): Name of the target DataTable
    - row_names (list, optional): List of specific row names to retrieve
  
  Returns: Dict containing the requested rows.

- **delete_datatable_row(datatable_name, row_name)**
  
  Delete a row from a DataTable.
  
  Args:
    - datatable_name (str): Name of the target DataTable
    - row_name (str): Name of the row to delete
  
  Returns: Dict containing success status and updated DataTable info.

- **get_datatable_row_names(datatable_name)**
  
  Get all row names and struct field names from a DataTable.
  
  Args:
    - datatable_name (str): Name of the target DataTable
  
  Returns: Dict containing:
    - 'row_names': list of row names (str)
    - 'field_names': list of struct field names (str)

- **add_rows_to_datatable(datatable_name, rows)**
  
  Add multiple rows to an existing DataTable.
  
  Args:
    - datatable_name (str): Name of the target DataTable (full path or simple name)
    - rows (list): List of dicts, each with:
      - 'row_name': Unique name for the row
      - 'row_data': Dict of property values using the internal GUID-based property names (get these from get_datatable_row_names)
  
  Returns: Dict containing success status and list of added row names.
  
  Example:
    property_info = get_datatable_row_names("MyItemTable")
    field_names = property_info["result"]["field_names"]
    add_rows_to_datatable(
        datatable_name="/Game/Data/MyItemTable",
        rows=[{
            "row_name": "Item1",
            "row_data": {
                field_names[0]: "Sword",
                field_names[1]: 99.99,
                field_names[2]: 5,
                field_names[3]: True,
                field_names[4]: ["weapon"]
            }
        }]
    )

- **update_rows_in_datatable(datatable_name, rows)**
  
  Update multiple rows in an existing DataTable.
  
  Args:
    - datatable_name (str): Name of the target DataTable (full path or simple name)
    - rows (list): List of dicts, each with:
      - 'row_name': Name of the row to update
      - 'row_data': Dict of property values using the internal GUID-based property names (get these from get_datatable_row_names)
  
  Returns: Dict containing success status and list of updated/failed row names.
  
  Example:
    property_info = get_datatable_row_names("MyItemTable")
    field_names = property_info["result"]["field_names"]
    update_rows_in_datatable(
        datatable_name="/Game/Data/MyItemTable",
        rows=[{
            "row_name": "Item1",
            "row_data": {
                field_names[0]: "Enhanced Sword",
                field_names[1]: 149.99,
                field_names[2]: 3,
                field_names[3]: True,
                field_names[4]: ["weapon", "rare"]
            }
        }]
    )

- **delete_datatable_rows(datatable_name, row_names)**
  
  Delete multiple rows from a DataTable.
  
  Args:
    - datatable_name (str): Name of the target DataTable
    - row_names (list): List of row names to delete
  
  Returns: Dict containing success status and updated DataTable info.
""" 