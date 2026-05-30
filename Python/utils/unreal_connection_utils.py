"""
Utilities for working with Unreal Engine connections.

This module provides helper functions for common operations with Unreal Engine connections.
"""

import logging
import os
import socket
import json
from typing import Dict, Any, Optional

# Get logger
logger = logging.getLogger("UnrealMCP")

# Configuration
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557
DEFAULT_COMMAND_TIMEOUT = 130.0
MAX_RESPONSE_BYTES = 4 * 1024 * 1024
RECV_CHUNK_SIZE = 8192
CONNECT_TIMEOUT = 8.0


def _tcp_debug_enabled() -> bool:
    return os.environ.get("UNREAL_MCP_TCP_DEBUG", "").strip() == "1"


def _write_tcp_debug(message: str) -> None:
    if not _tcp_debug_enabled():
        return

    debug_log_path = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "tcp_debug.log",
    )
    with open(debug_log_path, "a", encoding="utf-8") as f:
        f.write(message)


class UnrealConnection:
    """Connection to an Unreal Engine instance."""

    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False

    def connect(self) -> bool:
        """Connect to the Unreal Engine instance."""
        try:
            if self.socket:
                try:
                    self.socket.close()
                except Exception:
                    pass
                self.socket = None

            logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(CONNECT_TIMEOUT)

            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)

            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            logger.info("Connected to Unreal Engine")
            return True

        except socket.timeout:
            logger.error(f"Connection timeout to Unreal Engine at {UNREAL_HOST}:{UNREAL_PORT}")
            self.connected = False
            return False
        except ConnectionRefusedError:
            logger.error(
                f"Connection refused by Unreal Engine at {UNREAL_HOST}:{UNREAL_PORT} - is Unreal Engine running?"
            )
            self.connected = False
            return False
        except Exception as e:
            logger.error(f"Failed to connect to Unreal: {e}")
            self.connected = False
            return False

    def disconnect(self):
        """Disconnect from the Unreal Engine instance."""
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass
        self.socket = None
        self.connected = False

    def receive_full_response(self, sock, buffer_size=RECV_CHUNK_SIZE) -> bytes:
        """Receive a complete response from Unreal, handling chunked data."""
        chunks = []
        total_bytes = 0
        sock.settimeout(DEFAULT_COMMAND_TIMEOUT)

        _write_tcp_debug(f"\n=== RECEIVE START ===\n")

        try:
            while total_bytes < MAX_RESPONSE_BYTES:
                _write_tcp_debug(f"Calling sock.recv({buffer_size})...\n")
                chunk = sock.recv(buffer_size)
                _write_tcp_debug(f"Received chunk: {len(chunk) if chunk else 0} bytes\n")

                if not chunk:
                    if not chunks:
                        raise Exception("Connection closed before receiving data")
                    break

                chunks.append(chunk)
                total_bytes += len(chunk)
                data = b"".join(chunks)
                decoded_data = data.decode("utf-8")
                _write_tcp_debug(f"Total data so far: {len(data)} bytes\n")

                try:
                    json.loads(decoded_data)
                    logger.info(f"Received complete response ({len(data)} bytes)")
                    _write_tcp_debug("SUCCESS: Complete JSON received\n")
                    return data
                except json.JSONDecodeError:
                    logger.debug("Received partial response, waiting for more data...")
                    continue

            if total_bytes >= MAX_RESPONSE_BYTES:
                raise Exception(f"Response exceeds maximum size of {MAX_RESPONSE_BYTES} bytes")

            data = b"".join(chunks)
            json.loads(data.decode("utf-8"))
            return data

        except socket.timeout:
            logger.warning(f"Socket timeout during receive after {len(chunks)} chunks")
            raise Exception(
                f"Timeout receiving Unreal response after {DEFAULT_COMMAND_TIMEOUT} seconds "
                f"(received {len(chunks)} chunks)"
            )
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            _write_tcp_debug(f"FATAL ERROR: {e}\n")
            raise

    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        """Send a command to Unreal Engine and get the response."""
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass
            self.socket = None
            self.connected = False

        if not self.connect():
            logger.error("Failed to connect to Unreal Engine for command")
            return None

        try:
            command_obj = {
                "type": command,
                "params": params or {},
            }

            command_json = json.dumps(command_obj)
            logger.info(f"Sending command: {command_json}")
            self.socket.sendall(command_json.encode("utf-8"))

            response_data = self.receive_full_response(self.socket)
            response = json.loads(response_data.decode("utf-8"))
            logger.info(f"Complete response from Unreal: {response}")

            try:
                self.socket.close()
            except Exception:
                pass
            self.socket = None
            self.connected = False

            return response

        except Exception as e:
            logger.error(f"Error sending command: {e}")
            self.connected = False
            try:
                self.socket.close()
            except Exception:
                pass
            self.socket = None
            return {
                "status": "error",
                "error": str(e),
            }


# Global connection state
_unreal_connection: UnrealConnection = None


def get_unreal_engine_connection():
    """Get a connection to Unreal Engine."""
    global _unreal_connection
    try:
        if _unreal_connection is None:
            _unreal_connection = UnrealConnection()
        return _unreal_connection
    except Exception as e:
        logger.error(f"Error getting Unreal connection: {e}")
        return None


def send_unreal_command(command_name: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to Unreal Engine with proper error handling."""
    try:
        unreal = get_unreal_engine_connection()
        if not unreal:
            return {"status": "error", "error": "Failed to connect to Unreal Engine"}

        logger.info(f"Sending command '{command_name}' with params: {params}")
        response = unreal.send_command(command_name, params)

        if not response:
            logger.error(f"No response from Unreal Engine for command '{command_name}'")
            return {"status": "error", "error": "No response from Unreal Engine"}

        logger.info(f"Command '{command_name}' response: {response}")

        if response.get("success") is False:
            error_field = response.get("error")
            error_message = "Unknown Unreal error"

            if isinstance(error_field, dict):
                error_message = (
                    error_field.get("errorMessage")
                    or error_field.get("errorDetails")
                    or error_field.get("message")
                    or "Unknown nested error"
                )
                logger.error(f"Unreal nested error: {error_message}")
            elif isinstance(error_field, str):
                error_message = error_field
                logger.error(f"Unreal string error: {error_message}")
            else:
                error_message = response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal fallback error: {error_message}")

            return {
                "status": "error",
                "error": error_message,
            }

        return response

    except Exception as e:
        error_msg = f"Error executing Unreal command '{command_name}': {e}"
        logger.error(error_msg)
        return {"status": "error", "error": error_msg}


# Cache for project info to avoid repeated TCP calls
_project_info_cache: Dict[str, Any] = {}


def get_project_module_name() -> str:
    """
    Get the current Unreal project's module name dynamically.

    Returns:
        The project module name, or "MyGame" as fallback if unable to query.
    """
    global _project_info_cache

    if "module_name" in _project_info_cache:
        return _project_info_cache["module_name"]

    try:
        response = send_unreal_command("get_project_dir", {})
        if response and response.get("success") and response.get("project_name"):
            module_name = response["project_name"]
            _project_info_cache["module_name"] = module_name
            _project_info_cache["module_path"] = response.get("module_path", f"/Script/{module_name}")
            logger.info(f"Retrieved project module name: {module_name}")
            return module_name
    except Exception as e:
        logger.warning(f"Failed to get project module name from Unreal: {e}")

    logger.warning("Using fallback module name 'MyGame'")
    return "MyGame"


def get_project_module_path() -> str:
    """
    Get the full script module path for the current project.

    Returns:
        The module path (e.g., "/Script/MyProjectName"), or fallback if unable to query.
    """
    global _project_info_cache

    if "module_path" in _project_info_cache:
        return _project_info_cache["module_path"]

    module_name = get_project_module_name()
    return _project_info_cache.get("module_path", f"/Script/{module_name}")
