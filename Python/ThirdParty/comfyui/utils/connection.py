"""
HTTP client for ComfyUI REST API.

Handles all communication with the ComfyUI server.
"""

import logging
from typing import Any, Dict, Optional

import httpx

from .config import config

logger = logging.getLogger("ComfyUI-MCP")


class ComfyUIConnection:
    """Async HTTP client for ComfyUI API."""

    def __init__(self, base_url: str = None, timeout: float = 30.0):
        """
        Initialize the ComfyUI connection.

        Args:
            base_url: ComfyUI server URL (default: from .env config)
            timeout: Request timeout in seconds
        """
        self.base_url = (base_url or config.base_url).rstrip("/")
        self.timeout = timeout
        self._object_info_cache: Optional[Dict[str, Any]] = None

    async def _request(
        self,
        method: str,
        endpoint: str,
        json_data: Dict[str, Any] = None,
        params: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Make an HTTP request to ComfyUI.

        Args:
            method: HTTP method (GET, POST, etc.)
            endpoint: API endpoint (e.g., "/object_info")
            json_data: JSON body for POST requests
            params: Query parameters

        Returns:
            Response JSON as dictionary
        """
        url = f"{self.base_url}{endpoint}"

        try:
            async with httpx.AsyncClient(timeout=self.timeout) as client:
                response = await client.request(
                    method=method,
                    url=url,
                    json=json_data,
                    params=params
                )
                response.raise_for_status()
                return response.json()
        except httpx.ConnectError:
            logger.error(f"Failed to connect to ComfyUI at {self.base_url}")
            return {
                "success": False,
                "error": f"Cannot connect to ComfyUI at {self.base_url}. Is ComfyUI running?"
            }
        except httpx.TimeoutException:
            logger.error(f"Request to {url} timed out")
            return {
                "success": False,
                "error": f"Request timed out after {self.timeout}s"
            }
        except httpx.HTTPStatusError as e:
            logger.error(f"HTTP error {e.response.status_code}: {e.response.text}")
            return {
                "success": False,
                "error": f"HTTP {e.response.status_code}: {e.response.text}"
            }
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            return {
                "success": False,
                "error": str(e)
            }

    async def get_object_info(self, use_cache: bool = True) -> Dict[str, Any]:
        """
        Get all available node definitions from ComfyUI.

        This returns the complete node library including:
        - Node inputs (names, types, required/optional, default values)
        - Node outputs (names, types)
        - Node categories
        - Node descriptions

        Args:
            use_cache: Whether to use cached data if available

        Returns:
            Dictionary mapping node class names to their definitions
        """
        if use_cache and self._object_info_cache is not None:
            return self._object_info_cache

        result = await self._request("GET", "/object_info")

        if "success" not in result or result.get("success") is not False:
            # Cache successful response
            self._object_info_cache = result

        return result

    def clear_cache(self):
        """Clear the object_info cache."""
        self._object_info_cache = None

    async def queue_prompt(self, prompt: Dict[str, Any], client_id: str = None) -> Dict[str, Any]:
        """
        Queue a workflow (prompt) for execution.

        Args:
            prompt: The workflow JSON structure
            client_id: Optional client ID for WebSocket tracking

        Returns:
            Response containing prompt_id for tracking
        """
        data = {"prompt": prompt}
        if client_id:
            data["client_id"] = client_id

        return await self._request("POST", "/prompt", json_data=data)

    async def get_history(self, prompt_id: str = None, max_items: int = None) -> Dict[str, Any]:
        """
        Get execution history.

        Args:
            prompt_id: Specific prompt ID to get history for
            max_items: Maximum number of history items to return

        Returns:
            Dictionary of prompt execution results
        """
        if prompt_id:
            return await self._request("GET", f"/history/{prompt_id}")

        params = {}
        if max_items:
            params["max_items"] = max_items

        return await self._request("GET", "/history", params=params)

    async def get_queue(self) -> Dict[str, Any]:
        """
        Get the current execution queue status.

        Returns:
            Dictionary with 'queue_running' and 'queue_pending' lists
        """
        return await self._request("GET", "/queue")

    async def interrupt(self) -> Dict[str, Any]:
        """
        Interrupt the currently executing workflow.

        Returns:
            Empty dict on success
        """
        return await self._request("POST", "/interrupt")

    async def get_system_stats(self) -> Dict[str, Any]:
        """
        Get system statistics (GPU memory, etc.).

        Returns:
            System stats dictionary
        """
        return await self._request("GET", "/system_stats")

    async def is_connected(self) -> bool:
        """
        Check if ComfyUI server is reachable.

        Returns:
            True if connected, False otherwise
        """
        result = await self.get_system_stats()
        return result.get("success") is not False


# Global connection instance
_connection: Optional[ComfyUIConnection] = None


def get_connection() -> ComfyUIConnection:
    """Get or create the global ComfyUI connection."""
    global _connection
    if _connection is None:
        _connection = ComfyUIConnection()
    return _connection
