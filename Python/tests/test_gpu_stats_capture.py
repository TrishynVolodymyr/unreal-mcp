from pathlib import Path
import sys
import unittest
from unittest.mock import patch


PYTHON_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_ROOT))

from editor_tools import editor_tools
from editor_tools import runtime_tools
from utils.mcp_help import get_help_registry


class FakeMCP:
    def __init__(self):
        self.tools = {}

    def tool(self):
        def decorate(function):
            self.tools[function.__name__] = function
            return function

        return decorate


def get_registered_gpu_tool():
    mcp = FakeMCP()
    editor_tools.register_editor_tools(mcp)
    return mcp.tools["get_gpu_stats"]


class GPUStatsCaptureTests(unittest.TestCase):
    def test_runtime_split_registers_every_moved_tool(self):
        mcp = FakeMCP()
        editor_tools.register_editor_tools(mcp)
        expected = {
            "get_performance_stats",
            "execute_console_command",
            "set_viewport_camera",
            "get_gpu_stats",
            "get_scene_breakdown",
            "get_rendering_stats",
            "get_mesh_draw_stats",
            "start_pie",
            "stop_pie",
        }
        self.assertTrue(expected.issubset(mcp.tools.keys()))
        help_registry = get_help_registry()
        self.assertTrue(expected.issubset(help_registry.get_tool_names()))
        self.assertTrue(expected.issubset(help_registry.get_tools_by_category()["profiling"]))
        for name in expected:
            self.assertGreater(len(mcp.tools[name].__doc__ or ""), 80, name)
            self.assertTrue(help_registry.get_help(name)["description"], name)

    def test_one_call_runs_bounded_begin_wait_read_sequence(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(runtime_tools, "send_unreal_command") as send,
            patch.object(runtime_tools.time, "sleep") as sleep,
        ):
            send.side_effect = [
                {"success": True, "capture_started": True, "capture_owner": "mcp"},
                {"success": True, "detailed_available": True, "pass_count": 2},
                {"success": True, "capture_cancelled": True},
            ]

            result = tool(None, capture_seconds=0.25, timeout_seconds=2.0)

        self.assertEqual(result["pass_count"], 2)
        self.assertEqual(
            send.call_args_list[0].args,
            ("get_gpu_stats", {"action": "begin", "timeout_seconds": 2.75}),
        )
        sleep.assert_called_once_with(0.25)
        self.assertEqual(
            send.call_args_list[1].args,
            ("get_gpu_stats", {"action": "read", "finish": False}),
        )
        self.assertEqual(
            send.call_args_list[2].args,
            ("get_gpu_stats", {"action": "cancel"}),
        )

    def test_read_exception_attempts_cancel_and_preserves_original_error(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(runtime_tools, "send_unreal_command") as send,
            patch.object(runtime_tools.time, "sleep"),
        ):
            send.side_effect = [
                {"success": True, "capture_started": True, "capture_owner": "mcp"},
                RuntimeError("read failed"),
                {"success": True, "capture_cancelled": True},
            ]

            with self.assertRaisesRegex(RuntimeError, "read failed"):
                tool(None)

            self.assertEqual(
                send.call_args_list[-1].args,
                ("get_gpu_stats", {"action": "cancel"}),
            )

    def test_watchdog_covers_capture_and_full_retry_budget(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(runtime_tools, "send_unreal_command") as send,
            patch.object(runtime_tools.time, "sleep"),
        ):
            send.side_effect = [
                {"success": True, "capture_started": True},
                *[{"success": True, "detailed_available": False}] * 4,
                {"success": True, "capture_cancelled": True},
            ]
            result = tool(None, capture_seconds=0.1, timeout_seconds=0.1)

        self.assertFalse(result["detailed_available"])
        self.assertEqual(
            send.call_args_list[0].args,
            ("get_gpu_stats", {"action": "begin", "timeout_seconds": 2.6}),
        )
        self.assertEqual(
            send.call_args_list[-1].args,
            ("get_gpu_stats", {"action": "cancel"}),
        )
        self.assertEqual(send.call_count, 6)

    def test_begin_failure_does_not_cancel_unowned_capture(self):
        tool = get_registered_gpu_tool()
        with patch.object(runtime_tools, "send_unreal_command") as send:
            send.return_value = {"success": False, "error": "no viewport"}
            result = tool(None)

        self.assertFalse(result["success"])
        send.assert_called_once()

    def test_negative_capture_is_rejected_before_begin(self):
        tool = get_registered_gpu_tool()
        with patch.object(runtime_tools, "send_unreal_command") as send:
            result = tool(None, capture_seconds=-0.1)

        self.assertFalse(result["success"])
        self.assertIn("non-negative", result["error"])
        send.assert_not_called()

    def test_capture_budget_above_watchdog_limit_is_rejected_before_begin(self):
        tool = get_registered_gpu_tool()
        with patch.object(runtime_tools, "send_unreal_command") as send:
            result = tool(None, capture_seconds=29.0, timeout_seconds=1.0)

        self.assertFalse(result["success"])
        self.assertIn("30 second", result["error"])
        send.assert_not_called()

    def test_rendering_stats_is_an_unintrusive_snapshot(self):
        mcp = FakeMCP()
        editor_tools.register_editor_tools(mcp)
        tool = mcp.tools["get_rendering_stats"]
        with patch.object(runtime_tools, "send_unreal_command") as send:
            send.return_value = {
                "success": True,
                "visibility": {"detailed_available": False},
            }
            result = tool(None)

        self.assertFalse(result["visibility"]["detailed_available"])
        send.assert_called_once_with("get_rendering_stats", {"action": "snapshot"})

    def test_not_ready_capture_retries_before_cleanup(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(runtime_tools, "send_unreal_command") as send,
            patch.object(runtime_tools.time, "sleep") as sleep,
        ):
            send.side_effect = [
                {"success": True, "capture_started": True, "capture_owner": "mcp"},
                {"success": False, "detailed_available": False, "error": "not ready"},
                {"success": True, "detailed_available": True, "pass_count": 1},
                {"success": True, "capture_cancelled": True},
            ]

            result = tool(None, capture_seconds=0.1, timeout_seconds=3.0)

        self.assertEqual(result["pass_count"], 1)
        self.assertEqual(sleep.call_args_list[0].args, (0.1,))
        self.assertEqual(sleep.call_args_list[1].args, (0.5,))
        self.assertEqual(
            send.call_args_list[-1].args,
            ("get_gpu_stats", {"action": "cancel"}),
        )

    def test_real_tcp_envelope_is_unwrapped_for_control_flow(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(runtime_tools, "send_unreal_command") as send,
            patch.object(runtime_tools.time, "sleep"),
        ):
            send.side_effect = [
                {"status": "success", "result": {"success": True, "capture_started": True}},
                {"status": "success", "result": {"success": True, "detailed_available": True, "pass_count": 3}},
                {"status": "success", "result": {"success": True, "capture_cancelled": True}},
            ]

            result = tool(None)

        self.assertEqual(result["result"]["pass_count"], 3)
        self.assertEqual(len(send.call_args_list), 3)
        self.assertEqual(send.call_args_list[0].args[1]["timeout_seconds"], 5.0)

if __name__ == "__main__":
    unittest.main()
