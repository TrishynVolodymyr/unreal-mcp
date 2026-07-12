from pathlib import Path
import sys
import unittest
from unittest.mock import patch


PYTHON_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_ROOT))

from editor_tools import editor_tools


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
    def test_one_call_runs_bounded_begin_wait_read_sequence(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(editor_tools, "send_unreal_command") as send,
            patch.object(editor_tools.time, "sleep") as sleep,
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
            ("get_gpu_stats", {"action": "begin", "timeout_seconds": 2.0}),
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
            patch.object(editor_tools, "send_unreal_command") as send,
            patch.object(editor_tools.time, "sleep"),
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

    def test_rendering_stats_uses_same_bounded_capture_contract(self):
        mcp = FakeMCP()
        editor_tools.register_editor_tools(mcp)
        tool = mcp.tools["get_rendering_stats"]
        with (
            patch.object(editor_tools, "send_unreal_command") as send,
            patch.object(editor_tools.time, "sleep") as sleep,
        ):
            send.side_effect = [
                {"success": True, "capture_started": True, "capture_owner": "mcp"},
                {"success": True, "visibility": {"detailed_available": True}},
                {"success": True, "capture_cancelled": True},
            ]

            result = tool(None, capture_seconds=0.2, timeout_seconds=2.0)

        self.assertTrue(result["visibility"]["detailed_available"])
        self.assertEqual(
            send.call_args_list[0].args,
            ("get_rendering_stats", {"action": "begin", "timeout_seconds": 2.0}),
        )
        sleep.assert_called_once_with(0.2)
        self.assertEqual(
            send.call_args_list[1].args,
            ("get_rendering_stats", {"action": "read", "finish": False}),
        )
        self.assertEqual(
            send.call_args_list[2].args,
            ("get_rendering_stats", {"action": "cancel"}),
        )

    def test_not_ready_capture_retries_before_cleanup(self):
        tool = get_registered_gpu_tool()
        with (
            patch.object(editor_tools, "send_unreal_command") as send,
            patch.object(editor_tools.time, "sleep") as sleep,
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
            patch.object(editor_tools, "send_unreal_command") as send,
            patch.object(editor_tools.time, "sleep"),
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

    def test_rendering_stats_waits_for_visibility_payload(self):
        mcp = FakeMCP()
        editor_tools.register_editor_tools(mcp)
        tool = mcp.tools["get_rendering_stats"]
        with (
            patch.object(editor_tools, "send_unreal_command") as send,
            patch.object(editor_tools.time, "sleep") as sleep,
        ):
            send.side_effect = [
                {"success": True, "capture_started": True},
                {"success": True, "visibility": {"detailed_available": False}},
                {"success": True, "visibility": {"detailed_available": True}},
                {"success": True, "capture_cancelled": True},
            ]

            result = tool(None, capture_seconds=0.1)

        self.assertTrue(result["visibility"]["detailed_available"])
        self.assertEqual(sleep.call_args_list[1].args, (0.5,))


if __name__ == "__main__":
    unittest.main()
