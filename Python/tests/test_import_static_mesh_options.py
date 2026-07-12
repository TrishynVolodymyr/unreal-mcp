from pathlib import Path
import sys
import unittest
from unittest.mock import patch


PYTHON_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_ROOT))

from utils.editor import editor_operations


class ImportStaticMeshOptionsTests(unittest.TestCase):
    def test_forwards_vertex_color_and_collision(self):
        with patch.object(editor_operations, "send_unreal_command") as send:
            send.return_value = {"success": True}
            result = editor_operations.import_static_mesh(
                None,
                "E:/mesh.fbx",
                "SM_Test",
                "/Game/Test",
                False,
                auto_generate_collision=False,
                vertex_color_import_option="Replace",
            )

        self.assertEqual(result, {"success": True})
        command, params = send.call_args.args
        self.assertEqual(command, "import_static_mesh")
        self.assertIs(params["auto_generate_collision"], False)
        self.assertEqual(params["vertex_color_import_option"], "Replace")

    def test_preserves_legacy_defaults(self):
        with patch.object(editor_operations, "send_unreal_command") as send:
            send.return_value = {"success": True}
            editor_operations.import_static_mesh(None, "E:/mesh.fbx", "SM_Test")

        _, params = send.call_args.args
        self.assertIs(params["auto_generate_collision"], True)
        self.assertEqual(params["vertex_color_import_option"], "Replace")


if __name__ == "__main__":
    unittest.main()
