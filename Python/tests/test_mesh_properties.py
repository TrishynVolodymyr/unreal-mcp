from pathlib import Path
import sys
import unittest
from unittest.mock import AsyncMock, patch


PYTHON_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_ROOT))

import mesh_mcp_server


class SetStaticMeshPropertiesTests(unittest.IsolatedAsyncioTestCase):
    async def test_forwards_complete_material_slot_pair(self):
        with patch.object(
            mesh_mcp_server,
            "send_tcp_command",
            new_callable=AsyncMock,
        ) as send:
            send.return_value = {"success": True}
            result = await mesh_mcp_server.set_static_mesh_properties(
                mesh_path="/Game/Meshes/SM_Test",
                material_slot_index=2,
                material_slot_path="/Game/Materials/M_Test",
            )

        self.assertEqual(result, {"success": True})
        send.assert_awaited_once_with(
            "set_static_mesh_properties",
            {
                "mesh_path": "/Game/Meshes/SM_Test",
                "material_slot_index": 2,
                "material_slot_path": "/Game/Materials/M_Test",
            },
        )

    async def test_rejects_material_slot_index_without_path(self):
        with patch.object(
            mesh_mcp_server,
            "send_tcp_command",
            new_callable=AsyncMock,
        ) as send:
            with self.assertRaisesRegex(ValueError, "must be provided together"):
                await mesh_mcp_server.set_static_mesh_properties(
                    mesh_path="/Game/Meshes/SM_Test",
                    material_slot_index=0,
                )

        send.assert_not_awaited()

    async def test_rejects_material_slot_path_without_index(self):
        with patch.object(
            mesh_mcp_server,
            "send_tcp_command",
            new_callable=AsyncMock,
        ) as send:
            with self.assertRaisesRegex(ValueError, "must be provided together"):
                await mesh_mcp_server.set_static_mesh_properties(
                    mesh_path="/Game/Meshes/SM_Test",
                    material_slot_path="/Game/Materials/M_Test",
                )

        send.assert_not_awaited()


if __name__ == "__main__":
    unittest.main()
