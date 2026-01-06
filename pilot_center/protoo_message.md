# protoo message

## 1. join
Type: Request

sfu ---> pilot_center

```
{
    "request": true,
    "id": 9311734,
    "method": "join",
    "data": {
        "roomId": "6scujmas",
        "userId": "4443",
        "userName": "User_4443"
    }
}
```
Response(pilot_cent-->sfu):
```
{
    "data": {
        "code": 0,
        "message": "join success",
        "roomId": "xadfd",
        "users": [
            {
                "pushers": [
                    {
                        "mediaType": "video",
                        "pusherId": "7d4d8eed-2445-8fb0-046c-bb6c631a2199"
                    },
                    {
                        "mediaType": "audio",
                        "pusherId": "ab230cfc-0428-59e9-8bb9-40a68e145b4e"
                    }
                ],
                "userId": "4443",
                "userName": "User_4443"
            }
        ]
    },
    "id": 5616995,
    "ok": true,
    "response": true
}
```
## 2. newUser
Type: notification

pilot_center --> sfu

data:
```
{
    "notification": true,
    "method": "newUser",
    "data": {
        "roomId": "6scujmas",
        "userId": "4443",
        "userName": "User_4443"
    }
}
```
## 3. push
Type: notification

"sfu --> pilot_center" and "pilot_cent --> sfu"
```
{
    "notification": true,
    "method": "push",
    "data": {
        "roomId": "12345",
        "userId": "7890",
        "userName": "user_7890",
        "publishers": [
            {
                "pusherId": "7d4d8eed-2445-8fb0-046c-bb6c631a2199",
                "rtpParam": 
                    {
                        "av_type": "video",
                        "codec": "H264",
                        "fmtp_param": "profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1;",
                        "rtcp_features": [
                            "nack",
                            "pli"
                        ],
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
            },
            {
                "pusherId": "ab230cfc-0428-59e9-8bb9-40a68e145b4e",
                "rtpParam": 
                    {
                        "av_type": 2,
                        "codec": "opus",
                        "fmtp_param": "minptime=10;useinbandfec=1",
                        "rtcp_features": [
                            "nack"
                        ],
                        "ssrc": 23456789,
                        "payload_type": 111,
                        "clock_rate": 48000,
                        "use_nack": true,
                        "mid_ext_id": 1,
                        "tcc_ext_id": 3
                    }
            }
        ]
    }
}
```

## 4. newPusher
Type: newPusher

pilot_center --> sfu

data:
```
{
    "method": "newPusher",
    "notification": true,
    "data": {
        "pushers": [
            {
                "pusherId": "7d4d8eed-2445-8fb0-046c-bb6c631a2199",
                "rtpParam": 
                    {
                        "av_type": "video",
                        "codec": "H264",
                        "fmtp_param": "profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1;",
                        "rtcp_features": [
                            "nack",
                            "pli"
                        ],
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
            },
            {
                "pusherId": "ab230cfc-0428-59e9-8bb9-40a68e145b4e",
                "rtpParam": 
                    {
                        "av_type": 2,
                        "codec": "opus",
                        "fmtp_param": "minptime=10;useinbandfec=1",
                        "rtcp_features": [
                            "nack"
                        ],
                        "ssrc": 23456789,
                        "payload_type": 111,
                        "clock_rate": 48000,
                        "use_nack": true,
                        "mid_ext_id": 1,
                        "tcc_ext_id": 3
                    }
            }
        ],
        "roomId": "6scujmas",
        "userId": "4299",
        "userName": "User_4299"
    }
}
```

## 5. pullRemoteStream
Type: pullRemoteStream

sfu A --> pilot center --> sfu B

data:
```
{
    "roomId": "xxdd",
    "pusher_user_id": "123456",
    "udp_ip": "192.168.1.4",
    "udp_port": 10001,
    "mediaType": "video",
    "pushInfo": {
        "pusherId": "7d4d8eed-2445-8fb0-046c-bb6c631a2199",
        "rtpParam": 
            {
                "av_type": "video",
                "codec": "H264",
                "fmtp_param": "profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1;",
                "rtcp_features": [
                    "nack",
                    "pli"
                ],
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
    }
}
```