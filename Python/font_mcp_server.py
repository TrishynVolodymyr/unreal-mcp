from mcp.server.fastmcp import FastMCP
from font_tools.font_tools import register_font_tools

mcp = FastMCP("fontMCP")

register_font_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')
