"""Runtime, PIE, and profiling tools split from the core editor tool registry."""

import logging
import time
from typing import Any, Dict

from mcp.server.fastmcp import Context, FastMCP

from utils.mcp_help import get_help_registry
from utils.unreal_connection_utils import send_unreal_command


logger = logging.getLogger("UnrealMCP")
_help_registry = get_help_registry()


def _response_payload(response: Dict[str, Any]) -> Dict[str, Any]:
    result = response.get("result")
    return result if isinstance(result, dict) else response


def _capture_payload_ready(command: str, payload: Dict[str, Any]) -> bool:
    if command == "get_rendering_stats":
        visibility = payload.get("visibility")
        return isinstance(visibility, dict) and visibility.get("detailed_available", False)
    return payload.get("success", False) and payload.get("detailed_available", False)


def _run_bounded_stat_capture(
    command: str,
    capture_seconds: float,
    timeout_seconds: float,
) -> Dict[str, Any]:
    retry_interval_seconds = 0.5
    max_read_attempts = 4
    if capture_seconds < 0:
        return {"success": False, "error": "capture_seconds must be non-negative"}

    retry_wait_budget = retry_interval_seconds * (max_read_attempts - 1)
    transport_margin_seconds = 1.0
    effective_timeout = max(
        timeout_seconds,
        capture_seconds + retry_wait_budget + transport_margin_seconds,
    )
    if effective_timeout > 30.0:
        return {
            "success": False,
            "error": "capture and retry budget must fit within the 30 second editor watchdog",
        }

    begin = send_unreal_command(
        command,
        {"action": "begin", "timeout_seconds": effective_timeout},
    )
    if not _response_payload(begin).get("success", False):
        return begin

    try:
        time.sleep(capture_seconds)
        last_result = None
        for attempt in range(max_read_attempts):
            last_result = send_unreal_command(command, {"action": "read", "finish": False})
            if _capture_payload_ready(command, _response_payload(last_result)):
                send_unreal_command(command, {"action": "cancel"})
                return last_result
            if attempt + 1 < max_read_attempts:
                time.sleep(retry_interval_seconds)

        send_unreal_command(command, {"action": "cancel"})
        return last_result
    except Exception:
        try:
            send_unreal_command(command, {"action": "cancel"})
        except Exception:
            logger.warning("Failed to cancel %s capture after read error", command, exc_info=True)
        raise


def register_runtime_tools(mcp: FastMCP):
    """Register runtime control and profiling tools."""

    @mcp.tool()
    def get_performance_stats(ctx: Context) -> Dict[str, Any]:
        """Get real-time Unreal performance statistics.

        Returns current and smoothed FPS, frame/GPU time, draw calls,
        primitives drawn, and physical-memory usage. Use this lightweight,
        non-mutating snapshot before requesting detailed GPU profiling.
        """
        return send_unreal_command("get_performance_stats", {})

    @mcp.tool()
    def execute_console_command(ctx: Context, command: str) -> Dict[str, Any]:
        """Execute an Unreal console command and return captured output.

        Args:
            command: Command such as ``stat fps``, ``stat unit``,
                ``r.ScreenPercentage 50``, or ``t.MaxFPS 60``.

        Toggle commands may legitimately return an empty output string.
        """
        return send_unreal_command("execute_console_command", {"command": command})

    @mcp.tool()
    def set_viewport_camera(
        ctx: Context,
        location: list = None,
        look_at: list = None,
        rotation: list = None,
        fov: float = None,
    ) -> Dict[str, Any]:
        """Set the editor perspective viewport camera.

        Args:
            location: Optional ``[X, Y, Z]`` world position.
            look_at: Optional target point; overrides ``rotation``.
            rotation: Optional ``[Pitch, Yaw, Roll]`` when look-at is absent.
            fov: Optional horizontal field of view in degrees.

        This supports repeatable framing and view-dependent visual checks.
        """
        params = {}
        if location is not None:
            params["location"] = location
        if look_at is not None:
            params["look_at"] = look_at
        if rotation is not None:
            params["rotation"] = rotation
        if fov is not None:
            params["fov"] = fov
        return send_unreal_command("set_viewport_camera", params)

    @mcp.tool()
    def get_gpu_stats(
        ctx: Context,
        capture_seconds: float = 0.5,
        timeout_seconds: float = 5.0,
    ) -> Dict[str, Any]:
        """Capture bounded per-pass GPU timings.

        Temporarily enables detailed GPU collection when necessary, waits for
        samples, and always releases MCP-owned state. A profiler session that
        was active before the call remains externally owned and enabled.

        Args:
            capture_seconds: Initial sample accumulation window.
            timeout_seconds: Requested editor watchdog; automatically enlarged
                to cover the capture and retry budget, up to 30 seconds.

        Returns total GPU time and sorted pass samples with busy/wait/idle
        timing. Use this after the lightweight performance snapshot identifies
        a likely GPU bottleneck.
        """
        return _run_bounded_stat_capture("get_gpu_stats", capture_seconds, timeout_seconds)

    @mcp.tool()
    def get_scene_breakdown(
        ctx: Context,
        mesh_filter: str = "",
        max_results: int = 50,
    ) -> Dict[str, Any]:
        """Return a per-mesh scene rendering-cost breakdown.

        Args:
            mesh_filter: Optional case-insensitive mesh-name substring.
            max_results: Maximum meshes returned, sorted by total LOD0 cost.

        Results include instance/component counts, LOD0 triangles, aggregate
        triangle cost, shadow casters, Nanite state, and instancing state.
        """
        params = {}
        if mesh_filter:
            params["mesh_filter"] = mesh_filter
        if max_results != 50:
            params["max_results"] = max_results
        return send_unreal_command("get_scene_breakdown", params)

    @mcp.tool()
    def get_rendering_stats(
        ctx: Context,
    ) -> Dict[str, Any]:
        """Get a non-mutating rendering snapshot.

        Returns draw calls, primitives, GPU frame time, VRAM fields, and the
        visibility capability contract. In UE 5.8 fresh InitViews aggregates
        are not available through this editor snapshot, so
        ``visibility.detailed_available`` is always false; use Unreal Insights
        for visibility/culling profiling.
        """
        return send_unreal_command("get_rendering_stats", {"action": "snapshot"})

    @mcp.tool()
    def get_mesh_draw_stats(ctx: Context, mesh_filter: str = None) -> Dict[str, Any]:
        """Return per-resource mesh draw counts by render pass.

        Args:
            mesh_filter: Optional case-insensitive resource-name substring.

        Entries include pass, resource, material, visible primitives/vertices/
        instances, LOD index, and aggregate totals. Collection may require a
        rendered frame before data becomes available.
        """
        params = {}
        if mesh_filter:
            params["mesh_filter"] = mesh_filter
        return send_unreal_command("get_mesh_draw_stats", params)

    @mcp.tool()
    def start_pie(ctx: Context) -> Dict[str, Any]:
        """Queue a Play In Editor session.

        PIE starts asynchronously. The result reports whether the request was
        accepted or a session was already running.
        """
        return send_unreal_command("start_pie", {})

    @mcp.tool()
    def stop_pie(ctx: Context) -> Dict[str, Any]:
        """Stop the active Play In Editor session.

        The result reports whether the request was accepted or PIE was already
        stopped.
        """
        return send_unreal_command("stop_pie", {})

    for tool in (
        get_performance_stats,
        execute_console_command,
        set_viewport_camera,
        get_gpu_stats,
        get_scene_breakdown,
        get_rendering_stats,
        get_mesh_draw_stats,
        start_pie,
        stop_pie,
    ):
        _help_registry.register(tool, category="profiling")
