#!/usr/bin/env python3
"""
Config loader for Pilot Center / WebSocket Protoo.

- Reads a YAML file (e.g., pilot_center/websocket_protoo/center.yaml)
- Validates required fields
- Provides dump() to return a formatted string representation

Usage:
  python -m config.config pilot_center/websocket_protoo/center.yaml
"""
from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Optional


# -------- YAML helpers --------

def _ensure_yaml_available() -> None:
    try:
        import yaml  # noqa: F401
    except ModuleNotFoundError:
        print(
            "PyYAML is not installed. Install it with:\n  python -m pip install PyYAML",
            file=sys.stderr,
        )
        raise SystemExit(2)


def _load_yaml(path: Path) -> Dict[str, Any]:
    if not path.exists():
        print(f"Config file does not exist: {path}", file=sys.stderr)
        raise SystemExit(2)
    if not path.is_file():
        print(f"Path is not a file: {path}", file=sys.stderr)
        raise SystemExit(2)

    _ensure_yaml_available()
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


# -------- Data models --------

@dataclass
class WebsocketConfig:
    listen_ip: str
    listen_port: int
    cert_path: str
    key_path: str

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "WebsocketConfig":
        missing = [k for k in ("listen_ip", "listen_port", "cert_path", "key_path") if k not in d]
        if missing:
            raise ValueError(f"Missing websocket config keys: {', '.join(missing)}")
        # Basic type normalization
        listen_ip = str(d["listen_ip"]) if d["listen_ip"] is not None else ""
        try:
            listen_port = int(d["listen_port"])  # allow string->int
        except Exception as e:
            raise ValueError(f"Invalid listen_port: {d.get('listen_port')}") from e
        cert_path = str(d["cert_path"]) if d["cert_path"] is not None else ""
        key_path = str(d["key_path"]) if d["key_path"] is not None else ""
        return cls(
            listen_ip=listen_ip,
            listen_port=listen_port,
            cert_path=cert_path,
            key_path=key_path,
        )

    def to_dict(self) -> Dict[str, Any]:
        return {
            "listen_ip": self.listen_ip,
            "listen_port": self.listen_port,
            "cert_path": self.cert_path,
            "key_path": self.key_path,
        }


@dataclass
class CenterConfig:
    websocket: WebsocketConfig
    source_path: Optional[Path] = None  # original YAML file path

    @classmethod
    def from_file(cls, path: str | Path) -> "CenterConfig":
        p = Path(path)
        data = _load_yaml(p)
        ws = data.get("websocket")
        if not isinstance(ws, dict):
            raise SystemExit("Missing or invalid 'websocket' section in YAML")
        websocket = WebsocketConfig.from_dict(ws)
        return cls(websocket=websocket, source_path=p)

    def to_dict(self) -> Dict[str, Any]:
        return {"websocket": self.websocket.to_dict()}

    def dump(self) -> str:
        """Dump the config to a human-readable string (YAML if available)."""
        try:
            import yaml  # type: ignore
            return yaml.safe_dump(self.to_dict(), sort_keys=False, allow_unicode=True)
        except Exception:
            # Fallback to a simple manual format
            ws = self.websocket
            return (
                "websocket:\n"
                f"  listen_ip: {ws.listen_ip}\n"
                f"  listen_port: {ws.listen_port}\n"
                f"  cert_path: {ws.cert_path}\n"
                f"  key_path: {ws.key_path}\n"
            )


# -------- CLI --------

def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Load YAML config and print dump string",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("config", type=Path, help="Path to YAML configuration file")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    ns = _parse_args(sys.argv[1:] if argv is None else argv)
    cfg = CenterConfig.from_file(ns.config)
    print(cfg.dump(), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
