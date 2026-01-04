"""
Configuration management for ComfyUI MCP Server.

Loads settings from .env file in the project root.
"""

import os
from pathlib import Path
from typing import Optional

# Find the .env file (in the comfyui directory)
_COMFYUI_MCP_ROOT = Path(__file__).parent.parent
_ENV_FILE = _COMFYUI_MCP_ROOT / ".env"


def _load_env():
    """Load environment variables from .env file."""
    if not _ENV_FILE.exists():
        return

    with open(_ENV_FILE, 'r') as f:
        for line in f:
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            # Parse KEY=value
            if '=' in line:
                key, _, value = line.partition('=')
                key = key.strip()
                value = value.strip()
                # Strip surrounding quotes (single or double)
                if (value.startswith('"') and value.endswith('"')) or \
                   (value.startswith("'") and value.endswith("'")):
                    value = value[1:-1]
                # Only set if not already in environment
                if key and key not in os.environ:
                    os.environ[key] = value


# Load on import
_load_env()


class Config:
    """ComfyUI MCP configuration."""

    @property
    def host(self) -> str:
        """ComfyUI server host."""
        return os.environ.get("COMFYUI_HOST", "127.0.0.1")

    @property
    def port(self) -> int:
        """ComfyUI server port."""
        return int(os.environ.get("COMFYUI_PORT", "8188"))

    @property
    def base_url(self) -> str:
        """ComfyUI server base URL."""
        return f"http://{self.host}:{self.port}"

    @property
    def comfyui_path(self) -> Optional[Path]:
        """Path to ComfyUI installation."""
        path = os.environ.get("COMFYUI_PATH", "").strip()
        if path:
            return Path(path)
        return None

    @property
    def workflows_path(self) -> Optional[Path]:
        """Path to ComfyUI workflows directory."""
        # Check explicit workflows path first
        explicit = os.environ.get("COMFYUI_WORKFLOWS_PATH", "").strip()
        if explicit:
            return Path(explicit)

        # Fall back to default location within ComfyUI
        if self.comfyui_path:
            return self.comfyui_path / "user" / "default" / "workflows"

        return None


# Global config instance
config = Config()
