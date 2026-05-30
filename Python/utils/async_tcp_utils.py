"""
Async TCP client for Unreal MCP servers.

Provides timeout-protected, incremental JSON reads so MCP tools never hang silently.
"""

import asyncio
import json
import logging
from typing import Any, Dict

logger = logging.getLogger("UnrealMCP")

UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557
DEFAULT_COMMAND_TIMEOUT = 130.0
MAX_RESPONSE_BYTES = 4 * 1024 * 1024
RECV_CHUNK_SIZE = 8192


async def _read_complete_json(reader: asyncio.StreamReader, max_bytes: int) -> bytes:
    """Read from the socket until a complete JSON document is received."""
    chunks = []
    total_bytes = 0

    while total_bytes < max_bytes:
        chunk = await reader.read(RECV_CHUNK_SIZE)
        if not chunk:
            if not chunks:
                raise ConnectionError("Connection closed before receiving data")
            break

        chunks.append(chunk)
        total_bytes += len(chunk)
        data = b"".join(chunks)

        try:
            json.loads(data.decode("utf-8"))
            return data
        except json.JSONDecodeError:
            continue

    if total_bytes >= max_bytes:
        raise ValueError(f"Response exceeds maximum size of {max_bytes} bytes")

    data = b"".join(chunks)
    json.loads(data.decode("utf-8"))
    return data


async def _send_tcp_command_impl(command_type: str, params: Dict[str, Any]) -> Dict[str, Any]:
    command_data = {"type": command_type, "params": params}
    json_data = json.dumps(command_data)

    reader, writer = await asyncio.open_connection(UNREAL_HOST, UNREAL_PORT)
    try:
        writer.write(json_data.encode("utf-8"))
        await writer.drain()

        response_data = await _read_complete_json(reader, MAX_RESPONSE_BYTES)
        response_str = response_data.decode("utf-8").strip()

        if not response_str:
            return {"success": False, "error": "Empty response from server"}

        try:
            return json.loads(response_str)
        except json.JSONDecodeError as json_err:
            return {
                "success": False,
                "error": f"JSON decode error: {str(json_err)}, Response: {response_str[:200]}",
            }
    finally:
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass


async def send_tcp_command(
    command_type: str,
    params: Dict[str, Any] = None,
    timeout: float = DEFAULT_COMMAND_TIMEOUT,
) -> Dict[str, Any]:
    """Send a command to the Unreal Engine TCP server with a hard timeout."""
    try:
        return await asyncio.wait_for(
            _send_tcp_command_impl(command_type, params or {}),
            timeout=timeout,
        )
    except asyncio.TimeoutError:
        return {
            "success": False,
            "error": f"TCP communication timed out after {timeout} seconds",
        }
    except Exception as e:
        return {"success": False, "error": f"TCP communication error: {str(e)}"}
