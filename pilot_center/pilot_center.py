#!/usr/bin/env python3
"""
Pilot Center - WebSocket Protoo controller

CLI entry: use Config class to read a YAML configuration file.

Example:
  python pilot_center/websocket_protoo/pilot_center.py live_server/config.yaml
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Any
from logger.logger import logger, Logger
from room.room_mgr import RoomManager
from msu.msu_mgr import MsuManager
import asyncio

# Import Config class from sibling package
try:
	# Use local package path for Config loader
	from config.config import Config
	from websocket_protoo.ws_protoo_server import ServerOptions, run_server_forever
except Exception as _imp_err:
	# Provide a clearer message via logger if import fails
	logger.error(
		"Failed to import Config from config.config. Hint: run from repository root or ensure 'pilot_center/websocket_protoo' is on sys.path."
	)
	raise


def parse_args(argv: list[str]) -> argparse.Namespace:
	"""Parse command line arguments."""
	parser = argparse.ArgumentParser(
		description="Read YAML config using Config class and (placeholder) start Pilot Center",
		formatter_class=argparse.ArgumentDefaultsHelpFormatter,
	)
	parser.add_argument(
		"config",
		type=Path,
		help="Path to YAML configuration file",
	)
	parser.add_argument(
		"-v",
		"--verbose",
		action="count",
		default=0,
		help="Increase verbosity. Repeat -v for more details",
	)
	parser.add_argument(
		"--log-file",
		type=str,
		default=None,
		help="Log file path (default: ./logs/pilot_center.log)",
	)
	parser.add_argument(
		"--log-level",
		type=str,
		choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
		default="INFO",
		help="Log level",
	)
	parser.add_argument(
		"--no-console",
		action="store_true",
		help="Disable console logging (file only)",
	)
	return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
	"""Program entry point for CLI use."""
	ns = parse_args(sys.argv[1:] if argv is None else argv)
	# Initialize logger per user parameters
	import logging as _logging
	level = getattr(_logging, ns.log_level, _logging.INFO)
	if ns.verbose:
		# Each -v bumps one level up; cap at DEBUG
		level = _logging.DEBUG if ns.verbose >= 1 else level
	# Recreate logger with desired settings. Force console output disabled
	# so logs go only to the file as requested.
	global logger
	logger = Logger(log_file=ns.log_file, level=level, also_console=False).raw
	cfg = Config(ns.config)

	# Log loaded config and start websocket server
	logger.info("Loaded center config:\n%s", cfg.dump())

	# Build server options from config
	opts = ServerOptions(
		host=cfg.listen_ip,
		port=cfg.listen_port,
		cert_path=cfg.cert_path or None,
		key_path=cfg.key_path or None,
		subpath="/pilot/center",
	)
	
	# create RoomManager and pass it into the server so sessions can register
	# themselves and participate in room operations
	rm = RoomManager()
	msu_mgr = MsuManager()
	try:
		asyncio.run(run_server_forever(opts, room_manager=rm, msu_manager=msu_mgr))
	except KeyboardInterrupt:
		logger.info("Shutting down on keyboard interrupt")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())

