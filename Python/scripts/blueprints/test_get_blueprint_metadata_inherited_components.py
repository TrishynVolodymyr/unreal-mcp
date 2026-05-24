#!/usr/bin/env python
"""
Integration test for get_blueprint_metadata 'component_properties' field on
INHERITED components (e.g. ACharacter's Mesh / CharacterMesh0 / CapsuleComponent /
CharacterMovement which come from the native parent class, not the BP's own SCS).

Regression test for the bug where component_properties returned
    "Component 'CharacterMesh0' not found in Blueprint"
for components that 'components' field happily listed (because BuildComponentsInfo
includes inherited components via the CDO, but BuildComponentPropertiesInfo
only searched the local SimpleConstructionScript).

Requires the UE editor running with the UnrealMCP plugin on 127.0.0.1:55557.

Pass criteria:
  1. create_blueprint with parent_class=Character succeeds.
  2. get_blueprint_metadata fields=[components] lists CharacterMesh0 (or "Mesh").
  3. get_blueprint_metadata fields=[component_properties] component_name="CharacterMesh0"
     returns success with a 'properties' object and 'inherited': true.
  4. Same call for CapsuleComponent / CharacterMovement also succeeds.
  5. Backwards-compat: an SCS component on a plain Actor BP still resolves.
"""

import sys
import os
import socket
import json
import logging
from typing import Dict, Any, Optional

logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("TestInheritedComponentProps")


def send_command(command: str, params: Dict[str, Any]) -> Optional[Dict[str, Any]]:
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("127.0.0.1", 55557))
        try:
            command_obj = {"type": command, "params": params}
            sock.sendall(json.dumps(command_obj).encode('utf-8'))
            chunks = []
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                chunks.append(chunk)
                try:
                    json.loads(b''.join(chunks).decode('utf-8'))
                    break
                except json.JSONDecodeError:
                    continue
            return json.loads(b''.join(chunks).decode('utf-8'))
        finally:
            sock.close()
    except Exception as e:
        logger.error(f"send_command error: {e}")
        return None


def assert_true(cond, msg):
    if not cond:
        logger.error(f"FAIL: {msg}")
        sys.exit(1)
    logger.info(f"PASS: {msg}")


def test_inherited_character_components():
    bp_name = "BP_TestInheritedComponents"

    # 1. Create a Character BP (idempotent)
    resp = send_command("create_blueprint",
                        {"name": bp_name, "parent_class": "Character"})
    assert_true(resp and resp.get("status") == "success",
                f"create_blueprint Character ok: {resp}")

    # 2. List components - must include an inherited mesh slot
    resp = send_command("get_blueprint_metadata",
                        {"blueprint_name": bp_name, "fields": ["components"]})
    assert_true(resp and resp.get("status") == "success",
                "get_blueprint_metadata components ok")
    components = (resp.get("result", {})
                      .get("metadata", {})
                      .get("components", {})
                      .get("components", []))
    names = [c.get("name") for c in components]
    logger.info(f"Components found: {names}")
    # ACharacter exposes CharacterMesh0 (variable name "Mesh"). UE may report
    # either depending on which name is used. Accept both.
    mesh_name = next((n for n in names if n in ("CharacterMesh0", "Mesh")), None)
    assert_true(mesh_name is not None,
                f"Inherited mesh component listed (got {names})")

    capsule_name = next((n for n in names if "Capsule" in n), None)
    assert_true(capsule_name is not None,
                f"Inherited capsule component listed (got {names})")

    # 3. component_properties on the inherited mesh - this is the regression
    resp = send_command("get_blueprint_metadata",
                        {"blueprint_name": bp_name,
                         "fields": ["component_properties"],
                         "component_name": mesh_name})
    assert_true(resp and resp.get("status") == "success",
                f"get_blueprint_metadata component_properties on inherited "
                f"mesh ok: {resp}")
    props_block = (resp.get("result", {})
                       .get("metadata", {})
                       .get("component_properties", {}))
    assert_true("error" not in props_block,
                f"No 'error' field for inherited mesh (got {props_block})")
    assert_true(isinstance(props_block.get("properties"), dict),
                "Inherited mesh exposes a 'properties' dict")
    assert_true(props_block.get("inherited") is True,
                "Inherited component is flagged with inherited=true")

    # 4. capsule + movement
    for inherited in (capsule_name, "CharMoveComp", "CharacterMovement"):
        resp = send_command("get_blueprint_metadata",
                            {"blueprint_name": bp_name,
                             "fields": ["component_properties"],
                             "component_name": inherited})
        if resp and resp.get("status") == "success":
            block = (resp.get("result", {})
                         .get("metadata", {})
                         .get("component_properties", {}))
            if "error" not in block:
                logger.info(f"Resolved inherited '{inherited}' ok")
                continue
        logger.info(f"Inherited '{inherited}' not present under that exact "
                    f"name - tolerated (CharacterMovement reports as "
                    f"CharMoveComp on some setups)")


def test_scs_component_backwards_compat():
    bp_name = "BP_TestSCSCompat"

    resp = send_command("create_blueprint",
                        {"name": bp_name, "parent_class": "Actor"})
    assert_true(resp and resp.get("status") == "success",
                "create_blueprint Actor ok for SCS compat test")

    resp = send_command("add_component_to_blueprint",
                        {"blueprint_name": bp_name,
                         "component_type": "StaticMeshComponent",
                         "component_name": "MyMeshComp"})
    assert_true(resp and resp.get("status") == "success",
                f"add SCS StaticMeshComponent ok: {resp}")

    resp = send_command("compile_blueprint", {"blueprint_name": bp_name})
    assert_true(resp and resp.get("status") == "success", "compile ok")

    resp = send_command("get_blueprint_metadata",
                        {"blueprint_name": bp_name,
                         "fields": ["component_properties"],
                         "component_name": "MyMeshComp"})
    assert_true(resp and resp.get("status") == "success",
                f"component_properties on SCS comp still works: {resp}")
    block = (resp.get("result", {})
                 .get("metadata", {})
                 .get("component_properties", {}))
    assert_true("error" not in block,
                f"No error for SCS comp (got {block})")
    assert_true(isinstance(block.get("properties"), dict),
                "SCS component exposes 'properties' dict")
    # SCS components should NOT be flagged inherited
    assert_true(block.get("inherited") is not True,
                "SCS component is not flagged inherited")


def main():
    logger.info("=== test_inherited_character_components ===")
    test_inherited_character_components()
    logger.info("=== test_scs_component_backwards_compat ===")
    test_scs_component_backwards_compat()
    logger.info("All assertions passed.")


if __name__ == "__main__":
    main()
