#!/usr/bin/env python3
"""
Project-wide logging utility.

Provides:
- Logger: wrapper over Python logging with a consistent file format
- logger: shared singleton instance for import across modules

Format:
  [YYYY-MM-DD HH:MM:SS.mmm] LEVEL file.py:LINE - message

Usage:
  from pilot_center.logger.logger import logger
  logger.info("hello")
  logger.error("oops")
"""
from __future__ import annotations

import logging
import os
from logging.handlers import RotatingFileHandler
from pathlib import Path
from typing import Optional


DEFAULT_LOG_DIR = Path.cwd() / "logs"
DEFAULT_LOG_FILE = DEFAULT_LOG_DIR / "pilot_center.log"


class Logger:
    """Configure a rotating file logger with project-wide format."""

    def __init__(
        self,
        log_file: Optional[os.PathLike[str] | str] = None,
        level: int = logging.INFO,
        rotate_bytes: int = 10 * 1024 * 1024,
        backup_count: int = 3,
        also_console: bool = True,
    ) -> None:
        # Resolve log path and ensure directory exists
        lf = Path(log_file) if log_file else DEFAULT_LOG_FILE
        lf.parent.mkdir(parents=True, exist_ok=True)

        # Core logger
        self._logger = logging.getLogger("pilot_center")
        self._logger.setLevel(level)
        self._logger.propagate = False  # avoid duplicate handlers

        # Formatter: date, time (with ms), file:line, message
        fmt = "[%(asctime)s] %(levelname)s %(filename)s:%(lineno)d - %(message)s"
        datefmt = "%Y-%m-%d %H:%M:%S"
        formatter = logging.Formatter(fmt=fmt, datefmt=datefmt)

        # File handler (rotating)
        file_handler = RotatingFileHandler(
            filename=str(lf), maxBytes=rotate_bytes, backupCount=backup_count, encoding="utf-8"
        )
        file_handler.setFormatter(formatter)
        self._attach_handler(file_handler)

        # Optional console handler
        if also_console:
            console = logging.StreamHandler()
            console.setFormatter(formatter)
            self._attach_handler(console)

    def _attach_handler(self, handler: logging.Handler) -> None:
        # Avoid attaching duplicate handlers
        for h in list(self._logger.handlers):
            if type(h) is type(handler) and getattr(h, "baseFilename", None) == getattr(handler, "baseFilename", None):
                return
        self._logger.addHandler(handler)

    # Expose common logging methods
    def debug(self, msg: str, *args, **kwargs) -> None:
        self._logger.debug(msg, *args, **kwargs)

    def info(self, msg: str, *args, **kwargs) -> None:
        self._logger.info(msg, *args, **kwargs)

    def warning(self, msg: str, *args, **kwargs) -> None:
        self._logger.warning(msg, *args, **kwargs)

    def error(self, msg: str, *args, **kwargs) -> None:
        self._logger.error(msg, *args, **kwargs)

    def exception(self, msg: str, *args, **kwargs) -> None:
        self._logger.exception(msg, *args, **kwargs)

    def critical(self, msg: str, *args, **kwargs) -> None:
        self._logger.critical(msg, *args, **kwargs)

    @property
    def raw(self) -> logging.Logger:
        """Access the underlying logging.Logger if advanced control is needed."""
        return self._logger


# Export a shared logger instance. Create without console by default so
# background launches (nohup) don't capture logs to stdout/stderr.
logger = Logger(also_console=False).raw
