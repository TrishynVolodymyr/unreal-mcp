"""Level 1 smoke test: verify all *_mcp_server.py files import and start cleanly.

Spawns each server, waits briefly, then kills it. Pass = no traceback before
the server got far enough to be waiting on stdin.

Usage: .venv\\Scripts\\python.exe smoke_test_servers.py
Exit code: 0 if all pass, 1 if any fail.
"""

import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).parent
SERVERS = sorted(p.name for p in HERE.glob("*_mcp_server.py"))

STARTUP_WAIT_SEC = 3.0

FAILURE_KEYWORDS = (
    "Traceback",
    "ImportError",
    "ModuleNotFoundError",
    "TypeError",
    "ValidationError",
    "SyntaxError",
    "AttributeError",
)


def run_one(server: str) -> tuple[bool, str]:
    """Returns (passed, stderr_snippet)."""
    python_exe = HERE / ".venv" / "Scripts" / "python.exe"
    if not python_exe.exists():
        python_exe = Path(sys.executable)

    proc = subprocess.Popen(
        [str(python_exe), server],
        cwd=str(HERE),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        time.sleep(STARTUP_WAIT_SEC)
        still_running = proc.poll() is None
        proc.terminate()
        try:
            stdout, stderr = proc.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            stdout, stderr = proc.communicate()

        stderr_text = stderr.decode("utf-8", errors="replace")

        if any(kw in stderr_text for kw in FAILURE_KEYWORDS):
            return False, stderr_text.strip()

        if not still_running:
            return False, f"Server exited before timeout.\nSTDERR:\n{stderr_text.strip()}"

        return True, ""
    finally:
        if proc.poll() is None:
            proc.kill()


def main() -> int:
    print(f"Smoke-testing {len(SERVERS)} MCP servers...\n")
    failures = []
    for server in SERVERS:
        print(f"  {server:<40}", end="", flush=True)
        ok, err = run_one(server)
        if ok:
            print("PASS")
        else:
            print("FAIL")
            failures.append((server, err))

    print()
    if failures:
        print(f"=== {len(failures)} FAILURE(S) ===\n")
        for server, err in failures:
            print(f"--- {server} ---")
            print(err)
            print()
        return 1

    print(f"All {len(SERVERS)} servers passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
