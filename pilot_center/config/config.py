#!/usr/bin/env python3
"""
Simple Config loader for WebSocket Protoo center.

Reads YAML and exposes:
  - listen_ip
  - listen_port
  - cert_path
  - key_path

CLI:
  python pilot_center/websocket_protoo/config/config.py pilot_center/websocket_protoo/center.yaml
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Any, Dict


class Config:
    """Load center YAML and expose websocket fields."""

    listen_ip: str
    listen_port: int
    cert_path: str
    key_path: str

    def __init__(self, yaml_path: str | Path) -> None:
        p = Path(yaml_path)
        data = self._load_yaml(p)
        ws = data.get("websocket")
        if not isinstance(ws, dict):
            raise SystemExit("Missing or invalid 'websocket' section in YAML")

        # Required keys
        for k in ("listen_ip", "listen_port", "cert_path", "key_path"):
            if k not in ws:
                raise SystemExit(f"Missing key in websocket section: {k}")

        self.listen_ip = str(ws.get("listen_ip", ""))
        try:
            self.listen_port = int(ws.get("listen_port", 0))
        except Exception as e:
            raise SystemExit(f"Invalid listen_port: {ws.get('listen_port')}") from e

        # Resolve cert/key relative to YAML file location if they are relative paths
        self.cert_path = self._resolve_path(p, ws.get("cert_path"))
        self.key_path = self._resolve_path(p, ws.get("key_path"))

    @staticmethod
    def _ensure_yaml() -> None:
        try:
            import yaml  # noqa: F401
        except ModuleNotFoundError:
            print(
                "PyYAML is not installed. Install it with:\n  python -m pip install PyYAML",
                file=sys.stderr,
            )
            raise SystemExit(2)

    @classmethod
    def _load_yaml(cls, path: Path) -> Dict[str, Any]:
        if not path.exists():
            print(f"Config file does not exist: {path}", file=sys.stderr)
            raise SystemExit(2)
        if not path.is_file():
            print(f"Path is not a file: {path}", file=sys.stderr)
            raise SystemExit(2)

        cls._ensure_yaml()
        import yaml  # type: ignore
        try:
            with path.open("r", encoding="utf-8") as f:
                data = yaml.safe_load(f) or {}
        except yaml.YAMLError as e:  # type: ignore[attr-defined]
            print(f"Failed to parse YAML: {e}", file=sys.stderr)
            raise SystemExit(2)
        except OSError as e:
            print(f"Failed to read file: {e}", file=sys.stderr)
            raise SystemExit(2)
        if not isinstance(data, dict):
            print("YAML root should be a mapping (dict)", file=sys.stderr)
            raise SystemExit(2)
        return data

    @staticmethod
    def _resolve_path(yaml_file: Path, path_value: Any) -> str:
        s = "" if path_value is None else str(path_value)
        if not s:
            return s
        p = Path(s)
        if p.is_absolute():
            return str(p)
        return str((yaml_file.parent / p).resolve())

    def dump(self) -> str:
        """Dump selected fields as YAML-ish string."""
        try:
            import yaml  # type: ignore
            data = {
                "websocket": {
                    "listen_ip": self.listen_ip,
                    "listen_port": self.listen_port,
                    "cert_path": self.cert_path,
                    "key_path": self.key_path,
                }
            }
            return yaml.safe_dump(data, sort_keys=False, allow_unicode=True)
        except Exception:
            return (
                "websocket:\n"
                f"  listen_ip: {self.listen_ip}\n"
                f"  listen_port: {self.listen_port}\n"
                f"  cert_path: {self.cert_path}\n"
                f"  key_path: {self.key_path}\n"
            )


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Load center.yaml and print selected fields",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("config", type=Path, help="Path to YAML configuration file")
    return parser.parse_args(argv)