"""
PCG (Procedural Content Generation) tools for Unreal Engine.
Tools are registered on the FastMCP app passed from pcg_mcp_server.py.
"""

# Tool implementations will be added here by agents.
# Each tool follows the pattern:
#
# @app.tool()
# async def create_pcg_graph(name: str, path: str = "/Game/PCG") -> dict:
#     """Create a new PCG Graph asset."""
#     return await send_tcp_command("create_pcg_graph", {"name": name, "path": path})
