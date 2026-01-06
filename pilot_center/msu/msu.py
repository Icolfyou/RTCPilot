"""
MSU module

Defines `Msu`: a lightweight object representing a managed session unit.

Key fields provided via constructor:
- `session`: a `websocket_protoo.WsProtooSession` instance representing the WebSocket session
- `msu_id`: a string identifier for the MSU
- `alive_ms`: last active timestamp in milliseconds (defaults to now if not provided)
"""
from __future__ import annotations

import time
from typing import Optional, TYPE_CHECKING
import logging


def _now_ms() -> int:
	return int(time.time() * 1000)


class Msu:
	"""
	Represents a managed session unit (MSU).
    """

	def __init__(self, session: object, msu_id: str,
	             log: Optional[logging.Logger] = None) -> None:
		"""Initialize an MSU instance.
		
		Args:
			session: WebSocket session object
			msu_id: MSU identifier string
			alive_ms: Last activity timestamp in milliseconds (defaults to now)
			log: Logger instance (defaults to "msu" logger)
		"""
		self.session = session
		self.msu_id = msu_id
		self.alive_ms = _now_ms()
		self.log = log or logging.getLogger("msu")

	def touch(self, now_ms: Optional[int] = None) -> None:
		"""Update `alive_ms` to current time or provided `now_ms`."""
		self.alive_ms = _now_ms() if now_ms is None else int(now_ms)

	def ms_since_alive(self, now_ms: Optional[int] = None) -> int:
		"""Return milliseconds elapsed since last alive timestamp."""
		now = _now_ms() if now_ms is None else int(now_ms)
		return max(0, now - self.alive_ms)

	def is_alive(self, ttl_ms: int, now_ms: Optional[int] = None) -> bool:
		"""Check if the MSU is considered alive within `ttl_ms`."""
		return self.ms_since_alive(now_ms) <= int(ttl_ms)

	def __repr__(self) -> str:
		return f"Msu(msu_id={self.msu_id!r}, alive_ms={self.alive_ms}, peer={getattr(self.session, 'peer', None)!r})"

	async def handle_join_room(self, room_id: str, user_id: str, user_name: str) -> None:
		"""Handle logic when MSU joins a room."""
		self.log.info("MSU %s joining room %s", self.msu_id, room_id)
		# send invite request message to session
		try:
			result = await self.session.send_request("invite", {
				"roomId": room_id
			})
			self.log.info("MSU %s successfully sent invite request for room %s, response: %s", 
						  self.msu_id, room_id, result)
		except Exception as e:
			self.log.error("MSU %s failed to send invite request for room %s: %s", self.msu_id, room_id, e)