"""
Editor MCP Server - Level actor manipulation tools.
Use get_mcp_help() for detailed documentation on available tools.
"""
from mcp.server.fastmcp import FastMCP
from editor_tools.editor_tools import register_editor_tools

mcp = FastMCP(
    "editorMCP",
    description="Editor tools for Unreal via MCP"
)

register_editor_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio') 