from pathlib import Path
import sys
import unittest
from unittest.mock import patch


PYTHON_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_ROOT))

from editor_tools import editor_tools
from utils.editor import editor_operations


class FakeMCP:
    def __init__(self):
        self.tools = {}

    def tool(self):
        def decorate(function):
            self.tools[function.__name__] = function
            return function

        return decorate


def get_registered_import_tool():
    mcp = FakeMCP()
    editor_tools.register_editor_tools(mcp)
    return mcp.tools["import_static_mesh"]


class ImportStaticMeshOptionsTests(unittest.TestCase):
    def test_forwards_vertex_color_and_collision(self):
        tool = get_registered_import_tool()
        with patch.object(editor_operations, "send_unreal_command") as send:
            send.return_value = {"success": True}
            result = tool(
                None,
                "E:/mesh.fbx",
                "SM_Test",
                "/Game/Test",
                False,
                auto_generate_collision=False,
                vertex_color_import_option="Override",
                vertex_override_color=[0.2, 0.4, 0.6, 0.8],
            )

        self.assertEqual(result, {"success": True})
        command, params = send.call_args.args
        self.assertEqual(command, "import_static_mesh")
        self.assertIs(params["auto_generate_collision"], False)
        self.assertEqual(params["vertex_color_import_option"], "Override")
        self.assertEqual(params["vertex_override_color"], [0.2, 0.4, 0.6, 0.8])

    def test_preserves_legacy_defaults(self):
        tool = get_registered_import_tool()
        with patch.object(editor_operations, "send_unreal_command") as send:
            send.return_value = {"success": True}
            tool(None, "E:/mesh.fbx", "SM_Test")

        _, params = send.call_args.args
        self.assertIs(params["auto_generate_collision"], True)
        self.assertEqual(params["vertex_color_import_option"], "Replace")


if __name__ == "__main__":
    unittest.main()
