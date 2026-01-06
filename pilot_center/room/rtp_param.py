"""pilot_center.room.rtpParam

提供 RtpParam 类，表示 RTP/媒体相关参数。

示例数据结构:

{
    "av_type": "video",
    "codec": "H264",
    "fmtp_param": "profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1;",
    "rtcp_features": ["nack", "pli"],
    "channel": 2,
    "ssrc": 12345678,
    "payload_type": 96,
    "clock_rate": 90000,
    "rtx_ssrc": 87654321,
    "rtx_payload_type": 97,
    "use_nack": true,
    "key_request": true,
    "mid_ext_id": 1,
    "tcc_ext_id": 3
}

类功能：
- 支持按字段读取/设置（getter/setter）
- 序列化为 dict / JSON 字符串（dump/to_json）
- 从 dict 或 JSON 字符串构造对象（from_dict/from_json）
"""

from __future__ import annotations

from dataclasses import dataclass, field, asdict
from typing import List, Optional, Any, Dict
import json


@dataclass
class RtpParam:
    """表示 RTP 参数的简单数据类。

    所有字段均对应用户示例。字段类型基于常见含义进行定义。
    """

    av_type: Optional[str] = None
    codec: Optional[str] = None
    fmtp_param: Optional[str] = None
    rtcp_features: List[str] = field(default_factory=list)
    channel: Optional[int] = None
    ssrc: Optional[int] = None
    payload_type: Optional[int] = None
    clock_rate: Optional[int] = None
    rtx_ssrc: Optional[int] = None
    rtx_payload_type: Optional[int] = None
    use_nack: Optional[bool] = None
    key_request: Optional[bool] = None
    mid_ext_id: Optional[int] = None
    tcc_ext_id: Optional[int] = None

    # --- Getters and setters -------------------------------------------------
    # 为兼容现有代码，提供显式的 get_x / set_x 方法

    def get_av_type(self) -> Optional[str]:
        return self.av_type

    def set_av_type(self, v: Optional[str]) -> None:
        self.av_type = v

    def get_codec(self) -> Optional[str]:
        return self.codec

    def set_codec(self, v: Optional[str]) -> None:
        self.codec = v

    def get_fmtp_param(self) -> Optional[str]:
        return self.fmtp_param

    def set_fmtp_param(self, v: Optional[str]) -> None:
        self.fmtp_param = v

    def get_rtcp_features(self) -> List[str]:
        return list(self.rtcp_features)

    def set_rtcp_features(self, features: List[str]) -> None:
        # 简单验证：确保是字符串列表
        if features is None:
            self.rtcp_features = []
        else:
            self.rtcp_features = [str(x) for x in features]

    def get_channel(self) -> Optional[int]:
        return self.channel

    def set_channel(self, v: Optional[int]) -> None:
        self.channel = None if v is None else int(v)

    def get_ssrc(self) -> Optional[int]:
        return self.ssrc

    def set_ssrc(self, v: Optional[int]) -> None:
        self.ssrc = None if v is None else int(v)

    def get_payload_type(self) -> Optional[int]:
        return self.payload_type

    def set_payload_type(self, v: Optional[int]) -> None:
        self.payload_type = None if v is None else int(v)

    def get_clock_rate(self) -> Optional[int]:
        return self.clock_rate

    def set_clock_rate(self, v: Optional[int]) -> None:
        self.clock_rate = None if v is None else int(v)

    def get_rtx_ssrc(self) -> Optional[int]:
        return self.rtx_ssrc

    def set_rtx_ssrc(self, v: Optional[int]) -> None:
        self.rtx_ssrc = None if v is None else int(v)

    def get_rtx_payload_type(self) -> Optional[int]:
        return self.rtx_payload_type

    def set_rtx_payload_type(self, v: Optional[int]) -> None:
        self.rtx_payload_type = None if v is None else int(v)

    def get_use_nack(self) -> Optional[bool]:
        return self.use_nack

    def set_use_nack(self, v: Optional[bool]) -> None:
        self.use_nack = None if v is None else bool(v)

    def get_key_request(self) -> Optional[bool]:
        return self.key_request

    def set_key_request(self, v: Optional[bool]) -> None:
        self.key_request = None if v is None else bool(v)

    def get_mid_ext_id(self) -> Optional[int]:
        return self.mid_ext_id

    def set_mid_ext_id(self, v: Optional[int]) -> None:
        self.mid_ext_id = None if v is None else int(v)

    def get_tcc_ext_id(self) -> Optional[int]:
        return self.tcc_ext_id

    def set_tcc_ext_id(self, v: Optional[int]) -> None:
        self.tcc_ext_id = None if v is None else int(v)

    # --- Serialization ------------------------------------------------------
    def to_dict(self) -> Dict[str, Any]:
        """将对象转换为字典（适合 json 序列化）。

        仅包含非 None 值以保持输出简洁。
        """
        data = asdict(self)
        # 移除值为 None 的字段
        return {k: v for k, v in data.items() if v is not None}


    # --- Deserialization ----------------------------------------------------
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "RtpParam":
        """从字典构造 RtpParam 对象。会做最小类型转换。"""
        if data is None:
            raise ValueError("data is None")
        # 拷贝以防修改原始 dict
        d = dict(data)
        # 保证 rtcp_features 是列表
        rtcp_features = d.get("rtcp_features")
        if rtcp_features is None:
            d["rtcp_features"] = []
        else:
            d["rtcp_features"] = [str(x) for x in rtcp_features]

        # 将布尔/整数字段做类型转换（如果存在）
        int_fields = [
            "channel",
            "ssrc",
            "payload_type",
            "clock_rate",
            "rtx_ssrc",
            "rtx_payload_type",
            "mid_ext_id",
            "tcc_ext_id",
        ]
        for f in int_fields:
            if f in d and d[f] is not None:
                try:
                    d[f] = int(d[f])
                except Exception:
                    raise ValueError(f"field {f} must be int-like, got {d[f]!r}")

        bool_fields = ["use_nack", "key_request"]
        for f in bool_fields:
            if f in d and d[f] is not None:
                d[f] = bool(d[f])

        # 其余字段直接传递
        return cls(
            av_type=d.get("av_type"),
            codec=d.get("codec"),
            fmtp_param=d.get("fmtp_param"),
            rtcp_features=d.get("rtcp_features", []),
            channel=d.get("channel"),
            ssrc=d.get("ssrc"),
            payload_type=d.get("payload_type"),
            clock_rate=d.get("clock_rate"),
            rtx_ssrc=d.get("rtx_ssrc"),
            rtx_payload_type=d.get("rtx_payload_type"),
            use_nack=d.get("use_nack"),
            key_request=d.get("key_request"),
            mid_ext_id=d.get("mid_ext_id"),
            tcc_ext_id=d.get("tcc_ext_id"),
        )

    @classmethod
    def from_json(cls, payload: Any) -> "RtpParam":
        """从 JSON 字符串或字典创建对象。

        payload: JSON 字符串或已解析的字典。
        """
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

    # --- Helper / representation -------------------------------------------
    def dump(self) -> Dict[str, Any]:
        """兼容命名：返回字典形式（等价于 to_dict）。"""
        return self.to_dict()

    def __repr__(self) -> str:  # pragma: no cover - simple repr
        return f"RtpParam({self.to_dict()})"
