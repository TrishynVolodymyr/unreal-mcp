from pathlib import Path
import asyncio
import sys
import unittest
from unittest.mock import AsyncMock, patch


PYTHON_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_ROOT))

import material_mcp_server


class BatchMaterialParamsTests(unittest.TestCase):
    def test_create_instance_forwards_parameter_maps_as_json_objects(self):
        async def run_test():
            with patch.object(material_mcp_server, "send_tcp_command", new_callable=AsyncMock) as send:
                send.return_value = {"success": True}
                await material_mcp_server.create_material_instance(
                    name="MI_Test",
                    parent_material="/Game/Test/M_Parent",
                    scalar_params={"WindStrength": 0.03},
                    vector_params={"WindDirection": [1.0, 0.0, 0.0, 0.0]},
                )

            command, params = send.await_args.args
            self.assertEqual(command, "create_material_instance")
            self.assertIsInstance(params["scalar_params"], dict)
            self.assertIsInstance(params["vector_params"], dict)

        asyncio.run(run_test())

    def test_forwards_parameter_maps_as_json_objects(self):
        async def run_test():
            with patch.object(material_mcp_server, "send_tcp_command", new_callable=AsyncMock) as send:
                send.return_value = {"success": True}
                await material_mcp_server.batch_set_material_params(
                    material_instance="/Game/Test/MI_Test.MI_Test",
                    scalar_params={"WindStrength": 0.03},
                    vector_params={"WindDirection": [1.0, 0.0, 0.0, 0.0]},
                )

            command, params = send.await_args.args
            self.assertEqual(command, "batch_set_material_params")
            self.assertIsInstance(params["scalar_params"], dict)
            self.assertIsInstance(params["vector_params"], dict)
            self.assertEqual(params["scalar_params"]["WindStrength"], 0.03)

        asyncio.run(run_test())


if __name__ == "__main__":
    unittest.main()
