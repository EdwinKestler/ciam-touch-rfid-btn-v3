# MQTT JSON Contract -- CIAM Touch RFID Button v3

JSON schema reference for backend systems integrating with the device firmware (v6.00).

All payloads are UTF-8 JSON. Maximum payload size: 512 bytes (`MQTT_MAX_PACKET_SIZE`).

---

## Topics Overview

| Direction | Topic | QoS | Retain | Purpose |
| --- | --- | --- | --- | --- |
| Device -> Broker | `iot-2/evt/status/fmt/json` | 0 | false | Sensor events (button press, RFID card read) |
| Device -> Broker | `iotdevice-1/mgmt/manage` | 0 | true | Heartbeat, device metadata, LWT |
| Broker -> Device | `iotdm-1/response/` | 0 | -- | Server acknowledgments |
| Broker -> Device | `iotdm-1/device/update` | 0 | -- | Remote parameter changes |
| Broker -> Device | `iotdm-1/mgmt/d7e3f1a2` | 0 | -- | Remote reboot (obscured topic) |
| Broker -> Device | `iotdm-1/device/ctrl` | 0 | -- | RGB LED and buzzer control |

---

## Device -> Broker Payloads

### 1. Button Press Event

**Topic:** `iot-2/evt/status/fmt/json`
**Trigger:** Capacitive touch button pressed
**Retain:** false

```json
{
  "d": {
    "botondata": {
      "ChipID": "782184",
      "IDEventoBoton": "782184-3",
      "Tstamp": "2026-03-06T14:30:00",
      "seq": 5
    }
  }
}
```

| Field | Type | Description |
| --- | --- | --- |
| `ChipID` | string | ESP8266 chip ID (unique per device, numeric string) |
| `IDEventoBoton` | string | Event ID: `{ChipID}-{count}`. Count increments per press within a boot session (resets on reboot). |
| `Tstamp` | string | ISO 8601 timestamp (no timezone suffix -- device local time, UTC offset configured by `timeZone`). Format: `YYYY-MM-DDThh:mm:ss` (19 chars). Empty string `""` if NTP never synced. |
| `seq` | integer | Monotonic message sequence number (increments across ALL message types since boot). Use gaps to detect lost messages. |

**Buffer limit:** 300 bytes. Truncation logged to device serial if exceeded.

---

### 2. RFID Card Read Event

**Topic:** `iot-2/evt/status/fmt/json`
**Trigger:** Valid RFID card presented (passes frame validation, duplicate guard, and rate limit)
**Retain:** false

```json
{
  "d": {
    "tagdata": {
      "ChipID": "782184",
      "IDeventoTag": "782184-1",
      "Tstamp": "2026-03-06T14:30:05",
      "Tag": "7B226F",
      "seq": 6
    }
  }
}
```

| Field | Type | Description |
| --- | --- | --- |
| `ChipID` | string | ESP8266 chip ID |
| `IDeventoTag` | string | Event ID: `{ChipID}-{count}`. Count increments per card publish within a boot session. |
| `Tstamp` | string | ISO 8601 timestamp (same format as button event) |
| `Tag` | string | RFID card identifier extracted from bytes 4-7 of the 13-byte reader frame. Content is reader-model-dependent (typically hex-encoded). |
| `seq` | integer | Monotonic message sequence number |

**Rate limiting:** Same card ID can only be published once per 60 seconds. Duplicate reads within 5 seconds are silently dropped (no event generated).

**Buffer limit:** 300 bytes.

---

### 3. Heartbeat (Device Status)

**Topic:** `iotdevice-1/mgmt/manage`
**Trigger:** Every `heartbeat_minutes` (default 30 min), on low WiFi alarm, and before 24h reboot
**Retain:** true (broker stores last heartbeat; new subscribers receive it immediately)

```json
{
  "d": {
    "Ddata": {
      "ChipID": "782184",
      "DeviceID": "CIAM",
      "Msg": "on",
      "FW": "V6.00",
      "HW": "V3.00",
      "uptime": 3600,
      "free_heap": 28456,
      "hora": 1,
      "batt": 3.72,
      "RSSI": -52,
      "SSID": "OfficeWiFi",
      "Location": "Building A, Floor 2",
      "seq": 7,
      "boot_reason": "Power On",
      "publicados": 15,
      "enviados": 18,
      "fallidos": 3,
      "Tstamp": "2026-03-06T15:30:00",
      "Mac": "AA:BB:CC:DD:EE:FF",
      "Ip": "192.168.1.42"
    }
  }
}
```

| Field | Type | Description |
| --- | --- | --- |
| `ChipID` | string | ESP8266 chip ID |
| `DeviceID` | string | User-configurable device identifier (default `"CIAM"`, set via WiFiManager or LittleFS config) |
| `Msg` | string | Device status. Values: `"on"` (normal), `"LOWiFi"` (RSSI below threshold), `"24h Normal Reset"` (before scheduled reboot) |
| `FW` | string | Firmware version (e.g., `"V6.00"`) |
| `HW` | string | Hardware version (e.g., `"V3.00"`) |
| `uptime` | integer | Seconds since boot (`millis() / 1000`). Wraps at ~49 days (uint32 overflow). |
| `free_heap` | integer | Free RAM in bytes (`ESP.getFreeHeap()`). Healthy range: 20,000-40,000. Below 10,000 indicates memory pressure. |
| `hora` | integer | Hours since boot (incremented every 60 min). Resets to 0 on 24h reboot. |
| `batt` | float | Battery voltage from ADC (A0 pin). 0.0 if no battery connected. |
| `RSSI` | integer | WiFi signal strength in dBm (negative). Typical: -30 (excellent) to -90 (unusable). Alarm threshold default: -75. |
| `SSID` | string | Connected WiFi network name |
| `Location` | string | Installation location (user-configured via WiFiManager or MQTT). Empty string if not set. |
| `seq` | integer | Monotonic message sequence number |
| `boot_reason` | string | ESP8266 reset cause. Values: `"Power On"`, `"External System"`, `"Software/System restart"`, `"Hardware Watchdog"`, `"Exception"`, `"Deep-Sleep Wake"`, etc. |
| `publicados` | integer | Successful MQTT publishes since boot |
| `enviados` | integer | Total MQTT publish attempts since boot |
| `fallidos` | integer | Failed MQTT publishes since boot (halved on each success, device restarts when >= `fail_threshold`) |
| `Tstamp` | string | ISO 8601 timestamp |
| `Mac` | string | WiFi MAC address (`AA:BB:CC:DD:EE:FF` format) |
| `Ip` | string | Assigned IP address |

**Buffer limit:** 600 bytes.

---

### 4. Last Will and Testament (LWT)

**Topic:** `iotdevice-1/mgmt/manage`
**Trigger:** Published by the **broker** when the device disconnects unexpectedly (TCP keepalive timeout)
**Retain:** true (overwrites last heartbeat)

```json
{
  "d": {
    "Ddata": {
      "Msg": "offline"
    }
  }
}
```

**Backend logic:** When `Msg` equals `"offline"`, the device is unreachable. The next heartbeat (with `Msg: "on"`) indicates recovery.

---

### 5. Device Metadata (on connect/reconnect)

**Topic:** `iotdevice-1/mgmt/manage`
**Trigger:** Published once on initial MQTT connect and on every reconnect
**Retain:** false

```json
{
  "d": {
    "metadata": {
      "publish_interval": 1000,
      "btn_hold_time": 2000,
      "tono_corto": 250,
      "tono_medio": 500,
      "tono_largo": 1000,
      "flash_corto": 250,
      "flash_medio": 500,
      "flash_largo": 1000,
      "fail_threshold": 150,
      "rssi_threshold": -75,
      "heartbeat_minutes": 30,
      "timeZone": -6,
      "location": "Building A, Floor 2",
      "wifi_ssid": "OfficeWiFi",
      "fallback_ssid": "OfficeWiFi_5G"
    },
    "supports": {
      "deviceActions": true
    },
    "deviceInfo": {
      "NTP_Server": "time-a-g.nist.gov",
      "MQTT_server": "172.18.98.142",
      "MacAddress": "AA:BB:CC:DD:EE:FF",
      "IPAddress": "192.168.1.42"
    }
  }
}
```

This payload reports the device's current runtime configuration. All `metadata` fields correspond to remotely updatable parameters (see section below).

---

## Broker -> Device Payloads

### 6. Remote Parameter Update

**Topic:** `iotdm-1/device/update`

All fields are optional. Include only the parameters you want to change. Multiple parameters can be sent in a single message. Changes are **runtime only** and reset to defaults on reboot (except `location`, `wifi_*`, and `fallback_*` which persist to LittleFS).

```json
{
  "publish_interval": 2000,
  "heartbeat_minutes": 15,
  "fail_threshold": 100
}
```

| Field | Type | Range | Default | Persists | Description |
| --- | --- | --- | --- | --- | --- |
| `publish_interval` | integer (ms) | > 0 | 1000 | No | Base time unit. Affects heartbeat (N x 60 x interval), NTP resync (3600 x interval), 24h reset (3600 x interval). |
| `btn_hold_time` | integer (ms) | > 0 | 2000 | No | Button hold time for config/OTA mode at startup |
| `tono_corto` | integer (ms) | > 0 | 250 | No | Short beep duration |
| `tono_medio` | integer (ms) | > 0 | 500 | No | Medium beep duration |
| `tono_largo` | integer (ms) | > 0 | 1000 | No | Long beep duration |
| `flash_corto` | integer (ms) | > 0 | 250 | No | Short LED flash duration |
| `flash_medio` | integer (ms) | > 0 | 500 | No | Medium LED flash duration |
| `flash_largo` | integer (ms) | > 0 | 1000 | No | Long LED flash duration |
| `fail_threshold` | integer | > 0 | 150 | No | MQTT publish failures before auto-reboot |
| `rssi_threshold` | integer (dBm) | -100 to 0 | -75 | No | WiFi signal alarm threshold |
| `heartbeat_minutes` | integer (min) | 1-1440 | 30 | No | Heartbeat publish interval (clamped) |
| `location` | string | max 63 chars | `""` | **Yes** | Installation location (saved to LittleFS immediately) |
| `wifi_ssid` + `wifi_pass` | string | max 31 chars each | -- | **Yes** | Push new WiFi credentials (stored as fallback) |
| `fallback_ssid` + `fallback_pass` | string | max 31 chars each | -- | **Yes** | Set/update fallback WiFi network |
| `ota` | boolean | `true` | -- | No | Triggers OTA update mode (device creates `RFID_OTA` AP) |

---

### 7. Remote Reboot

**Topic:** `iotdm-1/mgmt/d7e3f1a2`

Requires exact JSON token. Any other payload is rejected.

```json
{"k": "A5F0"}
```

| Field | Type | Required Value | Description |
| --- | --- | --- | --- |
| `k` | string | `"A5F0"` | Authorization token. Device resets immediately on match. Invalid tokens, non-JSON, and empty payloads are rejected with serial warning. |

---

### 8. RGB LED Control

**Topic:** `iotdm-1/device/ctrl`

Three formats accepted (mutually exclusive -- first match wins):

**RGB values (PWM 0-255 per channel):**

```json
{"r": 255, "g": 128, "b": 0}
```

| Field | Type | Range | Description |
| --- | --- | --- | --- |
| `r` | integer | 0-255 | Red channel intensity |
| `g` | integer | 0-255 | Green channel intensity |
| `b` | integer | 0-255 | Blue channel intensity |

**Hex color:**

```json
{"hex": "#FF8000"}
```

| Field | Type | Format | Description |
| --- | --- | --- | --- |
| `hex` | string | 6 hex chars, optional `#` prefix | RGB color in hex notation |

**On/Off command:**

```json
{"cmd": "on"}
{"cmd": "off"}
```

| Field | Type | Values | Description |
| --- | --- | --- | --- |
| `cmd` | string | `"on"`, `"off"` | `"on"` sets all channels to 255; `"off"` sets all to 0 |

---

### 9. Buzzer Control

**Topic:** `iotdm-1/device/ctrl`

```json
{"beep": 5}
```

| Field | Type | Range | Description |
| --- | --- | --- | --- |
| `beep` | integer or string | 1-300 (seconds) | Activates buzzer for specified duration. Non-blocking -- FSM continues processing events. Accepts both `{"beep": 5}` and `{"beep": "60"}`. |

---

## Discriminating Payloads on Shared Topics

### `iot-2/evt/status/fmt/json` (sensor events)

Both button and RFID events share this topic. Discriminate by the key inside `d`:

| Key present | Event type |
| --- | --- |
| `d.botondata` | Button press |
| `d.tagdata` | RFID card read |

```python
# Python example
data = json.loads(payload)["d"]
if "botondata" in data:
    handle_button(data["botondata"])
elif "tagdata" in data:
    handle_card(data["tagdata"])
```

### `iotdevice-1/mgmt/manage` (device status)

Three payload types share this topic. Discriminate by structure:

| Key present | Payload type |
| --- | --- |
| `d.Ddata.Msg` == `"offline"` | LWT (device disconnected) |
| `d.Ddata` (with `ChipID`, etc.) | Heartbeat |
| `d.metadata` | Device metadata (on connect) |

```python
# Python example
data = json.loads(payload)["d"]
if "metadata" in data:
    handle_metadata(data["metadata"])
elif "Ddata" in data:
    if data["Ddata"].get("Msg") == "offline":
        handle_offline(data["Ddata"])
    else:
        handle_heartbeat(data["Ddata"])
```

---

## MQTT Client ID Format

```
d:FLATBOX:AC_WIFI_RFID_BTN:{DeviceID}{ChipID}
```

Example: `d:FLATBOX:AC_WIFI_RFID_BTN:CIAM782184`

---

## Sequence Number (`seq`) Contract

- Present in **all** device-published payloads (button, card, heartbeat).
- Starts at 1 on boot, increments by 1 for every MQTT publish attempt (regardless of success/failure).
- Never resets during a session. Resets to 0 on reboot.
- **Gap detection:** If backend sees `seq` jump from N to N+3, two messages were lost.
- **Reboot detection:** If `seq` drops (e.g., 150 -> 1), device rebooted. Correlate with `boot_reason` in the next heartbeat.

---

## Timestamp Contract

- Format: `YYYY-MM-DDThh:mm:ss` (19 characters, no `Z` suffix, no milliseconds).
- Timezone: Device-local time. Offset from UTC is configured by `timeZone` constant (default: -6, Central Time).
- Empty string `""` if NTP sync has never succeeded since boot.
- Updated before every publish via `ntp_check_time()`. Accuracy depends on last successful NTP sync (resync attempted every 60 minutes).
- NTP server configurable per device via LittleFS config (default: `time-a-g.nist.gov`).

---

## Device Identity Fields

| Field | Source | Uniqueness | Mutable |
| --- | --- | --- | --- |
| `ChipID` | `ESP.getChipId()` | Globally unique per ESP8266 chip | No (hardware) |
| `DeviceID` | LittleFS `config.json` | User-assigned (default `"CIAM"`) | Yes (WiFiManager) |
| `Mac` | `WiFi.macAddress()` | Unique per device | No (hardware) |
| `Ip` | DHCP-assigned | Changes per network | Yes (automatic) |

**Recommended primary key:** `ChipID` (immutable, unique, always present in every payload).

---

## Payload Size Limits

| Payload | Buffer (bytes) | Typical size | Overflow behavior |
| --- | --- | --- | --- |
| Button event | 300 | ~120 | Truncated JSON, `WARNING: Boton JSON truncated!` logged |
| RFID event | 300 | ~140 | Truncated JSON, `WARNING: Tarjeta JSON truncated!` logged |
| Heartbeat | 600 | ~450 | Truncated JSON, `WARNING: Manejo JSON truncated!` logged |
| Device metadata | 600 | ~400 | Silent truncation |
| MQTT packet (total) | 512 | -- | PubSubClient drops the publish silently |

**Note:** If `MQTT_MAX_PACKET_SIZE` (512) is smaller than the serialized payload + MQTT header + topic length, `client.publish()` returns false and the `failed` counter increments.
