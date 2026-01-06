"""pilot_center.room.push_info

定义 PushInfo 类：包含 pusherId 和 RtpParam 对象，并提供序列化工具。
"""

from __future__ import annotations

from dataclasses import dataclass, field, asdict
from typing import Optional, Dict, Any
import json

from .rtp_param import RtpParam


@dataclass
class PushInfo:
	"""表示发布者信息的简单类。

	字段:
		pusherId: 发布者唯一标识字符串
		rtpParam: 对应的 `RtpParam` 对象（可选）
	"""

	pusherId: Optional[str] = None
	rtpParam: Optional[RtpParam] = None

	# --- getters / setters -----------------------------------------------
	def get_pusher_id(self) -> Optional[str]:
		return self.pusherId

	def set_pusher_id(self, v: Optional[str]) -> None:
		self.pusherId = None if v is None else str(v)

	def get_rtp_param(self) -> Optional[RtpParam]:
		return self.rtpParam

	def set_rtp_param(self, param: Optional[RtpParam]) -> None:
		if param is None:
			self.rtpParam = None
		elif isinstance(param, RtpParam):
			self.rtpParam = param
		elif isinstance(param, dict):
			# 支持直接传入 dict
			self.rtpParam = RtpParam.from_dict(param)
		else:
			raise TypeError("rtpParam must be RtpParam, dict or None")

	# --- serialization --------------------------------------------------
	def to_dict(self) -> Dict[str, Any]:
		data: Dict[str, Any] = {}
		if self.pusherId is not None:
			data["pusherId"] = self.pusherId
		if self.rtpParam is not None:
			# RtpParam 已提供 .to_dict()
			data["rtpParam"] = self.rtpParam.to_dict()
		return data

	def to_json(self) -> str:
		return json.dumps(self.to_dict(), ensure_ascii=False)

	@classmethod
	def from_dict(cls, data: Dict[str, Any]) -> "PushInfo":
		if data is None:
			raise ValueError("data is None")
		pusherId = data.get("pusherId")
		rtp = data.get("rtpParam")
		rtp_obj = None
		if rtp is not None:
			if isinstance(rtp, RtpParam):
				rtp_obj = rtp
			elif isinstance(rtp, dict):
				rtp_obj = RtpParam.from_dict(rtp)
			else:
				raise TypeError("rtpParam must be dict or RtpParam")
		return cls(pusherId=pusherId, rtpParam=rtp_obj)

	@classmethod
	def from_json(cls, payload: Any) -> "PushInfo":
		if isinstance(payload, str):
			try:
				data = json.loads(payload)
			except json.JSONDecodeError as e:
				raise ValueError(f"invalid json: {e}")
		elif isinstance(payload, dict):
			data = payload
		else:
			raise TypeError("payload must be JSON string or dict")
		return cls.from_dict(data)

	def __repr__(self) -> str:  # pragma: no cover - 简单表示
		return f"PushInfo({self.to_dict()})"
