"""
MSU Manager

Manages `Msu` instances keyed by `msu_id` (string from WebSocket messages).
"""
from __future__ import annotations

import logging
from typing import Dict, Optional, Iterable, TYPE_CHECKING

from .msu import Msu


class MsuManager:
	"""Manage `Msu` objects in a dictionary keyed by `msu_id`."""

	def __init__(self, logger: Optional[logging.Logger] = None) -> None:
		self._log = logger or logging.getLogger("msu.manager")
		self._items: Dict[str, Msu] = {}
		# Map room_id -> Msu for room-scoped lookups
		self._rooms: Dict[str, Msu] = {}

	def add_or_update(self, session: object, msu_id: str, alive_ms: Optional[int] = None) -> Msu:
		"""Add a new `Msu` or update existing one with latest session/alive time."""
		if not isinstance(msu_id, str) or not msu_id:
			raise ValueError("msu_id must be a non-empty string")
		item = self._items.get(msu_id)
		if item is None:
			item = Msu(session=session, msu_id=msu_id)
			self._items[msu_id] = item
			self._log.info("MSU created: %s", msu_id)
		else:
			# update session reference if changed
			item.session = session
		if alive_ms is not None:
			item.alive_ms = int(alive_ms)
		else:
			item.touch()
		return item

	def get(self, msu_id: str) -> Optional[Msu]:
		return self._items.get(msu_id)

	def remove(self, msu_id: str) -> bool:
		if msu_id in self._items:
			del self._items[msu_id]
			self._log.info("MSU removed: %s", msu_id)
			return True
		return False

	def touch(self, msu_id: str) -> None:
		item = self._items.get(msu_id)
		if item is not None:
			item.touch()

	async def handle_join_room(self, room_id: str, user_id: str, user_name: str) -> None:
		"""Handle logic when an MSU joins a room."""
		self._log.info("Handling join room for room_id: %s, user_id: %s, user_name: %s", room_id, user_id, user_name)
		msu = self.get_msu_by_roomId(room_id)
		if msu is not None:
			await msu.handle_join_room(room_id, user_id, user_name)
		else:
			self._log.warning("No MSU available to join room: %s", room_id)

	def get_msu_by_roomId(self, room_id: str) -> Optional[Msu]:
		"""Get cached Msu by room_id; if absent, pick one from items and cache it."""
		if not isinstance(room_id, str) or not room_id:
			return None
		cached = self._rooms.get(room_id)
		if cached is not None:
			return cached
		item = next(iter(self._items.values()), None)
		if item is not None:
			self._rooms[room_id] = item
		return item

	def list_ids(self) -> Iterable[str]:
		return self._items.keys()

	def prune_stale(self, ttl_ms: int, now_ms: Optional[int] = None) -> list[str]:
		"""Remove MSUs that are not alive within `ttl_ms`. Return removed ids."""
		removed: list[str] = []
		for msu_id, item in list(self._items.items()):
			if not item.is_alive(ttl_ms, now_ms):
				removed.append(msu_id)
				del self._items[msu_id]
		if removed:
			self._log.info("Pruned stale MSUs: %s", ", ".join(removed))
		return removed

