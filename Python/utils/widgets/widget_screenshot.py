"""
Widget Screenshot Utilities for Unreal MCP.

This module provides utilities for capturing screenshots of UMG Widget Blueprints.
"""

import logging
from typing import Any, Dict
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

logger = logging.getLogger("UnrealMCP")


def capture_widget_screenshot_impl(
    ctx: Context,
    widget_name: str,
    width: int = 800,
    height: int = 600,
    format: str = "png"
) -> Dict[str, Any]:
    """
    Capture a screenshot of a UMG Widget Blueprint preview.

    This renders the widget to an image and returns it as base64-encoded data.
    The AI can view this image to see exactly how the widget looks.

    Args:
        widget_name: Name of the widget blueprint to capture
        width: Screenshot width in pixels (default: 800, max: 8192)
        height: Screenshot height in pixels (default: 600, max: 8192)
        format: Image format - "png" or "jpg" (default: "png")

    Returns:
        Dict containing:
        - success: bool - Whether the screenshot was captured successfully
        - image_base64: str - Base64-encoded image data (if successful)
        - width: int - Actual image width
        - height: int - Actual image height
        - format: str - Image format used
        - image_size_bytes: int - Size of the compressed image in bytes
        - message: str - Success or error message

    Raises:
        None - Errors are returned in the response dict

    Example:
        >>> result = capture_widget_screenshot_impl(ctx, "MyWidget", 1024, 768)
        >>> if result['success']:
        ...     image_data = result['image_base64']
        ...     # AI can now view this image
        ...     print(f"Screenshot size: {result['image_size_bytes']} bytes")
    """
    logger.info(f"Capturing screenshot for widget '{widget_name}' at {width}x{height}")

    # Validate parameters
    if not widget_name:
        return {
            "success": False,
            "error": "widget_name cannot be empty",
            "message": "Failed to capture screenshot: widget_name is required"
        }

    if width <= 0 or width > 8192:
        return {
            "success": False,
            "error": f"Invalid width: {width}. Must be between 1 and 8192",
            "message": f"Failed to capture screenshot: invalid width {width}"
        }

    if height <= 0 or height > 8192:
        return {
            "success": False,
            "error": f"Invalid height: {height}. Must be between 1 and 8192",
            "message": f"Failed to capture screenshot: invalid height {height}"
        }

    valid_formats = ["png", "jpg", "jpeg"]
    format_lower = format.lower()
    if format_lower not in valid_formats:
        return {
            "success": False,
            "error": f"Invalid format: {format}. Must be one of: {', '.join(valid_formats)}",
            "message": f"Failed to capture screenshot: unsupported format '{format}'"
        }

    try:
        # Send command to Unreal Engine
        response = send_unreal_command("capture_widget_screenshot", {
            "widget_name": widget_name,
            "width": width,
            "height": height,
            "format": format_lower
        })

        # Log result
        if response.get("success"):
            logger.info(f"Screenshot captured successfully for '{widget_name}', "
                       f"{response.get('image_size_bytes', 0)} bytes")
        else:
            logger.warning(f"Screenshot capture failed for '{widget_name}': "
                         f"{response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        error_msg = f"Exception during screenshot capture: {str(e)}"
        logger.error(error_msg)
        return {
            "success": False,
            "error": str(e),
            "message": f"Failed to capture widget screenshot: {str(e)}"
        }
